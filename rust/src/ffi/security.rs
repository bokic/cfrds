//! C-ABI security analyzer commands and result accessors
//! (port of `src/cfrds_security_analyzer.c`).

use std::ffi::{c_char, c_int};
use std::ptr;

use serde_json::Value;

use crate::error::Status;
use crate::ffi::{cfrds_status, cstr_alloc, cstr_to_str, FfiServer};
use crate::types::SecurityAnalyzerResult;

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

/// Submits a path list for a remote vulnerability/security scan.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_security_analyzer_scan(
    server: *mut FfiServer,
    pathnames: *const c_char,
    recursively: bool,
    cores: c_int,
    command_id: *mut c_int,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server)
        .server
        .security_analyzer_scan(&cstr_to_str(pathnames), recursively, cores)
    {
        Ok(id) => {
            if !command_id.is_null() {
                *command_id = id;
            }
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Cancels an active security scan task.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_security_analyzer_cancel(
    server: *mut FfiServer,
    command_id: c_int,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.security_analyzer_cancel(command_id) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Queries progress and metadata state of an active scan task.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_security_analyzer_status(
    server: *mut FfiServer,
    command_id: c_int,
    totalfiles: *mut c_int,
    filesvisitedcount: *mut c_int,
    percentage: *mut c_int,
    lastupdated: *mut i64,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.security_analyzer_status(command_id) {
        Ok(status) => {
            if !totalfiles.is_null() {
                *totalfiles = status.totalfiles as c_int;
            }
            if !filesvisitedcount.is_null() {
                *filesvisitedcount = status.filesvisitedcount as c_int;
            }
            if !percentage.is_null() {
                *percentage = status.percentage as c_int;
            }
            if !lastupdated.is_null() {
                *lastupdated = status.lastupdated;
            }
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Retrieves the scan results report for a command.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_security_analyzer_result(
    server: *mut FfiServer,
    command_id: c_int,
    result: *mut *mut SecurityAnalyzerResult,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.security_analyzer_result(command_id) {
        Ok(r) => {
            *result = Box::into_raw(Box::new(r));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Cleans/destroys scan task results state stored on the server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_security_analyzer_clean(
    server: *mut FfiServer,
    command_id: c_int,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.security_analyzer_clean(command_id) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// Result free / accessors
// ---------------------------------------------------------------------------

/// Frees an allocated security analyzer result structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_free(value: *mut SecurityAnalyzerResult) {
    if !value.is_null() {
        drop(Box::from_raw(value));
    }
}

/// Automatically deallocates and nullifies a security analyzer result pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_cleanup(
    value: *mut *mut SecurityAnalyzerResult,
) {
    if !value.is_null() {
        cfrds_security_analyzer_result_free(*value);
        *value = ptr::null_mut();
    }
}

fn json_of<'a>(value: *const SecurityAnalyzerResult) -> Option<&'a Value> {
    if value.is_null() {
        return None;
    }
    // SAFETY: the handle is only ever produced by this module and is valid.
    Some(unsafe { &(*value).json })
}

fn int_field(value: *const SecurityAnalyzerResult, key: &str) -> i64 {
    json_of(value)
        .and_then(|v| v.get(key))
        .and_then(Value::as_i64)
        .unwrap_or(-1)
}

fn array_len(value: *const SecurityAnalyzerResult, key: &str) -> i64 {
    match json_of(value)
        .and_then(|v| v.get(key))
        .and_then(Value::as_array)
    {
        Some(arr) => arr.len() as i64,
        None => -1,
    }
}

fn string_alloc(value: *const SecurityAnalyzerResult, key: &str) -> *mut c_char {
    match json_of(value)
        .and_then(|v| v.get(key))
        .and_then(Value::as_str)
    {
        Some(s) => cstr_alloc(s),
        None => ptr::null_mut(),
    }
}

fn array_item_string_alloc(
    value: *const SecurityAnalyzerResult,
    array_key: &str,
    ndx: usize,
    field_key: &str,
) -> *mut c_char {
    let item = json_of(value)
        .and_then(|v| v.get(array_key))
        .and_then(Value::as_array)
        .and_then(|arr| arr.get(ndx));
    match item.and_then(|i| i.get(field_key)).and_then(Value::as_str) {
        Some(s) => cstr_alloc(s),
        None => ptr::null_mut(),
    }
}

fn array_item_int(
    value: *const SecurityAnalyzerResult,
    array_key: &str,
    ndx: usize,
    field_key: &str,
) -> i64 {
    let item = json_of(value)
        .and_then(|v| v.get(array_key))
        .and_then(Value::as_array)
        .and_then(|arr| arr.get(ndx));
    item.and_then(|i| i.get(field_key))
        .and_then(Value::as_i64)
        .unwrap_or(-1)
}

macro_rules! int_accessor {
    ($name:ident, $key:literal) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(value: *const SecurityAnalyzerResult) -> c_int {
            int_field(value, $key) as c_int
        }
    };
}

macro_rules! array_len_accessor {
    ($name:ident, $key:literal) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(value: *const SecurityAnalyzerResult) -> c_int {
            array_len(value, $key) as c_int
        }
    };
}

int_accessor!(cfrds_security_analyzer_result_totalfiles, "totalfiles");
int_accessor!(
    cfrds_security_analyzer_result_filesvisitedcount,
    "filesvisitedcount"
);
int_accessor!(cfrds_security_analyzer_result_percentage, "percentage");
int_accessor!(
    cfrds_security_analyzer_result_filesnotscannedcount,
    "filesnotscannedcount"
);
int_accessor!(
    cfrds_security_analyzer_result_filesscannedcount,
    "filesscannedcount"
);
int_accessor!(cfrds_security_analyzer_result_id, "id");

array_len_accessor!(
    cfrds_security_analyzer_result_errorsdescription_count,
    "errorsdescription"
);
array_len_accessor!(
    cfrds_security_analyzer_result_filesscanned_count,
    "filesscanned"
);
array_len_accessor!(
    cfrds_security_analyzer_result_filesnotscanned_count,
    "filesnotscanned"
);
array_len_accessor!(cfrds_security_analyzer_result_files_count, "files");
array_len_accessor!(
    cfrds_security_analyzer_result_filesvisited_count,
    "filesvisited"
);
array_len_accessor!(cfrds_security_analyzer_result_errors_count, "errors");

/// Retrieves the last updated timestamp of the scan result state.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_lastupdated(
    value: *const SecurityAnalyzerResult,
) -> i64 {
    int_field(value, "lastupdated")
}

/// Retrieves the job status string (empty when missing).
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_status(
    value: *const SecurityAnalyzerResult,
) -> *mut c_char {
    match json_of(value)
        .and_then(|v| v.get("status"))
        .and_then(Value::as_str)
    {
        Some(s) => cstr_alloc(s),
        None => cstr_alloc(""),
    }
}

/// Retrieves the executor service backend description.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_executorservice(
    value: *const SecurityAnalyzerResult,
) -> *mut c_char {
    string_alloc(value, "executorservice")
}

/// Retrieves the path name value at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_files_value(
    value: *const SecurityAnalyzerResult,
    ndx: usize,
) -> *mut c_char {
    let item = json_of(value)
        .and_then(|v| v.get("files"))
        .and_then(Value::as_array)
        .and_then(|arr| arr.get(ndx));
    match item.and_then(Value::as_str) {
        Some(s) => cstr_alloc(s),
        None => ptr::null_mut(),
    }
}

/// Retrieves the result category of a scanned file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_filesscanned_item_result(
    value: *const SecurityAnalyzerResult,
    ndx: usize,
) -> *mut c_char {
    array_item_string_alloc(value, "filesscanned", ndx, "result")
}

/// Retrieves the file path of a scanned file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_filesscanned_item_filename(
    value: *const SecurityAnalyzerResult,
    ndx: usize,
) -> *mut c_char {
    array_item_string_alloc(value, "filesscanned", ndx, "filename")
}

/// Retrieves the skip reason of a skipped file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_filesnotscanned_item_reason(
    value: *const SecurityAnalyzerResult,
    ndx: usize,
) -> *mut c_char {
    array_item_string_alloc(value, "filesnotscanned", ndx, "reason")
}

