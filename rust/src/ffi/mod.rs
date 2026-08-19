//! C-ABI layer producing a drop-in `libcfrds.so`.
//!
//! Exports the exact same symbol set as the C library (`include/cfrds.h`,
//! `include/internal/cfrds_int.h`, `include/internal/wddx.h`,
//! `include/internal/cfrds_buffer.h`) so that existing C consumers can link
//! against a Rust-built shared library unchanged.
//!
//! All handle types (`cfrds_server`, `cfrds_buffer`, `cfrds_browse_dir`, …) are
//! opaque to the C consumer; the exported functions are the only way to
//! manipulate them. Handles are boxed Rust objects; `cfrds_str` values and
//! `cfrds_server_encode_password` output are allocated with the system
//! allocator so they may be released with `free()`/`cfrds_str_cleanup`.

#![allow(non_camel_case_types)]
#![allow(clippy::missing_safety_doc)]
// The exported `#[no_mangle] extern "C"` functions intentionally reference
// `pub(crate)` opaque handle types; Rust consumers never use them directly.
#![allow(private_interfaces)]

mod admin;
mod debugger;
mod file;
mod security;
mod sql;
mod wddx;

pub(crate) use wddx::FfiWddx;

use std::cell::RefCell;
use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::ptr;

use crate::buffer::encode_password;
use crate::error::Status;
use crate::parser;
use crate::server::Server;

/// `cfrds_status` return type (an integer, mirroring the C enum).
pub(crate) type cfrds_status = c_int;
/// `cfrds_str` (a caller-owned heap string).
pub(crate) type cfrds_str = *mut c_char;

/// Opaque `cfrds_server` handle.
pub(crate) struct FfiServer {
    pub(crate) server: Server,
    pub(crate) host: CString,
    pub(crate) username: CString,
    pub(crate) password: CString,
    pub(crate) error: RefCell<Option<CString>>,
}

/// Opaque `cfrds_buffer` handle (NUL-terminated byte buffer).
pub(crate) struct FfiBuffer {
    pub(crate) bytes: Vec<u8>,
    pub(crate) size: usize,
}

impl FfiBuffer {
    /// Builds a buffer from a raw response body, appending a NUL sentinel.
    pub(crate) fn from_body(body: &[u8]) -> FfiBuffer {
        let mut bytes = Vec::with_capacity(body.len() + 1);
        bytes.extend_from_slice(body);
        bytes.push(0);
        let size = body.len();
        FfiBuffer { bytes, size }
    }
}

/// Converts a Rust string to a `CString`, truncating at the first interior NUL
/// (mirroring C's `strlen` semantics for the server-provided data).
pub(crate) fn to_cstr(s: &str) -> CString {
    CString::new(s.split('\0').next().unwrap_or("")).unwrap_or_default()
}

/// Reads a NUL-terminated C string into a lossy UTF-8 `String`.
pub(crate) fn cstr_to_str(p: *const c_char) -> String {
    if p.is_null() {
        return String::new();
    }
    unsafe { CStr::from_ptr(p) }.to_string_lossy().into_owned()
}

/// Allocates a caller-owned heap string (system allocator, `free()` compatible).
pub(crate) fn cstr_alloc(s: &str) -> *mut c_char {
    let bytes = s.as_bytes();
    let len = bytes.iter().position(|&b| b == 0).unwrap_or(bytes.len());
    unsafe {
        let p = libc::malloc(len + 1) as *mut u8;
        if p.is_null() {
            return ptr::null_mut();
        }
        ptr::copy_nonoverlapping(bytes.as_ptr(), p, len);
        *p.add(len) = 0;
        p as *mut c_char
    }
}

/// Frees a string allocated by `cstr_alloc`.
pub(crate) fn cstr_free(p: *mut c_char) {
    if !p.is_null() {
        unsafe { libc::free(p as *mut c_void) };
    }
}

// ---------------------------------------------------------------------------
// Server context
// ---------------------------------------------------------------------------

