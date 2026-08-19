//! Debugger commands and event accessors (port of `src/cfrds_debugger.c`).

use crate::error::{Result, Status};
use crate::parser;
use crate::server::Server;
use crate::types::{DebuggerEvent, DebuggerEventType};
use crate::wddx::{Wddx, WddxNode, WddxType};

/// Counts the elements of the array node located at `path`, or 0.
fn array_count(wddx: &Wddx, path: &str) -> usize {
    match wddx.get_var(path) {
        Some(node) if node.node_type() == WddxType::Array => node.node_array_size(),
        _ => 0,
    }
}

impl DebuggerEvent {
    /// Classifies the debugger event type.
    pub fn event_type(&self) -> DebuggerEventType {
        match self.wddx.get_string("0,EVENT") {
            Some("CF_BREAKPOINT_SET") => DebuggerEventType::BreakpointSet,
            Some("BREAKPOINT") => DebuggerEventType::Breakpoint,
            Some("STEP") => DebuggerEventType::Step,
            _ => DebuggerEventType::Unknown,
        }
    }

    /// Returns the source file path of a breakpoint event.
    pub fn breakpoint_source(&self) -> Option<&str> {
        self.wddx.get_string("0,SOURCE")
    }

    /// Returns the line number location of a breakpoint event.
    pub fn breakpoint_line(&self) -> Option<f64> {
        self.wddx.get_number("0,LINE")
    }

    /// Returns the raw WDDX scopes variable of a breakpoint event.
    pub fn breakpoint_scopes(&self) -> Option<&WddxNode> {
        self.wddx.get_var("0,SCOPES")
    }

    /// Returns the execution thread name of a breakpoint event.
    pub fn breakpoint_thread_name(&self) -> Option<&str> {
        self.wddx.get_string("0,THREAD")
    }

    /// Returns the target file path where a breakpoint was set.
    pub fn breakpoint_set_pathname(&self) -> Option<&str> {
        self.wddx.get_string("0,CFML_PATH")
    }

    /// Returns the requested line number for a breakpoint set event.
    pub fn breakpoint_set_req_line(&self) -> Option<f64> {
        self.wddx.get_number("0,REQ_LINE_NUM")
    }

    /// Returns the actual bound line number for a breakpoint set event.
    pub fn breakpoint_set_act_line(&self) -> Option<f64> {
        self.wddx.get_number("0,ACTUAL_LINE_NUM")
    }

    /// Returns the number of variable scopes in the event.
    pub fn scopes_count(&self) -> usize {
        array_count(&self.wddx, "0,SCOPES")
    }

    /// Returns the scope variable name at `ndx`.
    pub fn scopes_item_name(&self, ndx: usize) -> Option<&str> {
        let node = self.wddx.get_var("0,SCOPES")?;
        node.node_struct_at(ndx).map(|(name, _)| name)
    }

    /// Returns the scope variable value node at `ndx`.
    pub fn scopes_item_value(&self, ndx: usize) -> Option<&WddxNode> {
        let node = self.wddx.get_var("0,SCOPES")?;
        node.node_struct_at(ndx).map(|(_, value)| value)
    }

    /// Returns the number of execution threads in the event.
    pub fn threads_count(&self) -> usize {
        array_count(&self.wddx, "0,THREADS")
    }

    /// Returns the thread name at `ndx`.
    pub fn threads_item_name(&self, ndx: usize) -> Option<&str> {
        let key = format!("0,THREADS,{},0", ndx);
        let node = self.wddx.get_var(&key)?;
        node.node_string()
    }

    /// Returns the thread state at `ndx`.
    pub fn threads_item_state(&self, ndx: usize) -> Option<&str> {
        let key = format!("0,THREADS,{},1", ndx);
        let node = self.wddx.get_var(&key)?;
        node.node_string()
    }

    /// Returns the count of watch variables in the event.
    pub fn watch_count(&self) -> usize {
        array_count(&self.wddx, "0,WATCH")
    }

