//! Security analyzer commands (port of `src/cfrds_security_analyzer.c`).

use crate::error::{Result, Status};
use crate::server::Server;
use crate::types::{SecurityAnalyzerResult, SecurityAnalyzerStatus};

impl Server {
    /// Submits a path list for a remote vulnerability/security scan.
    pub fn security_analyzer_scan(
        &self,
        pathnames: &str,
        recursively: bool,
        cores: i32,
    ) -> Result<i32> {
        let recursively_str = if recursively { "true" } else { "false" };
        let response = self.send_command(
            "SECURITYANALYZER",
            &["scan", pathnames, recursively_str, &cores.to_string()],
        )?;
        let json = parse_sa_json_response(self, &response)?;
        let id = json
            .get("id")
            .and_then(serde_json::Value::as_i64)
            .ok_or_else(|| self.set_error(Status::ResponseError, "invalid or missing id"))?;
        Ok(id as i32)
    }

    /// Cancels an active security scan task.
    pub fn security_analyzer_cancel(&self, command_id: i32) -> Result<()> {
        let response =
            self.send_command("SECURITYANALYZER", &["cancel", &command_id.to_string()])?;
        parse_sa_json_response(self, &response)?;
        Ok(())
    }

    /// Queries progress and metadata state of an active scan task.
    pub fn security_analyzer_status(&self, command_id: i32) -> Result<SecurityAnalyzerStatus> {
        let response =
            self.send_command("SECURITYANALYZER", &["status", &command_id.to_string()])?;
        let json = parse_sa_json_response(self, &response)?;

        let get_int = |key: &str| -> Result<i64> {
            json.get(key)
                .and_then(serde_json::Value::as_i64)
                .ok_or_else(|| self.set_error(Status::ResponseError, format!("invalid {key}")))
        };

        Ok(SecurityAnalyzerStatus {
            totalfiles: get_int("totalfiles")?,
            filesvisitedcount: get_int("filesvisitedcount")?,
            percentage: get_int("percentage")?,
            lastupdated: get_int("lastupdated")?,
        })
    }

    /// Retrieves the scan results report for a command.
    pub fn security_analyzer_result(&self, command_id: i32) -> Result<SecurityAnalyzerResult> {
        let response =
            self.send_command("SECURITYANALYZER", &["result", &command_id.to_string()])?;

        let mut p = crate::buffer::Parser::new(&response);
        let items = p.parse_number().map_err(|_| {
            self.set_error(Status::ResponseError, "failed to parse result response")
        })?;
        if items != 1 {
            return Err(self.set_error(Status::ResponseError, "invalid result response"));
        }
        let json_str = p.parse_bytes().map_err(|_| {
            self.set_error(Status::ResponseError, "failed to parse result response")
        })?;
        let json_str = String::from_utf8_lossy(&json_str).into_owned();

        let value: serde_json::Value = serde_json::from_str(&json_str)
            .map_err(|_| self.set_error(Status::ResponseError, "invalid JSON in result"))?;
        Ok(SecurityAnalyzerResult::from(value))
    }

    /// Cleans/destroys scan task results state stored on the server.
    pub fn security_analyzer_clean(&self, command_id: i32) -> Result<()> {
        let response =
            self.send_command("SECURITYANALYZER", &["clean", &command_id.to_string()])?;

        let mut p = crate::buffer::Parser::new(&response);
        let rows = p
            .parse_number()
            .map_err(|_| self.set_error(Status::ResponseError, "failed to parse clean response"))?;
        if rows != 1 {
            return Err(self.set_error(Status::ResponseError, "rows != 1"));
        }
        p.parse_bytes()
            .map_err(|_| self.set_error(Status::ResponseError, "invalid clean response"))?;
        Ok(())
    }
}

impl SecurityAnalyzerResult {
    fn field(&self, key: &str) -> Option<&serde_json::Value> {
        self.json.get(key)
    }

    fn array_len(&self, key: &str) -> usize {
        self.field(key)
            .and_then(serde_json::Value::as_array)
            .map_or(0, |v| v.len())
    }

    fn int(&self, key: &str) -> Option<i64> {
        self.field(key).and_then(serde_json::Value::as_i64)
    }

    fn str(&self, key: &str) -> Option<&str> {
        self.field(key).and_then(serde_json::Value::as_str)
    }

    fn array_item_str(&self, array_key: &str, ndx: usize, field_key: &str) -> Option<&str> {
        let arr = self.field(array_key)?.as_array()?;
        arr.get(ndx)?.get(field_key)?.as_str()
    }

    fn array_item_int(&self, array_key: &str, ndx: usize, field_key: &str) -> Option<i64> {
        let arr = self.field(array_key)?.as_array()?;
        arr.get(ndx)?.get(field_key)?.as_i64()
    }

    /// Returns the total files count.
    pub fn totalfiles(&self) -> Option<i64> {
        self.int("totalfiles")
    }

    /// Returns the completed files count.
    pub fn filesvisitedcount(&self) -> Option<i64> {
        self.int("filesvisitedcount")
    }

    /// Returns the count of error descriptions.
    pub fn errorsdescription_count(&self) -> usize {
        self.array_len("errorsdescription")
    }

    /// Returns the count of successfully scanned files.
    pub fn filesscanned_count(&self) -> usize {
        self.array_len("filesscanned")
    }

    /// Returns the result category of a scanned file.
    pub fn filesscanned_item_result(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("filesscanned", ndx, "result")
    }