/// Initializes a `cfrds_server` connection instance.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_init(
    server: *mut *mut FfiServer,
    host: *const c_char,
    port: u16,
    username: *const c_char,
    password: *const c_char,
) -> bool {
    if server.is_null() || host.is_null() || port == 0 || username.is_null() || password.is_null() {
        return false;
    }
    let host = cstr_to_str(host);
    let username = cstr_to_str(username);
    let password = cstr_to_str(password);

    match Server::new(&host, port, &username, &password) {
        Ok(srv) => {
            let ffi = Box::new(FfiServer {
                server: srv,
                host: CString::new(host.clone()).unwrap_or_default(),
                username: CString::new(username.clone()).unwrap_or_default(),
                password: CString::new(password).unwrap_or_default(),
                error: RefCell::new(None),
            });
            *server = Box::into_raw(ffi);
            true
        }
        Err(_) => false,
    }
}

/// Deallocates all resources associated with a `cfrds_server` instance.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_free(server: *mut FfiServer) {
    if !server.is_null() {
        drop(Box::from_raw(server));
    }
}

/// Automatically deallocates and nullifies a `cfrds_server` pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_cleanup(server: *mut *mut FfiServer) {
    if !server.is_null() {
        cfrds_server_free(*server);
        *server = ptr::null_mut();
    }
}

/// Clears the internal error status of the server instance.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_clear_error(server: *mut FfiServer) {
    if !server.is_null() {
        (*server).server.clear_error();
    }
}

/// Retrieves the last recorded error message for the server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_get_error(server: *const FfiServer) -> *const c_char {
    if server.is_null() {
        return ptr::null();
    }
    let srv = &*server;
    {
        let mut slot = srv.error.borrow_mut();
        *slot = srv
            .server
            .error()
            .map(|e| CString::new(e).unwrap_or_default());
    }
    srv.error
        .borrow()
        .as_ref()
        .map(|c| c.as_ptr())
        .unwrap_or(ptr::null())
}

/// Retrieves the server host address configuration.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_get_host(server: *const FfiServer) -> *const c_char {
    if server.is_null() {
        return ptr::null();
    }
    (*server).host.as_ptr()
}

/// Retrieves the server port configuration.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_get_port(server: *const FfiServer) -> u16 {
    if server.is_null() {
        return 0;
    }
    (*server).server.port()
}

/// Retrieves the server username configuration.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_get_username(server: *const FfiServer) -> *const c_char {
    if server.is_null() {
        return ptr::null();
    }
    (*server).username.as_ptr()
}

/// Retrieves the server password configuration (plaintext).
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_get_password(server: *const FfiServer) -> *const c_char {
    if server.is_null() {
        return ptr::null();
    }
    (*server).password.as_ptr()
}

/// Sets the error state of a `cfrds_server` instance.
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_set_error(
    server: *mut FfiServer,
    error_code: i64,
    error: *const c_char,
) {
    if server.is_null() {
        return;
    }
    let msg = if error.is_null() {
        None
    } else {
        Some(cstr_to_str(error))
    };
    (*server).server.set_error_state(error_code, msg.as_deref());
}

/// Encodes a plaintext password for RDS transmission (caller frees the result).
#[no_mangle]
pub unsafe extern "C" fn cfrds_server_encode_password(password: *const c_char) -> *mut c_char {
    if password.is_null() {
        return ptr::null_mut();
    }
    let encoded = encode_password(&cstr_to_str(password));
    cstr_alloc(&encoded)
}

// ---------------------------------------------------------------------------
// Buffer
// ---------------------------------------------------------------------------

/// Automatically deallocates and nullifies a `cfrds_buffer` pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_cleanup(buffer: *mut *mut FfiBuffer) {
    if !buffer.is_null() && !(*buffer).is_null() {
        drop(Box::from_raw(*buffer));
        *buffer = ptr::null_mut();
    }
}

/// Retrieves the raw character data pointer of the buffer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_data(buffer: *mut FfiBuffer) -> *mut c_char {
    if buffer.is_null() {
        return ptr::null_mut();
    }
    (*buffer).bytes.as_mut_ptr() as *mut c_char
}

/// Returns the active data size in bytes of the buffer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_data_size(buffer: *mut FfiBuffer) -> usize {
    if buffer.is_null() {
        return 0;
    }
    (*buffer).size
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

/// Automatically deallocates and nullifies a `cfrds_str` pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_str_cleanup(string: *mut cfrds_str) {
    if !string.is_null() {
        cstr_free(*string);
        *string = ptr::null_mut();
    }
}

