//! # cfrds — ColdFusion RDS protocol library (Rust port)
//!
//! A Rust port of the C11 `libcfrds` library. Communicates with a ColdFusion
//! server via the Adobe RDS protocol over plain HTTP (TLS is deliberately not
//! supported, matching the C library).
//!
//! ## Example
//!
//! ```no_run
//! use cfrds::Server;
//!
//! let server = Server::new("127.0.0.1", 8500, "admin", "secret")?;
//! let root = server.file_get_root_dir()?;
//! let dir = server.browse_dir("/")?;
//! for item in dir.iter() {
//!     println!("{} {}", item.kind, item.name);
//! }
//! # Ok::<(), cfrds::Error>(())
//! ```
//!
//! ## Thread safety
//!
//! Like the C library, this port is **not thread-safe**. A [`Server`] instance
//! must not be shared or used concurrently from multiple threads. Each thread
//! must create its own [`Server`] instance.

pub mod buffer;
pub mod commands;
pub mod error;
pub mod ffi;
pub mod http;
pub mod parser;
pub mod server;
pub mod types;
pub mod wddx;

pub use error::{Error, Result, Status};
pub use server::Server;
pub use types::{
    AdminApiCustomTagPaths, AdminApiMappings, BrowseDir, BrowseDirItem, ColumnInfo, ColumnInfoItem,
    DebuggerEvent, DebuggerEventType, DsnInfo, FileContent, IdeDefaultResult, KeyInfo, KeyInfoItem,
    PrimaryKeyItem, PrimaryKeys, ResultSet, SecurityAnalyzerResult, SecurityAnalyzerStatus,
    SqlMetadata, SqlMetadataItem, SupportedCommands, TableInfo, TableInfoItem,
};
pub use wddx::{Wddx, WddxNode, WddxType};

/// The library version string.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");
