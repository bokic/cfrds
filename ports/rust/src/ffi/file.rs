//! C-ABI file/directory commands (port of `src/cfrds_file.c` and the related
//! result accessors from `src/cfrds.c`).

use std::ffi::{c_char, c_void, CString};
use std::ptr;

use crate::error::Status;
use crate::ffi::{cfrds_status, cstr_alloc, cstr_to_str, to_cstr, FfiServer};
use crate::types::{BrowseDir, FileContent};

// ---------------------------------------------------------------------------
// Result handles
// ---------------------------------------------------------------------------

/// Opaque `cfrds_browse_dir` handle.
pub(crate) struct FBrowseDir {
    items: Vec<FBrowseDirItem>,
}

pub(crate) struct FBrowseDirItem {
    kind: c_char,
    name: CString,
    permissions: u8,
    size: usize,
    modified: u64,
}

impl From<BrowseDir> for FBrowseDir {
    fn from(dir: BrowseDir) -> FBrowseDir {
        FBrowseDir {
            items: dir
                .iter()
                .map(|i| FBrowseDirItem {
                    kind: i.kind as c_char,
                    name: to_cstr(&i.name),
                    permissions: i.permissions,
                    size: i.size,
                    modified: i.modified,
                })
                .collect(),
        }
    }
}

/// Opaque `cfrds_file_content` handle.
pub(crate) struct FFileContent {
    data: Vec<u8>,
    size: usize,
    modified: CString,
    permission: CString,
}

