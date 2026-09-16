//! Server connection context and RDS command transport.
//!
//! Port of `src/cfrds_server.c` from the C library.

use std::cell::RefCell;

use crate::buffer::{encode_password, Buffer};
use crate::error::{Error, Result, Status};
use crate::http;

/// An RDS command argument that is either a string or raw bytes.
#[derive(Debug, Clone, Copy)]
pub(crate) enum RdsArg<'a> {
    Str(&'a str),
    Bytes(&'a [u8]),
}

/// Connection context for a ColdFusion RDS server.
///
/// The library is not thread-safe; a `Server` instance must not be shared
/// across threads without external synchronisation.
#[derive(Debug)]
pub struct Server {
    host: String,
    port: u16,
    username: String,
    orig_password: String,
    /// XOR-obfuscated password sent on the wire.
    password: Option<String>,
    state: RefCell<ServerState>,
}

#[derive(Debug, Default)]
struct ServerState {
    errno: i32,
    error_code: i64,
    error: Option<String>,
}

impl Server {
    /// Creates a new server connection context.
    ///
    /// Mirrors `cfrds_server_init`: `host` must be non-empty, `port` non-zero.
    /// The plaintext password is stored for credential access, and the XOR
    /// obfuscated variant is prepared for transmission.
    pub fn new(host: &str, port: u16, username: &str, password: &str) -> Result<Server> {
        if host.is_empty() || port == 0 {
            return Err(Error::new(
                Status::InvalidInputParameter,
                "invalid host or port",
            ));
        }

        let encoded = if password.is_empty() {
            None
        } else {
            Some(encode_password(password))
        };

        Ok(Server {
            host: host.to_string(),
            port,
            username: username.to_string(),
            orig_password: password.to_string(),
            password: encoded,
            state: RefCell::new(ServerState {
                errno: 0,
                error_code: 1,
                error: None,
            }),
        })
    }

    /// Returns the server host.
    pub fn host(&self) -> &str {
        &self.host
    }

    /// Returns the server port.
    pub fn port(&self) -> u16 {
        self.port
    }

    /// Returns the configured username.
    pub fn username(&self) -> &str {
        &self.username
    }

    /// Returns the plaintext password (as stored).
    pub fn password(&self) -> &str {
        &self.orig_password
    }

    /// Returns the last recorded error message, if any.
    pub fn error(&self) -> Option<String> {
        self.state.borrow().error.clone()
    }

    /// Returns the last recorded numeric error code.
    pub fn error_code(&self) -> i64 {
        self.state.borrow().error_code
    }

    /// Returns the last recorded OS/errno value, if any.
    pub fn errno(&self) -> i32 {
        self.state.borrow().errno
    }

    /// Clears the internal error state. Mirrors `cfrds_server_clear_error`,
    /// which resets `error_code` to `1`.
    pub fn clear_error(&self) {
        let mut state = self.state.borrow_mut();
        state.errno = 0;
        state.error_code = 1;
        state.error = None;
    }

    /// Sets the numeric error code without a message (used by protocol parsers).
    pub(crate) fn set_error_code(&self, code: i64) {
        self.state.borrow_mut().error_code = code;
    }

    /// Sets the raw error state from an arbitrary numeric code and message.
    pub(crate) fn set_error_state(&self, code: i64, msg: Option<&str>) {
        let mut state = self.state.borrow_mut();
        state.error_code = code;
        state.error = msg.map(|s| s.to_string());
    }

    /// Sets an error message with a numeric code, returning a corresponding
    /// library `Error`. Mirrors `cfrds_server_set_error`.
    pub(crate) fn set_error(&self, status: Status, msg: impl Into<String>) -> Error {
        let msg = msg.into();
        let mut state = self.state.borrow_mut();
        state.error_code = status as i64;
        state.error = Some(msg.clone());
        Error::new(status, msg)
    }

    /// Sends an RDS command with the given argument list and returns the raw
    /// response body. Mirrors `cfrds_send_command`.
    pub(crate) fn send_command(&self, command: &str, list: &[&str]) -> Result<Vec<u8>> {
        self.send_command_args(
            command,
            &list.iter().copied().map(RdsArg::Str).collect::<Vec<_>>(),
        )
    }

    /// Sends an RDS command whose arguments may mix strings and raw bytes.
    pub(crate) fn send_command_args(&self, command: &str, list: &[RdsArg]) -> Result<Vec<u8>> {
        let mut total_cnt = list.len();
        if !self.username.is_empty() {
            total_cnt += 1;
        }
        if let Some(pw) = &self.password {
            if !pw.is_empty() {
                total_cnt += 1;
            }
        }

        self.clear_error();

        let mut post = Buffer::new();
        post.append_rds_count(total_cnt);

        for item in list {
            match item {
                RdsArg::Str(s) => post.append_rds_string(s),
                RdsArg::Bytes(b) => post.append_rds_bytes(b),
            };
        }

        if !self.username.is_empty() {
            post.append_rds_string(&self.username);
        }
        if let Some(pw) = &self.password {
            if !pw.is_empty() {
                post.append_rds_string(pw);
            }
        }

        http::http_post(self, command, post.data())
    }
}