    /// Returns the watch variable detail at `ndx`.
    pub fn watch_item(&self, ndx: usize) -> Option<&str> {
        let node = self.wddx.get_var("0,WATCH")?;
        if node.node_type() == WddxType::Struct {
            node.node_struct_at(ndx).map(|(name, _)| name)
        } else {
            let item = node.node_array_at(ndx)?;
            item.node_string()
        }
    }

    /// Returns the count of ColdFusion trace entries in the event.
    pub fn cf_trace_count(&self) -> usize {
        array_count(&self.wddx, "0,CF_TRACE")
    }

    /// Returns the CF trace item at `ndx`.
    pub fn cf_trace_item(&self, ndx: usize) -> Option<&str> {
        let key = format!("0,CF_TRACE,{}", ndx);
        let node = self.wddx.get_var(&key)?;
        node.node_string()
    }

    /// Returns the count of Java trace entries in the event.
    pub fn java_trace_count(&self) -> usize {
        array_count(&self.wddx, "0,JAVA_TRACE")
    }

    /// Returns the Java trace item at `ndx`.
    pub fn java_trace_item(&self, ndx: usize) -> Option<&str> {
        let key = format!("0,JAVA_TRACE,{}", ndx);
        let node = self.wddx.get_var(&key)?;
        node.node_string()
    }
}

impl Server {
    /// Starts a ColdFusion debugging session and returns the session ID.
    pub fn debugger_start(&self) -> Result<String> {
        let mut wddx = Wddx::new();
        wddx.put_bool("0,REMOTE_SESSION", true);

        let response = self.send_command("DBGREQUEST", &["DBG_START", &wddx.to_xml()])?;
        parser::buffer_to_debugger_start(&response).ok_or_else(|| {
            self.set_error(Status::ResponseError, "failed to parse DBG_START response")
        })
    }

