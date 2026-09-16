//! C-ABI debugger commands and event accessors
//! (port of `src/cfrds_debugger.c`).

use std::ffi::{c_char, c_int, c_void, CString};
use std::ptr;

use crate::error::Status;
use crate::ffi::wddx::{
    wddx_get_number_safe, wddx_get_string_safe, wddx_get_var_safe, wddx_node_array_size_safe,
    wddx_node_string_safe, wddx_node_struct_at_safe, wddx_node_type_safe, FfiWddx, WNode,
    WDDX_STRING, WDDX_STRUCT,
};
use crate::ffi::{cfrds_status, cstr_alloc, cstr_to_str, FfiServer};

// ---------------------------------------------------------------------------
// Event accessors
// ---------------------------------------------------------------------------

/// Retrieves the type classification of a debugging event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_type(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 3; // CFRDS_DEBUGGER_EVENT_UNKNOWN
    }
    let name = wddx_get_string_safe(event as *const c_void, c"0,EVENT".as_ptr());
    if name.is_null() {
        return 3;
    }
    match cstr_to_str(name).as_str() {
        "CF_BREAKPOINT_SET" => 0,
        "BREAKPOINT" => 1,
        "STEP" => 2,
        _ => 3,
    }
}

/// Retrieves the source file path associated with a breakpoint event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_get_source(
    event: *const FfiWddx,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    wddx_get_string_safe(event as *const c_void, c"0,SOURCE".as_ptr())
}

/// Retrieves the line number location of a breakpoint event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_get_line(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    wddx_get_number_safe(event as *const c_void, c"0,LINE".as_ptr(), ptr::null_mut()) as c_int
}

/// Retrieves raw WDDX scopes variable associated with a breakpoint context.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_get_scopes(
    event: *const FfiWddx,
) -> *const WNode {
    if event.is_null() {
        return ptr::null();
    }
    wddx_get_var_safe(event as *const c_void, c"0,SCOPES".as_ptr())
}

/// Retrieves the execution thread name of the breakpoint event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_get_thread_name(
    event: *const FfiWddx,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    wddx_get_string_safe(event as *const c_void, c"0,THREAD".as_ptr())
}

/// Retrieves the target file path where a breakpoint was set.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_set_get_pathname(
    event: *const FfiWddx,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    wddx_get_string_safe(event as *const c_void, c"0,CFML_PATH".as_ptr())
}

/// Retrieves the requested line number for a breakpoint set event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_set_get_req_line(
    event: *const FfiWddx,
) -> c_int {
    if event.is_null() {
        return 0;
    }
    wddx_get_number_safe(
        event as *const c_void,
        c"0,REQ_LINE_NUM".as_ptr(),
        ptr::null_mut(),
    ) as c_int
}

/// Retrieves the actual bound line number for a breakpoint set event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_breakpoint_set_get_act_line(
    event: *const FfiWddx,
) -> c_int {
    if event.is_null() {
        return 0;
    }
    wddx_get_number_safe(
        event as *const c_void,
        c"0,ACTUAL_LINE_NUM".as_ptr(),
        ptr::null_mut(),
    ) as c_int
}

/// Returns the number of variable scopes in the event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_scopes_count(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,SCOPES".as_ptr());
    wddx_node_array_size_safe(node as *const c_void)
}

/// Retrieves the scope variable name at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_scopes_item_name(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,SCOPES".as_ptr());
    if node.is_null() {
        return ptr::null();
    }
    let mut name: *const c_char = ptr::null();
    wddx_node_struct_at_safe(node as *const c_void, ndx, &mut name);
    name
}

/// Retrieves the scope variable value node at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_scopes_item_value(
    event: *const FfiWddx,
    ndx: usize,
) -> *const WNode {
    if event.is_null() {
        return ptr::null();
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,SCOPES".as_ptr());
    if node.is_null() {
        return ptr::null();
    }
    wddx_node_struct_at_safe(node as *const c_void, ndx, ptr::null_mut())
}

/// Returns the number of execution threads in the event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_threads_count(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,THREADS".as_ptr());
    wddx_node_array_size_safe(node as *const c_void)
}

fn thread_item(event: *const FfiWddx, ndx: usize, suffix: u8) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    let key = CString::new(format!("0,THREADS,{},{suffix}", ndx)).unwrap_or_default();
    let node = wddx_get_var_safe(event as *const c_void, key.as_ptr());
    if node.is_null() || wddx_node_type_safe(node as *const c_void) != WDDX_STRING {
        return ptr::null();
    }
    wddx_node_string_safe(node)
}

