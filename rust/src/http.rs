//! HTTP transport layer for the RDS protocol.
//!
//! Port of `src/cfrds_http.c` from the C library. Sends plain HTTP POST
//! requests to `/CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=<command>` over a raw TCP
//! socket. TLS is deliberately not supported (matching the C library, which
//! targets remote ColdFusion development/debugging rather than production).

use std::io::{Read, Write};
use std::net::{TcpStream, ToSocketAddrs};
use std::time::{Duration, Instant};

use crate::error::{Result, Status};
use crate::server::Server;

/// Maximum accepted response size (mirrors `CFRDS_MAX_RESPONSE_SIZE`).
const CFRDS_MAX_RESPONSE_SIZE: usize = 100 * 1024 * 1024;
/// Overall response read deadline in seconds (mirrors `CFRDS_MAX_RESPONSE_TIMEOUT_SEC`).
const CFRDS_MAX_RESPONSE_TIMEOUT_SEC: u64 = 60;
/// Socket send/receive timeout (mirrors the C library's 30 second timeout).
const CFRDS_SOCKET_TIMEOUT_SEC: u64 = 30;

/// Sends an HTTP POST to the RDS server and returns the raw response body
/// (including the leading RDS error-code field).
pub fn http_post(server: &Server, command: &str, payload: &[u8]) -> Result<Vec<u8>> {
    let send_buf = build_request(server, command, payload)?;

    let mut sock = http_connect(server)?;

    send_all(server, &mut sock, &send_buf)?;

    let response = receive_response(server, &mut sock)?;

    let good_http1_1 = b"HTTP/1.1 200 ";
    let good_http1_0 = b"HTTP/1.0 200 ";
    let min_len = good_http1_1.len();
    if response.len() < min_len
        || (!response.starts_with(good_http1_1) && !response.starts_with(good_http1_0))
    {
        return Err(server.set_error(Status::ResponseError, "Invalid server response..."));
    }

    let body = skip_http_header(&response).ok_or_else(|| {
        server.set_error(
            Status::HttpResponseNotFound,
            "HTTP response header not found",
        )
    })?;

    let (error_code, rest) = parse_leading_number(body).ok_or_else(|| {
        server.set_error(Status::ResponseError, "cfrds_buffer_parse_number FAILED...")
    })?;

    server.set_error_code(error_code);

    if error_code < 0 {
        let msg = String::from_utf8_lossy(rest).into_owned();
        return Err(server.set_error(Status::ResponseError, msg));
    }

    // The full body (including the leading number, which doubles as both the
    // RDS status/error code and the first field count for the response
    // parsers) is returned to the caller.
    Ok(body.to_vec())
}

/// Builds the raw HTTP request byte payload.
fn build_request(server: &Server, command: &str, payload: &[u8]) -> Result<Vec<u8>> {
    let mut out = Vec::new();
    out.extend_from_slice(b"POST /CFIDE/main/ide.cfm?CFSRV=IDE&ACTION=");
    out.extend_from_slice(command.as_bytes());
    out.extend_from_slice(b" HTTP/1.0\r\nHost: ");
    out.extend_from_slice(server.host().as_bytes());
    if server.port() != 80 {
        out.extend_from_slice(b":");
        out.extend_from_slice(server.port().to_string().as_bytes());
    }
    out.extend_from_slice(
        b"\r\nConnection: close\r\nUser-Agent: Mozilla/3.0 (compatible; Macromedia RDS Client)\r\nAccept: text/html, */*\r\nAccept-Encoding: deflate\r\nContent-type: text/html\r\nContent-length: ",
    );
    out.extend_from_slice(payload.len().to_string().as_bytes());
    out.extend_from_slice(b"\r\n\r\n");
    out.extend_from_slice(payload);
    Ok(out)
}