impl From<FileContent> for FFileContent {
    fn from(fc: FileContent) -> FFileContent {
        let mut data = fc.data().to_vec();
        data.push(0);
        FFileContent {
            size: fc.size(),
            data,
            modified: to_cstr(fc.modified()),
            permission: to_cstr(fc.permission()),
        }
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

/// Lists files and folders in a remote directory.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_browse_dir(
    server: *mut FfiServer,
    path: *const c_char,
    out: *mut *mut FBrowseDir,
) -> cfrds_status {
    if server.is_null() || path.is_null() || out.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.browse_dir(&cstr_to_str(path)) {
        Ok(dir) => {
            *out = Box::into_raw(Box::new(FBrowseDir::from(dir)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Reads the contents of a remote file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_read(
    server: *mut FfiServer,
    pathname: *const c_char,
    out: *mut *mut FFileContent,
) -> cfrds_status {
    if server.is_null() || pathname.is_null() || out.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.file_read(&cstr_to_str(pathname)) {
        Ok(content) => {
            *out = Box::into_raw(Box::new(FFileContent::from(content)));
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Writes raw bytes to a remote file path.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_write(
    server: *mut FfiServer,
    pathname: *const c_char,
    data: *const c_void,
    length: usize,
) -> cfrds_status {
    if server.is_null() || pathname.is_null() || (data.is_null() && length > 0) {
        return Status::ParamIsNull as cfrds_status;
    }
    let bytes = if length > 0 {
        std::slice::from_raw_parts(data as *const u8, length)
    } else {
        &[]
    };
    match (*server).server.file_write(&cstr_to_str(pathname), bytes) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Renames or moves a remote file or folder.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_rename(
    server: *mut FfiServer,
    current_pathname: *const c_char,
    new_pathname: *const c_char,
) -> cfrds_status {
    if server.is_null() || current_pathname.is_null() || new_pathname.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server)
        .server
        .file_rename(&cstr_to_str(current_pathname), &cstr_to_str(new_pathname))
    {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Deletes a remote file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_remove_file(
    server: *mut FfiServer,
    pathname: *const c_char,
) -> cfrds_status {
    if server.is_null() || pathname.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.file_remove_file(&cstr_to_str(pathname)) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Deletes a remote directory.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_remove_dir(
    server: *mut FfiServer,
    path: *const c_char,
) -> cfrds_status {
    if server.is_null() || path.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.file_remove_dir(&cstr_to_str(path)) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Checks if a file or directory exists on the remote server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_exists(
    server: *mut FfiServer,
    pathname: *const c_char,
    out: *mut bool,
) -> cfrds_status {
    if pathname.is_null() || out.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    if server.is_null() {
        return Status::ServerIsNull as cfrds_status;
    }
    match (*server).server.file_exists(&cstr_to_str(pathname)) {
        Ok(exists) => {
            *out = exists;
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

/// Creates a directory on the remote server.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_create_dir(
    server: *mut FfiServer,
    path: *const c_char,
) -> cfrds_status {
    if server.is_null() || path.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.file_create_dir(&cstr_to_str(path)) {
        Ok(()) => Status::Ok as cfrds_status,
        Err(e) => e.status() as cfrds_status,
    }
}

/// Retrieves the root directory path of the ColdFusion installation.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_file_get_root_dir(
    server: *mut FfiServer,
    out: *mut *mut c_char,
) -> cfrds_status {
    if server.is_null() || out.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    match (*server).server.file_get_root_dir() {
        Ok(root) => {
            *out = cstr_alloc(&root);
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// File content accessors
// ---------------------------------------------------------------------------

/// Frees an allocated `cfrds_file_content` structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_free(value: *mut FFileContent) {
    if !value.is_null() {
        drop(Box::from_raw(value));
    }
}

/// Automatically deallocates and nullifies a `cfrds_file_content` pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_cleanup(value: *mut *mut FFileContent) {
    if !value.is_null() {
        cfrds_file_content_free(*value);
        *value = ptr::null_mut();
    }
}

/// Retrieves the raw file data content.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_get_data(value: *const FFileContent) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    (*value).data.as_ptr() as *const c_char
}

/// Retrieves the file size of the read file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_get_size(value: *const FFileContent) -> usize {
    if value.is_null() {
        return 0;
    }
    (*value).size
}

/// Retrieves the remote modification timestamp string.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_get_modified(
    value: *const FFileContent,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    (*value).modified.as_ptr()
}

/// Retrieves the remote permission string of the file.
#[no_mangle]
pub unsafe extern "C" fn cfrds_file_content_get_permission(
    value: *const FFileContent,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    (*value).permission.as_ptr()
}

// ---------------------------------------------------------------------------
// Browse dir accessors
// ---------------------------------------------------------------------------

/// Frees an allocated `cfrds_browse_dir` directory listing structure.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_free(value: *mut FBrowseDir) {
    if !value.is_null() {
        drop(Box::from_raw(value));
    }
}

/// Automatically deallocates and nullifies a `cfrds_browse_dir` pointer.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_cleanup(value: *mut *mut FBrowseDir) {
    if !value.is_null() {
        cfrds_browse_dir_free(*value);
        *value = ptr::null_mut();
    }
}

/// Returns the number of items returned in the directory listing.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_count(value: *const FBrowseDir) -> usize {
    if value.is_null() {
        return 0;
    }
    (*value).items.len()
}

/// Retrieves the item type at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_item_get_kind(
    value: *const FBrowseDir,
    ndx: usize,
) -> c_char {
    if value.is_null() {
        return 0;
    }
    match (&*value).items.get(ndx) {
        Some(item) => item.kind,
        None => 0,
    }
}

/// Retrieves the item name at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_item_get_name(
    value: *const FBrowseDir,
    ndx: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    match (&*value).items.get(ndx) {
        Some(item) => item.name.as_ptr(),
        None => ptr::null(),
    }
}

/// Retrieves the access permissions of the item at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_item_get_permissions(
    value: *const FBrowseDir,
    ndx: usize,
) -> u8 {
    if value.is_null() {
        return 0;
    }
    match (&*value).items.get(ndx) {
        Some(item) => item.permissions,
        None => 0,
    }
}

/// Retrieves the file size in bytes of the item at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_item_get_size(
    value: *const FBrowseDir,
    ndx: usize,
) -> usize {
    if value.is_null() {
        return 0;
    }
    match (&*value).items.get(ndx) {
        Some(item) => item.size,
        None => 0,
    }
}

/// Retrieves the last modified timestamp of the item at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_browse_dir_item_get_modified(
    value: *const FBrowseDir,
    ndx: usize,
) -> u64 {
    if value.is_null() {
        return 0;
    }
    match (&*value).items.get(ndx) {
        Some(item) => item.modified,
        None => 0,
    }
}