/// Retrieves the execution thread name at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_threads_item_name(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    thread_item(event, ndx, 0)
}

/// Retrieves the execution thread state at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_threads_item_state(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    thread_item(event, ndx, 1)
}

/// Returns the count of watch variables in the event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_watch_count(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,WATCH".as_ptr());
    wddx_node_array_size_safe(node as *const c_void)
}

/// Retrieves the watch variable detail at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_watch_item(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,WATCH".as_ptr());
    if node.is_null() {
        return ptr::null();
    }
    if wddx_node_type_safe(node as *const c_void) == WDDX_STRUCT {
        let mut name: *const c_char = ptr::null();
        wddx_node_struct_at_safe(node as *const c_void, ndx, &mut name);
        name
    } else {
        let item = crate::ffi::wddx::wddx_node_array_at_safe(node as *const c_void, ndx);
        if item.is_null() || wddx_node_type_safe(item as *const c_void) != WDDX_STRING {
            return ptr::null();
        }
        wddx_node_string_safe(item)
    }
}

/// Returns the count of ColdFusion trace entries in the event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_cf_trace_count(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,CF_TRACE".as_ptr());
    wddx_node_array_size_safe(node as *const c_void)
}

/// Retrieves the CF trace item at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_cf_trace_item(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    let key = CString::new(format!("0,CF_TRACE,{}", ndx)).unwrap_or_default();
    let node = wddx_get_var_safe(event as *const c_void, key.as_ptr());
    if node.is_null() || wddx_node_type_safe(node as *const c_void) != WDDX_STRING {
        return ptr::null();
    }
    wddx_node_string_safe(node)
}

/// Returns the count of Java trace entries in the event.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_java_trace_count(event: *const FfiWddx) -> c_int {
    if event.is_null() {
        return 0;
    }
    let node = wddx_get_var_safe(event as *const c_void, c"0,JAVA_TRACE".as_ptr());
    wddx_node_array_size_safe(node as *const c_void)
}

/// Retrieves the Java trace item at an index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_get_java_trace_item(
    event: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    if event.is_null() {
        return ptr::null();
    }
    let key = CString::new(format!("0,JAVA_TRACE,{}", ndx)).unwrap_or_default();
    let node = wddx_get_var_safe(event as *const c_void, key.as_ptr());
    if node.is_null() || wddx_node_type_safe(node as *const c_void) != WDDX_STRING {
        return ptr::null();
    }
    wddx_node_string_safe(node)
}

/// Frees an allocated debugger event structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_free(event: *mut FfiWddx) {
    if !event.is_null() {
        drop(Box::from_raw(event));
    }
}