    /// Returns the file path of a scanned file.
    pub fn filesscanned_item_filename(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("filesscanned", ndx, "filename")
    }

    /// Returns the count of skipped files.
    pub fn filesnotscanned_count(&self) -> usize {
        self.array_len("filesnotscanned")
    }

    /// Returns the skip reason of a skipped file.
    pub fn filesnotscanned_item_reason(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("filesnotscanned", ndx, "reason")
    }

    /// Returns the path of a skipped file.
    pub fn filesnotscanned_item_filename(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("filesnotscanned", ndx, "filename")
    }

    /// Returns the executor service backend description.
    pub fn executorservice(&self) -> Option<&str> {
        self.str("executorservice")
    }

    /// Returns the scan progress percentage.
    pub fn percentage(&self) -> Option<i64> {
        self.int("percentage")
    }

    /// Returns the count of the overall files list.
    pub fn files_count(&self) -> usize {
        self.array_len("files")
    }

    /// Returns the path name at `ndx` from the overall files list.
    pub fn files_value(&self, ndx: usize) -> Option<&str> {
        let arr = self.field("files")?.as_array()?;
        arr.get(ndx)?.as_str()
    }

    /// Returns the last updated timestamp.
    pub fn lastupdated(&self) -> Option<i64> {
        self.int("lastupdated")
    }

    /// Returns the visited files count.
    pub fn filesvisited_count(&self) -> usize {
        self.array_len("filesvisited")
    }

    /// Returns the count of skipped files.
    pub fn filesnotscannedcount(&self) -> Option<i64> {
        self.int("filesnotscannedcount")
    }

    /// Returns the scanned files count.
    pub fn filesscannedcount(&self) -> Option<i64> {
        self.int("filesscannedcount")
    }

    /// Returns the scan tracking ID.
    pub fn id(&self) -> Option<i64> {
        self.int("id")
    }

    /// Returns the vulnerability error instance count.
    pub fn errors_count(&self) -> usize {
        self.array_len("errors")
    }

    /// Returns the error message of a vulnerability.
    pub fn errors_item_errormessage(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "errormessage")
    }

    /// Returns the ending line of a vulnerability range.
    pub fn errors_item_endline(&self, ndx: usize) -> Option<i64> {
        self.array_item_int("errors", ndx, "endline")
    }

    /// Returns the directory path of a vulnerable code item.
    pub fn errors_item_path(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "path")
    }

    /// Returns the code snippet of a vulnerable item.
    pub fn errors_item_vulnerablecode(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "vulnerablecode")
    }

    /// Returns the file path of a vulnerability.
    pub fn errors_item_filename(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "filename")
    }

    /// Returns the starting line of a vulnerability range.
    pub fn errors_item_beginline(&self, ndx: usize) -> Option<i64> {
        self.array_item_int("errors", ndx, "beginline")
    }

    /// Returns the column offset of a vulnerability.
    pub fn errors_item_column(&self, ndx: usize) -> Option<i64> {
        self.array_item_int("errors", ndx, "column")
    }

    /// Returns the detailed error description of a vulnerability.
    pub fn errors_item_error(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "Error")
    }

    /// Returns the starting column of a vulnerability range.
    pub fn errors_item_begincolumn(&self, ndx: usize) -> Option<i64> {
        self.array_item_int("errors", ndx, "begincolumn")
    }

    /// Returns the type category of a vulnerability.
    pub fn errors_item_type(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "type")
    }

    /// Returns the ending column of a vulnerability range.
    pub fn errors_item_endcolumn(&self, ndx: usize) -> Option<i64> {
        self.array_item_int("errors", ndx, "endcolumn")
    }

    /// Returns the reference type of a vulnerability.
    pub fn errors_item_referencetype(&self, ndx: usize) -> Option<&str> {
        self.array_item_str("errors", ndx, "referencetype")
    }

    /// Returns the job status string (empty when missing).
    pub fn status(&self) -> &str {
        self.str("status").unwrap_or("")
    }
}

/// Parses a security analyzer JSON response, verifying `status == "success"`.
fn parse_sa_json_response(server: &Server, response: &[u8]) -> Result<serde_json::Value> {
    let mut p = crate::buffer::Parser::new(response);
    let rows = p
        .parse_number()
        .map_err(|_| server.set_error(Status::ResponseError, "rows != 1"))?;
    if rows != 1 {
        return Err(server.set_error(Status::ResponseError, "rows != 1"));
    }

    let json_bytes = p
        .parse_bytes()
        .map_err(|_| server.set_error(Status::ResponseError, "invalid json string in response"))?;
    if p.remaining() != 0 {
        return Err(server.set_error(Status::ResponseError, "invalid json string in response"));
    }
    let json_str = String::from_utf8_lossy(&json_bytes).into_owned();

    let value: serde_json::Value = serde_json::from_str(&json_str)
        .map_err(|_| server.set_error(Status::ResponseError, "json parse failed"))?;

    let status = value
        .get("status")
        .and_then(serde_json::Value::as_str)
        .ok_or_else(|| server.set_error(Status::ResponseError, "invalid status"))?;

    if status != "success" {
        let msg = value
            .get("errormessage")
            .and_then(serde_json::Value::as_str)
            .unwrap_or("status != success");
        return Err(server.set_error(Status::ResponseError, msg));
    }

    Ok(value)
}