/// Retrieves the path of a skipped file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_security_analyzer_result_filesnotscanned_item_filename(
    value: *const SecurityAnalyzerResult,
    ndx: usize,
) -> *mut c_char {
    array_item_string_alloc(value, "filesnotscanned", ndx, "filename")
}

// --- errors array accessors ---

macro_rules! errors_str_accessor {
    ($name:ident, $key:literal) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(
            value: *const SecurityAnalyzerResult,
            ndx: usize,
        ) -> *mut c_char {
            array_item_string_alloc(value, "errors", ndx, $key)
        }
    };
}

macro_rules! errors_int_accessor {
    ($name:ident, $key:literal) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(value: *const SecurityAnalyzerResult, ndx: usize) -> c_int {
            array_item_int(value, "errors", ndx, $key) as c_int
        }
    };
}

errors_str_accessor!(
    cfrds_security_analyzer_result_errors_item_errormessage,
    "errormessage"
);
errors_str_accessor!(cfrds_security_analyzer_result_errors_item_path, "path");
errors_str_accessor!(
    cfrds_security_analyzer_result_errors_item_vulnerablecode,
    "vulnerablecode"
);
errors_str_accessor!(
    cfrds_security_analyzer_result_errors_item_filename,
    "filename"
);
errors_str_accessor!(cfrds_security_analyzer_result_errors_item_error, "Error");
errors_str_accessor!(cfrds_security_analyzer_result_errors_item_type, "type");
errors_str_accessor!(
    cfrds_security_analyzer_result_errors_item_referencetype,
    "referencetype"
);
errors_int_accessor!(
    cfrds_security_analyzer_result_errors_item_endline,
    "endline"
);
errors_int_accessor!(
    cfrds_security_analyzer_result_errors_item_beginline,
    "beginline"
);
errors_int_accessor!(cfrds_security_analyzer_result_errors_item_column, "column");
errors_int_accessor!(
    cfrds_security_analyzer_result_errors_item_begincolumn,
    "begincolumn"
);
errors_int_accessor!(
    cfrds_security_analyzer_result_errors_item_endcolumn,
    "endcolumn"
);