/// Resolves the server host and establishes a TCP connection.
fn http_connect(server: &Server) -> Result<TcpStream> {
    let port = server.port();
    let addr_iter = (server.host(), port).to_socket_addrs().map_err(|_| {
        server.set_error(Status::SocketHostNotFound, "failed to resolve hostname...")
    })?;

    let mut last_error: Option<std::io::Error> = None;
    for addr in addr_iter {
        match TcpStream::connect(addr) {
            Ok(stream) => {
                let timeout = Duration::from_secs(CFRDS_SOCKET_TIMEOUT_SEC);
                if stream.set_read_timeout(Some(timeout)).is_err()
                    || stream.set_write_timeout(Some(timeout)).is_err()
                {
                    return Err(server.set_error(
                        Status::ConnectionToServerFailed,
                        "failed to set socket timeout",
                    ));
                }
                return Ok(stream);
            }
            Err(e) => last_error = Some(e),
        }
    }

    let _ = last_error;
    Err(server.set_error(
        Status::ConnectionToServerFailed,
        "failed to establish connection to the server...",
    ))
}

/// Writes the entire request to the socket.
fn send_all(server: &Server, sock: &mut TcpStream, data: &[u8]) -> Result<()> {
    let mut written = 0usize;
    while written < data.len() {
        match sock.write(&data[written..]) {
            Ok(n) if n > 0 => written += n,
            Ok(_) => {
                return Err(server.set_error(
                    Status::WritingToSocketFailed,
                    "failed to write to socket...",
                ));
            }
            Err(e) if e.kind() == std::io::ErrorKind::Interrupted => continue,
            Err(_) => {
                return Err(server.set_error(
                    Status::WritingToSocketFailed,
                    "failed to write to socket...",
                ));
            }
        }
    }
    Ok(())
}

/// Reads the full HTTP response from the socket.
fn receive_response(server: &Server, sock: &mut TcpStream) -> Result<Vec<u8>> {
    let start = Instant::now();
    let mut response = Vec::new();
    let mut buf = [0u8; 4096];

    loop {
        if start.elapsed() > Duration::from_secs(CFRDS_MAX_RESPONSE_TIMEOUT_SEC) {
            return Err(server.set_error(
                Status::ReadingFromSocketFailed,
                "response read timed out (overall deadline exceeded)",
            ));
        }

        match sock.read(&mut buf) {
            Ok(0) => break,
            Ok(n) => {
                response.extend_from_slice(&buf[..n]);
                if response.len() > CFRDS_MAX_RESPONSE_SIZE {
                    return Err(server
                        .set_error(Status::ResponseTooLarge, "response exceeded maximum size"));
                }
            }
            Err(_) => {
                return Err(server.set_error(
                    Status::ReadingFromSocketFailed,
                    "failed to read from socket...",
                ));
            }
        }
    }

    Ok(response)
}

/// Skips the HTTP header block, returning the body (or `None` if the header
/// terminator `\r\n\r\n` cannot be found).
fn skip_http_header(data: &[u8]) -> Option<&[u8]> {
    if data.len() < 4 {
        return None;
    }
    data.windows(4)
        .position(|w| w == b"\r\n\r\n")
        .map(|i| &data[i + 4..])
}

/// Parses a leading `N:` field, returning the number and the remaining bytes.
fn parse_leading_number(data: &[u8]) -> Option<(i64, &[u8])> {
    let colon = data.iter().position(|&b| b == b':')?;
    let digits = std::str::from_utf8(&data[..colon]).ok()?;
    if digits.is_empty() {
        return None;
    }
    let val: i64 = digits.parse().ok()?;
    Some((val, &data[colon + 1..]))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn skip_http_header_finds_separator() {
        let data = b"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
        let body = skip_http_header(data).unwrap();
        assert_eq!(body, b"hello");
    }

    #[test]
    fn skip_http_header_missing() {
        assert!(skip_http_header(b"no header here").is_none());
        assert!(skip_http_header(b"ab").is_none());
    }

    #[test]
    fn parse_leading_number_works() {
        let (n, rest) = parse_leading_number(b"42:rest").unwrap();
        assert_eq!(n, 42);
        assert_eq!(rest, b"rest");
    }

    #[test]
    fn parse_leading_number_failure() {
        assert!(parse_leading_number(b"nodigits").is_none());
        assert!(parse_leading_number(b":rest").is_none());
        assert!(parse_leading_number(b"12x:rest").is_none());
    }
}
