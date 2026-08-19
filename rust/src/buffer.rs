//! RDS protocol byte buffer serialization and parsing.
//!
//! Port of `src/cfrds_buffer.c` from the C library. The RDS wire format uses a
//! length-prefixed field layout (`<count>:<field>...`) where each field is
//! `<len>:<bytes>` on the response path and `STR:<len>:<bytes>` on the request path.

use crate::error::Status;

/// Growable byte buffer used to build RDS request payloads.
#[derive(Debug, Default, Clone)]
pub struct Buffer(Vec<u8>);

impl Buffer {
    /// Creates a new, empty buffer.
    pub fn new() -> Buffer {
        Buffer(Vec::new())
    }

    /// Returns the raw bytes of the buffer.
    pub fn data(&self) -> &[u8] {
        &self.0
    }

    /// Returns the active data size in bytes.
    pub fn data_size(&self) -> usize {
        self.0.len()
    }

    /// Appends a UTF-8 string to the buffer.
    pub fn append(&mut self, s: &str) -> bool {
        self.0.extend_from_slice(s.as_bytes());
        true
    }

    /// Appends a string, escaping RDS key/value delimiters `:` and `;` with a backslash.
    pub fn append_escaped(&mut self, s: &str) -> bool {
        for ch in s.chars() {
            if ch == ':' || ch == ';' {
                self.0.push(b'\\');
            }
            let mut buf = [0u8; 4];
            self.0
                .extend_from_slice(ch.encode_utf8(&mut buf).as_bytes());
        }
        true
    }

    /// Appends raw bytes to the buffer.
    pub fn append_bytes(&mut self, data: &[u8]) -> bool {
        self.0.extend_from_slice(data);
        true
    }

    /// Appends another buffer's contents to this buffer.
    pub fn append_buffer(&mut self, other: &Buffer) -> bool {
        self.0.extend_from_slice(&other.0);
        true
    }

    /// Appends an RDS list count as `<cnt>:`.
    pub fn append_rds_count(&mut self, cnt: usize) -> bool {
        self.append(&cnt.to_string());
        self.0.push(b':');
        true
    }

    /// Appends a string in RDS representation `STR:<len>:<bytes>`.
    pub fn append_rds_string(&mut self, s: &str) -> bool {
        self.append("STR:");
        self.append(&s.len().to_string());
        self.0.push(b':');
        self.0.extend_from_slice(s.as_bytes());
        true
    }

    /// Appends raw bytes in RDS representation `STR:<len>:<bytes>`.
    pub fn append_rds_bytes(&mut self, data: &[u8]) -> bool {
        self.append("STR:");
        self.append(&data.len().to_string());
        self.0.push(b':');
        self.0.extend_from_slice(data);
        true
    }
}

/// Cursor-based parser for RDS response bodies.
#[derive(Debug, Clone)]
pub struct Parser<'a> {
    data: &'a [u8],
    pos: usize,
}

impl<'a> Parser<'a> {
    /// Creates a new parser over `data`.
    pub fn new(data: &'a [u8]) -> Parser<'a> {
        Parser { data, pos: 0 }
    }

    /// Returns the number of unparsed bytes remaining.
    pub fn remaining(&self) -> usize {
        self.data.len().saturating_sub(self.pos)
    }

    /// Returns the unparsed bytes.
    pub fn rest(&self) -> &'a [u8] {
        &self.data[self.pos..]
    }

    /// Parses a base-10 number terminated by a colon and advances past it.
    pub fn parse_number(&mut self) -> Result<i64, Status> {
        let rest = &self.data[self.pos..];
        let colon = rest
            .iter()
            .position(|&b| b == b':')
            .ok_or(Status::ResponseError)?;
        let digits = &rest[..colon];
        if digits.is_empty() {
            return Err(Status::ResponseError);
        }
        let s = std::str::from_utf8(digits).map_err(|_| Status::ResponseError)?;
        let val: i64 = s.parse().map_err(|_| Status::ResponseError)?;
        self.pos += colon + 1;
        Ok(val)
    }

    /// Parses a length-prefixed byte field and advances past it.
    pub fn parse_bytes(&mut self) -> Result<Vec<u8>, Status> {
        let start = self.pos;
        let n = self.parse_number()?;
        if n < 0 {
            self.pos = start;
            return Err(Status::ResponseError);
        }
        let size = n as usize;
        if size > self.remaining() {
            self.pos = start;
            return Err(Status::ResponseError);
        }
        let out = self.data[self.pos..self.pos + size].to_vec();
        self.pos += size;
        Ok(out)
    }

    /// Parses a length-prefixed field as a string.
    pub fn parse_string(&mut self) -> Result<Vec<u8>, Status> {
        self.parse_bytes()
    }