// ---------------------------------------------------------------------------
// Internal command helpers (exported by the C library)
// ---------------------------------------------------------------------------

/// Sends an RDS command with a NULL-terminated argument list.
#[no_mangle]
pub unsafe extern "C" fn cfrds_send_command(
    server: *mut FfiServer,
    response: *mut *mut FfiBuffer,
    command: *const c_char,
    list: *const *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if command.is_null() || list.is_null() {
        return Status::InvalidInputParameter as cfrds_status;
    }

    let command = cstr_to_str(command);

    let mut args: Vec<String> = Vec::new();
    for i in 0..1024usize {
        let p = *list.add(i);
        if p.is_null() {
            break;
        }
        args.push(cstr_to_str(p));
        if i == 1023 {
            return Status::InvalidInputParameter as cfrds_status;
        }
    }

    let refs: Vec<&str> = args.iter().map(|s| s.as_str()).collect();
    match (*server).server.send_command(&command, &refs) {
        Ok(body) => {
            if !response.is_null() {
                *response = Box::into_raw(Box::new(FfiBuffer::from_body(&body)));
            }
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Parser callback type used by `cfrds_execute_sql_cmd`.
pub(crate) type SqlParserFn = unsafe extern "C" fn(*mut FfiBuffer) -> *mut c_void;

/// Executes a SQL command with a NULL-terminated parameter list and a parser
/// callback.
#[no_mangle]
pub unsafe extern "C" fn cfrds_execute_sql_cmd(
    server: *mut FfiServer,
    params: *const *const c_char,
    parser: Option<SqlParserFn>,
    out_result: *mut *mut c_void,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    let parser = match parser {
        Some(p) => p,
        None => return Status::ParamIsNull as cfrds_status,
    };

    let mut args: Vec<String> = Vec::new();
    for i in 0..1024usize {
        let p = *params.add(i);
        if p.is_null() {
            break;
        }
        args.push(cstr_to_str(p));
        if i == 1023 {
            return Status::InvalidInputParameter as cfrds_status;
        }
    }

    let refs: Vec<&str> = args.iter().map(|s| s.as_str()).collect();
    match (*server).server.send_command("DBFUNCS", &refs) {
        Ok(body) => {
            let buffer = Box::new(FfiBuffer::from_body(&body));
            let result = parser(Box::into_raw(buffer));
            if result.is_null() {
                (*server).server.set_error_code(-1);
                return Status::ResponseError as cfrds_status;
            }
            *out_result = result;
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// Debugger response parsers (exported by the C library)
// ---------------------------------------------------------------------------

/// Parses a debugger start response.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_to_debugger_start(buffer: *mut FfiBuffer) -> *mut c_char {
    if buffer.is_null() {
        return ptr::null_mut();
    }
    let buf = &*buffer;
    let data = &buf.bytes[..buf.size];
    match parser::buffer_to_debugger_start(data) {
        Some(s) => cstr_alloc(&s),
        None => ptr::null_mut(),
    }
}

/// Parses a debugger stop response.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_to_debugger_stop(buffer: *mut FfiBuffer) -> bool {
    if buffer.is_null() {
        return false;
    }
    let buf = &*buffer;
    let data = &buf.bytes[..buf.size];
    parser::buffer_to_debugger_stop(data)
}

/// Parses a debugger info response, returning the debug port or -1.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_to_debugger_info(buffer: *mut FfiBuffer) -> c_int {
    if buffer.is_null() {
        return -1;
    }
    let buf = &*buffer;
    let data = &buf.bytes[..buf.size];
    parser::buffer_to_debugger_info(data).unwrap_or(-1)
}

/// Parses a debugger events response into a `cfrds_debugger_event`.
#[no_mangle]
pub unsafe extern "C" fn cfrds_buffer_to_debugger_event(buffer: *mut FfiBuffer) -> *mut FfiWddx {
    if buffer.is_null() {
        return ptr::null_mut();
    }
    let buf = &*buffer;
    let data = &buf.bytes[..buf.size];
    match parser::buffer_to_debugger_event(data) {
        Some(wddx) => Box::into_raw(Box::new(FfiWddx::from_rust(wddx))),
        None => ptr::null_mut(),
    }
}