/// Automatically deallocates and nullifies a debugger event pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_debugger_event_cleanup(event: *mut *mut FfiWddx) {
    if !event.is_null() {
        cfrds_debugger_event_free(*event);
        *event = ptr::null_mut();
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

/// Starts a ColdFusion debugging session and returns the session ID.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_start(
    server: *mut FfiServer,
    session_id: *mut *mut c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_start() {
        Ok(session) => {
            *session_id = cstr_alloc(&session);
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Stops a ColdFusion debugging session.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_stop(
    server: *mut FfiServer,
    session_id: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_stop(&cstr_to_str(session_id)) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Stops ColdFusion debugger server and disconnects from JVM.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_server_stop(
    server: *mut FfiServer,
    session_id: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_server_stop(&cstr_to_str(session_id)) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Retrieves the debugger server host connection port.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_get_server_info(
    server: *mut FfiServer,
    session_id: *const c_char,
    port: *mut u16,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || port.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_get_server_info(&cstr_to_str(session_id))
    {
        Ok(p) => {
            *port = p;
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Configures whether breakpoint traps trigger on unhandled exceptions (session level).
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_breakpoint_on_exception(
    server: *mut FfiServer,
    session_id: *const c_char,
    value: bool,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_breakpoint_on_exception(&cstr_to_str(session_id), value)
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Configures whether breakpoint traps trigger on unhandled exceptions globally.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_global_breakpoint_on_exception(
    server: *mut FfiServer,
    session_id: *const c_char,
    value: bool,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_global_breakpoint_on_exception(&cstr_to_str(session_id), value)
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Configures a breakpoint at a remote file path line location.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_breakpoint(
    server: *mut FfiServer,
    session_id: *const c_char,
    filepath: *const c_char,
    line: c_int,
    enable: bool,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || filepath.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_breakpoint(
        &cstr_to_str(session_id),
        &cstr_to_str(filepath),
        line,
        enable,
    ) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Clears all configured breakpoints in the debugger session.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_clear_all_breakpoints(
    server: *mut FfiServer,
    session_id: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_clear_all_breakpoints(&cstr_to_str(session_id))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Long-polls the server for a debugging event (blocking).
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_get_debug_events(
    server: *mut FfiServer,
    session_id: *const c_char,
    event: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || event.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_get_debug_events(&cstr_to_str(session_id))
    {
        Ok(ev) => {
            *event = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Long-polls the server for a debugging event with detail fetch flags.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_all_fetch_flags_enabled(
    server: *mut FfiServer,
    session_id: *const c_char,
    threads: bool,
    watch: bool,
    scopes: bool,
    cf_trace: bool,
    java_trace: bool,
    event: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_all_fetch_flags_enabled(
        &cstr_to_str(session_id),
        threads,
        watch,
        scopes,
        cf_trace,
        java_trace,
    ) {
        Ok(ev) => {
            *event = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Executes a thread control action.
unsafe fn thread_action(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    action: &str,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    let srv = &(*server).server;
    let result = match action {
        "STEP_IN" => srv.debugger_step_in(&cstr_to_str(session_id), &cstr_to_str(thread_name)),
        "STEP_OVER" => srv.debugger_step_over(&cstr_to_str(session_id), &cstr_to_str(thread_name)),
        "STEP_OUT" => srv.debugger_step_out(&cstr_to_str(session_id), &cstr_to_str(thread_name)),
        _ => srv.debugger_continue(&cstr_to_str(session_id), &cstr_to_str(thread_name)),
    };
    match result {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Steps into the given execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_step_in(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
) -> cfrds_status {
    thread_action(server, session_id, thread_name, "STEP_IN")
}

/// Steps over the given execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_step_over(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
) -> cfrds_status {
    thread_action(server, session_id, thread_name, "STEP_OVER")
}

/// Steps out of the given execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_step_out(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
) -> cfrds_status {
    thread_action(server, session_id, thread_name, "STEP_OUT")
}

/// Executes synchronous step-into debugger command on a target execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_sync_step_in(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    event: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() || event.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_sync_step_in(&cstr_to_str(session_id), &cstr_to_str(thread_name))
    {
        Ok(ev) => {
            *event = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Executes synchronous step-over debugger command on a target execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_sync_step_over(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    event: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() || event.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_sync_step_over(&cstr_to_str(session_id), &cstr_to_str(thread_name))
    {
        Ok(ev) => {
            *event = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Executes synchronous step-out debugger command on a target execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_sync_step_out(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    event: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() || event.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_sync_step_out(&cstr_to_str(session_id), &cstr_to_str(thread_name))
    {
        Ok(ev) => {
            *event = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Resumes the given execution thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_continue(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
) -> cfrds_status {
    thread_action(server, session_id, thread_name, "CONTINUE")
}

/// Retrieves all ColdFusion variable scopes and values for a thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_get_cf_variables(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    variables: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() || variables.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_get_cf_variables(&cstr_to_str(session_id), &cstr_to_str(thread_name))
    {
        Ok(ev) => {
            *variables = Box::into_raw(Box::new(FfiWddx::from_rust(ev.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Configures watch expression evaluation for a thread.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_watch_expression(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    expression: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_watch_expression(
        &cstr_to_str(session_id),
        &cstr_to_str(thread_name),
        &cstr_to_str(expression),
    ) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Changes a variable value in the active execution context.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_set_variable(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    variable: *const c_char,
    value: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.debugger_set_variable(
        &cstr_to_str(session_id),
        &cstr_to_str(thread_name),
        &cstr_to_str(variable),
        &cstr_to_str(value),
    ) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Submits a comma-separated list of variables to watch in a session.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_watch_variables(
    server: *mut FfiServer,
    session_id: *const c_char,
    variables: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || variables.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_watch_variables(&cstr_to_str(session_id), &cstr_to_str(variables))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Retrieves the standard output generated in a thread execution context.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_get_output(
    server: *mut FfiServer,
    session_id: *const c_char,
    thread_name: *const c_char,
    output: *mut *mut c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() || thread_name.is_null() || output.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_get_output(&cstr_to_str(session_id), &cstr_to_str(thread_name))
    {
        Ok(out) => {
            *output = cstr_alloc(&out);
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Configures variable scopes detail filter rules.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_debugger_set_scope_filter(
    server: *mut FfiServer,
    session_id: *const c_char,
    filter: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if session_id.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .debugger_set_scope_filter(&cstr_to_str(session_id), &cstr_to_str(filter))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}