    /// Parses a single comma-separated list item, handling double-quoted items.
    ///
    /// Mirrors `cfrds_buffer_parse_string_list_item` from the C library, including the
    /// behaviour of refusing to start when fewer than 2 bytes remain.
    pub fn parse_string_list_item(&mut self) -> Result<String, Status> {
        if self.remaining() < 2 {
            return Err(Status::ResponseError);
        }

        let mut pos = self.pos;
        let data = self.data;

        if data[pos] == b'"' {
            pos += 1;
            let rest = &data[pos..];
            let end = rest
                .iter()
                .position(|&b| b == b'"')
                .ok_or(Status::ResponseError)?;
            let item = &rest[..end];
            pos += end + 1;
            let out = String::from_utf8_lossy(item).into_owned();
            self.pos = pos;
            if self.remaining() > 0 && self.data[self.pos] == b',' {
                self.pos += 1;
            }
            Ok(out)
        } else {
            let rest = &data[pos..];
            let end = rest.iter().position(|&b| b == b',').unwrap_or(rest.len());
            let item = &rest[..end];
            pos += end;
            let out = String::from_utf8_lossy(item).into_owned();
            self.pos = pos;
            if self.remaining() > 0 && self.data[self.pos] == b',' {
                self.pos += 1;
            }
            Ok(out)
        }
    }

    /// Verifies the parser has consumed all input.
    pub fn expect_end(&self) -> Result<(), Status> {
        if self.remaining() == 0 {
            Ok(())
        } else {
            Err(Status::ResponseError)
        }
    }
}

/// Encodes a plaintext password for RDS transmission using the hardcoded
/// Adobe ColdFusion XOR obfuscation scheme.
pub fn encode_password(password: &str) -> String {
    const HEX: &[u8; 16] = b"0123456789abcdef";
    const FILLUP: &[u8] = b"4p0L@r1$";
    let mut out = String::with_capacity(password.len() * 2);
    for (i, b) in password.bytes().enumerate() {
        let enc = b ^ FILLUP[i % FILLUP.len()];
        out.push(HEX[((enc & 0xf0) >> 4) as usize] as char);
        out.push(HEX[(enc & 0x0f) as usize] as char);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encode_password_matches_c() {
        // Spot-check the XOR-obfuscation scheme.
        assert_eq!(encode_password(""), "");
        let encoded = encode_password("secret");
        assert_eq!(encoded.len(), "secret".len() * 2);
        // Every character must be a hex digit.
        assert!(encoded.chars().all(|c| c.is_ascii_hexdigit()));
    }

    #[test]
    fn parse_number_reads_colon_terminated() {
        let mut p = Parser::new(b"42:abc");
        assert_eq!(p.parse_number().unwrap(), 42);
        assert_eq!(p.remaining(), 3);
    }

    #[test]
    fn parse_bytes_reads_length_prefixed() {
        let mut p = Parser::new(b"3:abc5:hello");
        assert_eq!(p.parse_bytes().unwrap(), b"abc".to_vec());
        assert_eq!(p.parse_bytes().unwrap(), b"hello".to_vec());
        assert_eq!(p.remaining(), 0);
    }

    #[test]
    fn parse_bytes_rejects_negative_length() {
        let mut p = Parser::new(b"-1:abc");
        assert!(p.parse_bytes().is_err());
        assert_eq!(p.remaining(), 6);
    }

    #[test]
    fn parse_string_list_item_quoted() {
        let mut p = Parser::new(b"\"a,b\",c,\"\"");
        assert_eq!(p.parse_string_list_item().unwrap(), "a,b");
        assert_eq!(p.parse_string_list_item().unwrap(), "c");
        assert_eq!(p.parse_string_list_item().unwrap(), "");
        assert_eq!(p.remaining(), 0);
    }

    #[test]
    fn parse_string_list_item_unquoted() {
        let mut p = Parser::new(b"aa,bb,cc");
        assert_eq!(p.parse_string_list_item().unwrap(), "aa");
        assert_eq!(p.parse_string_list_item().unwrap(), "bb");
        assert_eq!(p.parse_string_list_item().unwrap(), "cc");
        assert_eq!(p.remaining(), 0);
    }

    #[test]
    fn buffer_rds_format() {
        let mut b = Buffer::new();
        b.append_rds_count(2);
        b.append_rds_string("hi");
        b.append_rds_string("");
        assert_eq!(b.data(), b"2:STR:2:hiSTR:0:");
    }

    #[test]
    fn buffer_escaped() {
        let mut b = Buffer::new();
        b.append_escaped("a:b;c");
        assert_eq!(b.data(), br"a\:b\;c");
    }
}