    /// Stops a ColdFusion debugging session.
    pub fn debugger_stop(&self, session_id: &str) -> Result<()> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let response = self.send_command("DBGREQUEST", &["DBG_STOP", session_id])?;
        if !parser::buffer_to_debugger_stop(&response) {
            return Err(self.set_error(Status::ResponseError, "DBG_STOP failed"));
        }
        Ok(())
    }

    /// Retrieves the debugger server host connection port.
    pub fn debugger_get_server_info(&self, session_id: &str) -> Result<u16> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let response =
            self.send_command("DBGREQUEST", &["DBG_GET_DEBUG_SERVER_INFO", session_id])?;
        let port = parser::buffer_to_debugger_info(&response).ok_or_else(|| {
            self.set_error(
                Status::ResponseError,
                "failed to parse DBG_GET_DEBUG_SERVER_INFO",
            )
        })?;
        if port < 0 || port > u16::MAX as i32 {
            return Err(self.set_error(Status::ResponseError, "invalid debugger port"));
        }
        Ok(port as u16)
    }

    /// Configures whether breakpoint traps trigger on unhandled exceptions.
    pub fn debugger_breakpoint_on_exception(&self, session_id: &str, value: bool) -> Result<()> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_bool("0,BREAK_ON_EXCEPTION", value);
        wddx.put_string("0,COMMAND", "SESSION_BREAK_ON_EXCEPTION");

        let response =
            self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        if parser::buffer_to_debugger_info(&response).is_none() {
            return Err(self.set_error(Status::ResponseError, "SESSION_BREAK_ON_EXCEPTION failed"));
        }
        Ok(())
    }

    /// Configures a breakpoint at a remote file path line location.
    pub fn debugger_breakpoint(
        &self,
        session_id: &str,
        filepath: &str,
        line: i32,
        enable: bool,
    ) -> Result<()> {
        if session_id.is_empty() || filepath.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_number("0,Y", line as f64);
        wddx.put_string(
            "0,COMMAND",
            if enable {
                "SET_BREAKPOINT"
            } else {
                "UNSET_BREAKPOINT"
            },
        );
        wddx.put_string("0,FILE", filepath);
        wddx.put_number("0,SEQ", 1.0);

        let response =
            self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        if !debugger_response_ok(&response) {
            return Err(self.set_error(Status::ResponseError, "breakpoint command failed"));
        }
        Ok(())
    }

    /// Clears all configured breakpoints in the debugger session.
    pub fn debugger_clear_all_breakpoints(&self, session_id: &str) -> Result<()> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,COMMAND", "UNSET_ALL_BREAKPOINTS");

        let response =
            self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        if !debugger_response_ok(&response) {
            return Err(self.set_error(Status::ResponseError, "UNSET_ALL_BREAKPOINTS failed"));
        }
        Ok(())
    }

    /// Long-polls the server for a debugging event (blocking).
    pub fn debugger_get_debug_events(&self, session_id: &str) -> Result<DebuggerEvent> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let response = self.send_command("DBGREQUEST", &["DBG_EVENTS", session_id])?;
        parse_debugger_event(self, &response)
    }

    /// Long-polls the server for a debugging event with detail fetch flags.
    pub fn debugger_all_fetch_flags_enabled(
        &self,
        session_id: &str,
        threads: bool,
        watch: bool,
        scopes: bool,
        cf_trace: bool,
        java_trace: bool,
    ) -> Result<DebuggerEvent> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_bool("THREADS", threads);
        wddx.put_bool("WATCH", watch);
        wddx.put_bool("SCOPES", scopes);
        wddx.put_bool("CF_TRACE", cf_trace);
        wddx.put_bool("JAVA_TRACE", java_trace);

        let response =
            self.send_command("DBGREQUEST", &["DBG_EVENTS", session_id, &wddx.to_xml()])?;
        parse_debugger_event(self, &response)
    }

    /// Steps into the given execution thread.
    pub fn debugger_step_in(&self, session_id: &str, thread_name: &str) -> Result<()> {
        self.debugger_thread_action(session_id, thread_name, "STEP_IN")
    }

    /// Steps over the given execution thread.
    pub fn debugger_step_over(&self, session_id: &str, thread_name: &str) -> Result<()> {
        self.debugger_thread_action(session_id, thread_name, "STEP_OVER")
    }

    /// Steps out of the given execution thread.
    pub fn debugger_step_out(&self, session_id: &str, thread_name: &str) -> Result<()> {
        self.debugger_thread_action(session_id, thread_name, "STEP_OUT")
    }

    /// Resumes the given execution thread.
    pub fn debugger_continue(&self, session_id: &str, thread_name: &str) -> Result<()> {
        self.debugger_thread_action(session_id, thread_name, "CONTINUE")
    }

    /// Configures watch expression evaluation for a thread.
    pub fn debugger_watch_expression(
        &self,
        session_id: &str,
        thread_name: &str,
        expression: &str,
    ) -> Result<()> {
        if session_id.is_empty() || thread_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,VARIABLE_NAME", expression);
        wddx.put_string("0,COMMAND", "GET_SINGLE_CF_VARIABLE");
        wddx.put_string("0,THREAD", thread_name);

        self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        Ok(())
    }

    /// Changes a variable value in the active execution context.
    pub fn debugger_set_variable(
        &self,
        session_id: &str,
        thread_name: &str,
        variable: &str,
        value: &str,
    ) -> Result<()> {
        if session_id.is_empty() || thread_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,VARIABLE_VALUE", value);
        wddx.put_string("0,VARIABLE_NAME", variable);
        wddx.put_string("0,COMMAND", "SET_VARIABLE_VALUE");
        wddx.put_string("0,THREAD", thread_name);

        self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        Ok(())
    }

    /// Submits a comma-separated list of variables to watch in a session.
    pub fn debugger_watch_variables(&self, session_id: &str, variables: &str) -> Result<()> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,COMMAND", "SET_WATCH_VARIABLES");

        let mut index = 0usize;
        for variable in variables.split(',') {
            if variable.is_empty() {
                continue;
            }
            let key = format!("0,WATCH,{}", index);
            wddx.put_string(&key, variable);
            index += 1;
        }

        if index == 0 {
            return Err(Status::InvalidInputParameter.into());
        }

        self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        Ok(())
    }

    /// Retrieves the standard output generated in a thread execution context.
    pub fn debugger_get_output(&self, session_id: &str, thread_name: &str) -> Result<String> {
        if session_id.is_empty() || thread_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,COMMAND", "GET_OUTPUT");
        wddx.put_bool("0,BODY_ONLY", true);
        wddx.put_string("0,THREAD", thread_name);

        let response =
            self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;

        let mut p = crate::buffer::Parser::new(&response);
        let count = p.parse_number().map_err(|_| {
            self.set_error(Status::ResponseError, "failed to parse GET_OUTPUT response")
        })?;
        if count != 1 {
            return Err(self.set_error(Status::ResponseError, "invalid GET_OUTPUT response"));
        }
        let xml = p.parse_bytes().map_err(|_| {
            self.set_error(Status::ResponseError, "failed to parse GET_OUTPUT response")
        })?;
        let xml = String::from_utf8_lossy(&xml).into_owned();

        if !xml.is_empty() {
            let result = Wddx::from_xml(&xml).ok_or_else(|| {
                self.set_error(Status::ResponseError, "invalid WDDX in GET_OUTPUT response")
            })?;
            Ok(result.get_string("0,VALUE").unwrap_or("").to_string())
        } else {
            Ok(String::new())
        }
    }

    /// Configures variable scopes detail filter rules.
    pub fn debugger_set_scope_filter(&self, session_id: &str, filter: &str) -> Result<()> {
        if session_id.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,FILTER", filter);
        wddx.put_string("0,COMMAND", "SET_SCOPE_FILTER");

        self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        Ok(())
    }

    /// Sends a thread action command and verifies the response.
    fn debugger_thread_action(
        &self,
        session_id: &str,
        thread_name: &str,
        action: &str,
    ) -> Result<()> {
        if session_id.is_empty() || thread_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        let mut wddx = Wddx::new();
        wddx.put_string("0,COMMAND", action);
        wddx.put_string("0,THREAD", thread_name);

        let response =
            self.send_command("DBGREQUEST", &["DBG_REQUEST", session_id, &wddx.to_xml()])?;
        if !debugger_response_ok(&response) {
            return Err(self.set_error(Status::ResponseError, "debugger thread action failed"));
        }
        Ok(())
    }
}

/// Parses a debugger events response into a `DebuggerEvent`.
fn parse_debugger_event(server: &Server, response: &[u8]) -> Result<DebuggerEvent> {
    let wddx = parser::buffer_to_debugger_event(response)
        .ok_or_else(|| server.set_error(Status::ResponseError, "failed to parse debugger event"))?;
    Ok(DebuggerEvent::from(wddx))
}

/// Verifies a generic debugger request response (`0` rows, or a `VALUE != -1`).
fn debugger_response_ok(response: &[u8]) -> bool {
    let mut p = crate::buffer::Parser::new(response);
    let rows = match p.parse_number() {
        Ok(n) => n,
        Err(_) => return false,
    };

    if rows == 0 {
        return true;
    }
    if rows == 1 {
        let xml = match p.parse_bytes() {
            Ok(b) => String::from_utf8_lossy(&b).into_owned(),
            Err(_) => return false,
        };
        let wddx = match Wddx::from_xml(&xml) {
            Some(w) => w,
            None => return false,
        };
        if let Some(value) = wddx.get_number("0,VALUE") {
            if value == -1.0 {
                return false;
            }
        }
        return true;
    }

    false
}
