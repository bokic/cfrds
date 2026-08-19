//! Admin API, IDE handshake and graphing commands (port of `src/cfrds.c`).

use crate::buffer::Parser;
use crate::error::{Result, Status};
use crate::server::Server;
use crate::types::{AdminApiCustomTagPaths, AdminApiMappings, IdeDefaultResult};
use crate::wddx::{Wddx, WddxType};

impl Server {
    /// Performs the internal IDE setup check handshake.
    pub fn ide_default(&self, version: i32) -> Result<IdeDefaultResult> {
        // Protocol quirk: IDE_DEFAULT expects the version argument formatted
        // with a trailing comma (e.g. "N,").
        let param = format!("{},", version);
        let response = self.send_command("IDE_DEFAULT", &["", &param])?;

        let mut p = Parser::new(&response);
        let count = p.parse_number().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        if count != 5 {
            return Err(self.set_error(Status::ResponseError, "invalid IDE_DEFAULT response"));
        }

        let num1 = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        let server_version = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        let client_version = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        let num2 = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        let num3 = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse IDE_DEFAULT response",
            )
        })?;
        p.expect_end()
            .map_err(|_| self.set_error(Status::ResponseError, "invalid IDE_DEFAULT response"))?;

        Ok(IdeDefaultResult {
            num1: parse_atoi(&num1),
            server_version: String::from_utf8_lossy(&server_version).into_owned(),
            client_version: String::from_utf8_lossy(&client_version).into_owned(),
            num2: parse_atoi(&num2),
            num3: parse_atoi(&num3),
        })
    }

    /// Queries a system debugging log path directory.
    pub fn adminapi_debugging_getlogproperty(&self, logdirectory: &str) -> Result<String> {
        let response = self.send_command(
            "ADMINAPI",
            &["cfide.adminapi.debugging", "getlogproperty", logdirectory],
        )?;

        let mut p = Parser::new(&response);
        p.parse_number().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse getlogproperty response",
            )
        })?;
        let xml = p.parse_bytes().map_err(|_| {
            self.set_error(
                Status::ResponseError,
                "failed to parse getlogproperty response",
            )
        })?;
        let xml = String::from_utf8_lossy(&xml).into_owned();

        if xml.is_empty() {
            return Ok(String::new());
        }

        let wddx = Wddx::from_xml(&xml).ok_or_else(|| {
            self.set_error(
                Status::ResponseError,
                "invalid WDDX in getlogproperty response",
            )
        })?;
        let data = wddx
            .data()
            .ok_or_else(|| self.set_error(Status::ResponseError, "missing getlogproperty data"))?;
        if data.node_type() != WddxType::String {
            return Err(
                self.set_error(Status::ResponseError, "wddx_node_type(data) != WDDX_STRING")
            );
        }
        Ok(data.node_string().unwrap_or("").to_string())
    }

    /// Queries the custom tag path mappings on the server.
    pub fn adminapi_extensions_getcustomtagpaths(&self) -> Result<AdminApiCustomTagPaths> {
        let response = self.send_command(
            "ADMINAPI",
            &["cfide.adminapi.extensions", "getcustomtagpaths"],
        )?;
        let wddx = parse_wddx_response(self, &response, "getcustomtagpaths")?;
        Ok(AdminApiCustomTagPaths::from(wddx))
    }

    /// Adds or updates a logical mapping configuration.
    pub fn adminapi_extensions_setmapping(&self, name: &str, path: &str) -> Result<()> {
        let arg = format!("name:{};path:{}", escape_arg(name), escape_arg(path));
        let response = self.send_command(
            "ADMINAPI",
            &["cfide.adminapi.extensions", "setmappings", &arg],
        )?;
        parse_adminapi_simple_response(self, &response, "setmappings")
    }

    /// Deletes a logical mapping configuration.
    pub fn adminapi_extensions_deletemapping(&self, mapping: &str) -> Result<()> {
        // NOTE: "deleltemappings" (with the extra 'l') is a required typo
        // hardcoded in the Adobe ColdFusion RDS backend.
        let response = self.send_command(
            "ADMINAPI",
            &["cfide.adminapi.extensions", "deleltemappings", mapping],
        )?;
        parse_adminapi_simple_response(self, &response, "deleltemappings")
    }

    /// Queries all logical mapping configurations on the server.
    pub fn adminapi_extensions_getmappings(&self) -> Result<AdminApiMappings> {
        let response =
            self.send_command("ADMINAPI", &["cfide.adminapi.extensions", "getmappings"])?;
        let wddx = parse_wddx_response(self, &response, "getmappings")?;
        Ok(AdminApiMappings::from(wddx))
    }

    /// Handshakes a charting/rendering query, returning the binary result.
    pub fn graphing(&self, chart_attributes: &str, series_data: &[&str]) -> Result<Vec<u8>> {
        if chart_attributes.is_empty() {
            return Err(Status::ParamIsNull.into());
        }

        let num_series = series_data.len().to_string();
        let mut args: Vec<&str> = Vec::with_capacity(3 + series_data.len());
        args.push("GRAPH");
        args.push(chart_attributes);
        args.push(&num_series);
        args.extend_from_slice(series_data);

        let response = self.send_command("GRAPHING", &args)?;
        Ok(response)
    }
}

/// Escapes `:` and `;` in an admin API argument value.
fn escape_arg(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        if ch == ':' || ch == ';' {
            out.push('\\');
        }
        out.push(ch);
    }
    out
}

/// Parses a response that is either empty or a single XML field whose content,
/// when non-empty, signals an error.
fn parse_adminapi_simple_response(server: &Server, response: &[u8], _what: &str) -> Result<()> {
    let mut p = Parser::new(response);
    p.parse_number().map_err(|_| {
        server.set_error(Status::ResponseError, "failed to parse adminapi response")
    })?;

    if p.remaining() > 0 {
        let xml = p.parse_bytes().map_err(|_| {
            server.set_error(Status::ResponseError, "failed to parse adminapi response")
        })?;
        let xml = String::from_utf8_lossy(&xml).into_owned();
        if !xml.is_empty() {
            return Err(server.set_error(Status::ResponseError, xml));
        }
    }
    Ok(())
}

/// Parses a WDDX-bearing admin API response.
fn parse_wddx_response(server: &Server, response: &[u8], what: &str) -> Result<Wddx> {
    let mut p = Parser::new(response);
    p.parse_number().map_err(|_| {
        server.set_error(
            Status::ResponseError,
            format!("failed to parse {what} response"),
        )
    })?;
    let xml = p.parse_bytes().map_err(|_| {
        server.set_error(
            Status::ResponseError,
            format!("failed to parse {what} response"),
        )
    })?;
    let xml = String::from_utf8_lossy(&xml).into_owned();
    Wddx::from_xml(&xml).ok_or_else(|| {
        server.set_error(
            Status::ResponseError,
            format!("invalid WDDX in {what} response"),
        )
    })
}

/// Parses an integer the way C's `atoi` does (0 on failure).
fn parse_atoi(bytes: &[u8]) -> i32 {
    let s = std::str::from_utf8(bytes).unwrap_or("");
    s.parse().unwrap_or(0)
}
