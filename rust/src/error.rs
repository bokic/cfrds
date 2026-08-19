//! Error and status types for the cfrds library.
//!
//! Port of the `cfrds_status` enum and error handling from `include/cfrds.h`.

use std::fmt;

/// Status codes returned by RDS command functions, mirroring `cfrds_status`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(i32)]
pub enum Status {
    Ok = 0,
    MemoryError = 1,
    ParamIsNull = 2,
    ServerIsNull = 3,
    InvalidInputParameter = 4,
    IndexOutOfBounds = 5,
    CommandFailed = 6,
    ResponseError = 7,
    HttpResponseNotFound = 8,
    DirAlreadyExists = 9,
    SocketHostNotFound = 10,
    SocketCreationFailed = 11,
    ConnectionToServerFailed = 12,
    WritingToSocketFailed = 13,
    PartiallyWriteToSocket = 14,
    ReadingFromSocketFailed = 15,
    ResponseTooLarge = 16,
}

impl Status {
    /// Returns `true` if this status represents success.
    pub fn is_ok(&self) -> bool {
        *self == Status::Ok
    }

    /// Returns a short human-readable description of the status.
    pub fn as_str(&self) -> &'static str {
        match self {
            Status::Ok => "OK",
            Status::MemoryError => "memory error",
            Status::ParamIsNull => "parameter is NULL",
            Status::ServerIsNull => "server is NULL",
            Status::InvalidInputParameter => "invalid input parameter",
            Status::IndexOutOfBounds => "index out of bounds",
            Status::CommandFailed => "command failed",
            Status::ResponseError => "response error",
            Status::HttpResponseNotFound => "HTTP response not found",
            Status::DirAlreadyExists => "directory already exists",
            Status::SocketHostNotFound => "socket host not found",
            Status::SocketCreationFailed => "socket creation failed",
            Status::ConnectionToServerFailed => "connection to server failed",
            Status::WritingToSocketFailed => "writing to socket failed",
            Status::PartiallyWriteToSocket => "partially written to socket",
            Status::ReadingFromSocketFailed => "reading from socket failed",
            Status::ResponseTooLarge => "response too large",
        }
    }
}

impl fmt::Display for Status {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_str())
    }
}

/// An error returned by the library, carrying an RDS status code and a message.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    status: Status,
    message: String,
}

impl Error {
    /// Creates a new error from a status code and message.
    pub fn new(status: Status, message: impl Into<String>) -> Error {
        Error {
            status,
            message: message.into(),
        }
    }

    /// Creates a generic response-error status.
    pub fn response(message: impl Into<String>) -> Error {
        Error::new(Status::ResponseError, message)
    }

    /// Returns the RDS status code.
    pub fn status(&self) -> Status {
        self.status
    }

    /// Returns the human-readable message.
    pub fn message(&self) -> &str {
        &self.message
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.message.is_empty() {
            write!(f, "{}", self.status)
        } else {
            write!(f, "{}: {}", self.status, self.message)
        }
    }
}

impl std::error::Error for Error {}

impl From<Status> for Error {
    fn from(status: Status) -> Error {
        Error {
            status,
            message: String::new(),
        }
    }
}

/// Alias for `Result<T, Error>` used throughout the library.
pub type Result<T> = std::result::Result<T, Error>;
