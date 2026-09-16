//! File and directory commands (port of `src/cfrds_file.c`).

use crate::error::{Result, Status};
use crate::parser;
use crate::server::{RdsArg, Server};
use crate::types::{BrowseDir, FileContent};

/// Prefix of the server error message when a file does not exist.
const FILE_NOT_FOUND_PREFIX: &str = "The system cannot find the path specified: ";

impl Server {
    /// Lists files and folders in a remote directory.
    pub fn browse_dir(&self, path: &str) -> Result<BrowseDir> {
        if path.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let response = self.send_command("BROWSEDIR", &[path, ""])?;
        parser::buffer_to_browse_dir(&response).ok_or_else(|| {
            self.set_error(Status::ResponseError, "failed to parse BROWSEDIR response")
        })
    }

    /// Reads the contents of a remote file.
    pub fn file_read(&self, pathname: &str) -> Result<FileContent> {
        if pathname.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let response = self.send_command("FILEIO", &[pathname, "READ", ""])?;
        parser::buffer_to_file_content(&response).ok_or_else(|| {
            self.set_error(
                Status::ResponseError,
                "failed to parse FILEIO READ response",
            )
        })
    }

    /// Writes raw bytes to a remote file path.
    pub fn file_write(&self, pathname: &str, data: &[u8]) -> Result<()> {
        if pathname.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        self.send_command_args(
            "FILEIO",
            &[
                RdsArg::Str(pathname),
                RdsArg::Str("WRITE"),
                RdsArg::Str(""),
                RdsArg::Bytes(data),
            ],
        )?;
        Ok(())
    }

    /// Renames or moves a remote file or folder.
    pub fn file_rename(&self, current_pathname: &str, new_pathname: &str) -> Result<()> {
        if current_pathname.is_empty() || new_pathname.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        self.send_command("FILEIO", &[current_pathname, "RENAME", "", new_pathname])?;
        Ok(())
    }

    /// Deletes a remote file.
    pub fn file_remove_file(&self, pathname: &str) -> Result<()> {
        if pathname.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        self.send_command("FILEIO", &[pathname, "REMOVE", "", "F"])?;
        Ok(())
    }

    /// Deletes a remote directory.
    pub fn file_remove_dir(&self, path: &str) -> Result<()> {
        if path.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        self.send_command("FILEIO", &[path, "REMOVE", "", "D"])?;
        Ok(())
    }

    /// Checks whether a file or directory exists on the remote server.
    ///
    /// The server reports a "path not found" error for missing entries, which
    /// is treated as `Ok(false)`.
    pub fn file_exists(&self, pathname: &str) -> Result<bool> {
        if pathname.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        match self.send_command("FILEIO", &[pathname, "EXISTENCE", "", ""]) {
            Ok(_) => Ok(true),
            Err(e) => {
                if e.message().starts_with(FILE_NOT_FOUND_PREFIX) {
                    self.clear_error();
                    self.set_error_code(1);
                    Ok(false)
                } else {
                    Err(e)
                }
            }
        }
    }

    /// Creates a directory on the remote server.
    pub fn file_create_dir(&self, path: &str) -> Result<()> {
        if path.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        self.send_command("FILEIO", &[path, "CREATE", "", ""])?;
        Ok(())
    }

    /// Retrieves the root directory path of the ColdFusion installation.
    pub fn file_get_root_dir(&self) -> Result<String> {
        let response = self.send_command("FILEIO", &["", "CF_DIRECTORY"])?;
        let mut p = crate::buffer::Parser::new(&response);
        p.parse_number().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse CF_DIRECTORY response",
            )
        })?;
        let out = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse CF_DIRECTORY response",
            )
        })?;
        Ok(String::from_utf8_lossy(&out).into_owned())
    }
}
