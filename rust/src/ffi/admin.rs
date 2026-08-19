//! C-ABI Admin API, IDE handshake and graphing commands
//! (port of the corresponding parts of `src/cfrds.c`).

use std::ffi::{c_char, c_int, c_void};
use std::ptr;

use crate::error::Status;
use crate::ffi::wddx::{
    wddx_data_safe, wddx_node_array_at_safe, wddx_node_array_size_safe, wddx_node_string_safe,
    wddx_node_struct_at_safe, wddx_node_struct_size_safe, wddx_node_type_safe, FfiWddx,
    WDDX_STRING,
};
use crate::ffi::{cfrds_status, cstr_alloc, cstr_to_str, FfiBuffer, FfiServer};

// ---------------------------------------------------------------------------
// IDE handshake
// ---------------------------------------------------------------------------

/// Performs the internal IDE setup check handshake.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_ide_default(
    server: *mut FfiServer,
    version: c_int,
    num1: *mut c_int,
    server_version: *mut *mut c_char,
    client_version: *mut *mut c_char,
    num2: *mut c_int,
    num3: *mut c_int,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.ide_default(version) {
        Ok(result) => {
            *num1 = result.num1;
            *server_version = cstr_alloc(&result.server_version);
            *client_version = cstr_alloc(&result.client_version);
            *num2 = result.num2;
            *num3 = result.num3;
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// Admin API commands
// ---------------------------------------------------------------------------

/// Queries a system debugging log path directory.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_adminapi_debugging_getlogproperty(
    server: *mut FfiServer,
    logdirectory: *const c_char,
    result: *mut *mut c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server)
        .server
        .adminapi_debugging_getlogproperty(&cstr_to_str(logdirectory))
    {
        Ok(value) => {
            *result = cstr_alloc(&value);
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Queries the custom tag path mappings on the server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_adminapi_extensions_getcustomtagpaths(
    server: *mut FfiServer,
    result: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.adminapi_extensions_getcustomtagpaths() {
        Ok(paths) => {
            *result = Box::into_raw(Box::new(FfiWddx::from_rust(paths.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Adds or updates a logical mapping configuration.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_adminapi_extensions_setmapping(
    server: *mut FfiServer,
    name: *const c_char,
    path: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server)
        .server
        .adminapi_extensions_setmapping(&cstr_to_str(name), &cstr_to_str(path))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Deletes a logical mapping configuration.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_adminapi_extensions_deletemapping(
    server: *mut FfiServer,
    mapping: *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server)
        .server
        .adminapi_extensions_deletemapping(&cstr_to_str(mapping))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Queries all logical mapping configurations on the server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_adminapi_extensions_getmappings(
    server: *mut FfiServer,
    result: *mut *mut FfiWddx,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.adminapi_extensions_getmappings() {
        Ok(mappings) => {
            *result = Box::into_raw(Box::new(FfiWddx::from_rust(mappings.wddx)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// Custom tag paths accessors
// ---------------------------------------------------------------------------

/// Frees a custom tag paths collection structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_customtagpaths_free(value: *mut FfiWddx) {
    if !value.is_null() {
        drop(Box::from_raw(value));
    }
}

/// Automatically deallocates and nullifies a custom tag paths pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_customtagpaths_cleanup(value: *mut *mut FfiWddx) {
    if !value.is_null() {
        cfrds_adminapi_customtagpaths_free(*value);
        *value = ptr::null_mut();
    }
}

/// Returns the count of custom tag paths.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_customtagpaths_count(value: *const FfiWddx) -> c_int {
    let data = wddx_data_safe(value as *const c_void);
    wddx_node_array_size_safe(data as *const c_void)
}

/// Retrieves the path string at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_customtagpaths_at(
    value: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    let data = wddx_data_safe(value as *const c_void);
    if data.is_null() {
        return ptr::null();
    }
    let item = wddx_node_array_at_safe(data as *const c_void, ndx);
    if item.is_null() || wddx_node_type_safe(item as *const c_void) != WDDX_STRING {
        return ptr::null();
    }
    wddx_node_string_safe(item)
}

// ---------------------------------------------------------------------------
// Mappings accessors
// ---------------------------------------------------------------------------

/// Frees a mappings collection structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_mappings_free(value: *mut FfiWddx) {
    if !value.is_null() {
        drop(Box::from_raw(value));
    }
}

/// Automatically deallocates and nullifies a mappings pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_mappings_cleanup(value: *mut *mut FfiWddx) {
    if !value.is_null() {
        cfrds_adminapi_mappings_free(*value);
        *value = ptr::null_mut();
    }
}

/// Returns the count of mappings defined.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_mappings_count(value: *const FfiWddx) -> c_int {
    let data = wddx_data_safe(value as *const c_void);
    wddx_node_struct_size_safe(data as *const c_void)
}

/// Retrieves the logical mapping key name at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_mappings_key(
    value: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    let data = wddx_data_safe(value as *const c_void);
    if data.is_null() {
        return ptr::null();
    }
    let mut key: *const c_char = ptr::null();
    wddx_node_struct_at_safe(data as *const c_void, ndx, &mut key);
    key
}

/// Retrieves the physical target path associated with a mapping.
#[no_mangle]
pub unsafe extern "C" fn cfrds_adminapi_mappings_value(
    value: *const FfiWddx,
    ndx: usize,
) -> *const c_char {
    let data = wddx_data_safe(value as *const c_void);
    if data.is_null() {
        return ptr::null();
    }
    let item = wddx_node_struct_at_safe(data as *const c_void, ndx, ptr::null_mut());
    if item.is_null() || wddx_node_type_safe(item as *const c_void) != WDDX_STRING {
        return ptr::null();
    }
    wddx_node_string_safe(item)
}

// ---------------------------------------------------------------------------
// Graphing
// ---------------------------------------------------------------------------

/// Handshakes a charting/rendering query, returning the binary result.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_graphing(
    server: *mut FfiServer,
    out_buffer: *mut *mut FfiBuffer,
    chart_attributes: *const c_char,
    num_series: usize,
    series_data: *const *const c_char,
) -> cfrds_status {
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    if chart_attributes.is_null()
        || out_buffer.is_null()
        || (num_series > 0 && series_data.is_null())
    {
        return Status::ParamIsNull as cfrds_status;
    }

    let mut series: Vec<String> = Vec::with_capacity(num_series);
    for i in 0..num_series {
        series.push(cstr_to_str(*series_data.add(i)));
    }
    let refs: Vec<&str> = series.iter().map(|s| s.as_str()).collect();

    match (*server)
        .server
        .graphing(&cstr_to_str(chart_attributes), &refs)
    {
        Ok(body) => {
            *out_buffer = Box::into_raw(Box::new(FfiBuffer::from_body(&body)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}
