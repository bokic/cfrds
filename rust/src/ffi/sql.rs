//! C-ABI SQL/database commands and result accessors
//! (port of `src/cfrds_sql.c` and the related accessors from `src/cfrds.c`).

use std::ffi::{c_char, c_int, CString};
use std::ptr;

use crate::error::{Error, Status};
use crate::ffi::{cfrds_status, cstr_alloc, cstr_to_str, to_cstr, FfiServer};
use crate::parser;
use crate::server::Server;
use crate::types::{
    ColumnInfo, DsnInfo, KeyInfo, PrimaryKeys, ResultSet, SqlMetadata, SupportedCommands, TableInfo,
};

// ---------------------------------------------------------------------------
// Result handles
// ---------------------------------------------------------------------------

/// Opaque `cfrds_sql_dsninfo` handle.
pub(crate) struct FSqlDsnInfo {
    names: Vec<CString>,
}

/// Opaque `cfrds_sql_tableinfo` handle.
pub(crate) struct FTableInfo {
    items: Vec<FTableInfoItem>,
}

pub(crate) struct FTableInfoItem {
    unknown: CString,
    schema: CString,
    name: CString,
    table_type: CString,
}

/// Opaque `cfrds_sql_columninfo` handle.
pub(crate) struct FColumnInfo {
    items: Vec<FColumnInfoItem>,
}

pub(crate) struct FColumnInfoItem {
    schema: CString,
    owner: CString,
    table: CString,
    name: CString,
    data_type: c_int,
    type_str: CString,
    precision: c_int,
    length: c_int,
    scale: c_int,
    radix: c_int,
    nullable: c_int,
}

/// Opaque `cfrds_sql_primarykeys` handle.
pub(crate) struct FPrimaryKeys {
    items: Vec<FPrimaryKeyItem>,
}

pub(crate) struct FPrimaryKeyItem {
    catalog: CString,
    owner: CString,
    table: CString,
    column: CString,
    key_sequence: c_int,
}

/// Opaque foreign/imported/exported keys handle.
pub(crate) struct FKeyInfo {
    items: Vec<FKeyInfoItem>,
}

pub(crate) struct FKeyInfoItem {
    pk_catalog: CString,
    pk_owner: CString,
    pk_table: CString,
    pk_column: CString,
    fk_catalog: CString,
    fk_owner: CString,
    fk_table: CString,
    fk_column: CString,
    key_sequence: c_int,
    update_rule: c_int,
    delete_rule: c_int,
}

/// Opaque `cfrds_sql_resultset` handle.
pub(crate) struct FResultSet {
    names: Vec<CString>,
    rows: Vec<Vec<Option<CString>>>,
}

/// Opaque `cfrds_sql_metadata` handle.
pub(crate) struct FSqlMetadata {
    items: Vec<FSqlMetadataItem>,
}

pub(crate) struct FSqlMetadataItem {
    name: CString,
    data_type: CString,
    jtype: CString,
}

/// Opaque `cfrds_sql_supportedcommands` handle.
pub(crate) struct FSupportedCommands {
    commands: Vec<CString>,
}

impl From<DsnInfo> for FSqlDsnInfo {
    fn from(info: DsnInfo) -> FSqlDsnInfo {
        FSqlDsnInfo {
            names: info.iter().map(to_cstr).collect(),
        }
    }
}

impl From<TableInfo> for FTableInfo {
    fn from(info: TableInfo) -> FTableInfo {
        FTableInfo {
            items: info
                .iter()
                .map(|i| FTableInfoItem {
                    unknown: to_cstr(&i.unknown),
                    schema: to_cstr(&i.schema),
                    name: to_cstr(&i.name),
                    table_type: to_cstr(&i.table_type),
                })
                .collect(),
        }
    }
}

impl From<ColumnInfo> for FColumnInfo {
    fn from(info: ColumnInfo) -> FColumnInfo {
        FColumnInfo {
            items: info
                .iter()
                .map(|i| FColumnInfoItem {
                    schema: to_cstr(&i.schema),
                    owner: to_cstr(&i.owner),
                    table: to_cstr(&i.table),
                    name: to_cstr(&i.name),
                    data_type: i.data_type,
                    type_str: to_cstr(&i.type_str),
                    precision: i.precision,
                    length: i.length,
                    scale: i.scale,
                    radix: i.radix,
                    nullable: i.nullable,
                })
                .collect(),
        }
    }
}

impl From<PrimaryKeys> for FPrimaryKeys {
    fn from(keys: PrimaryKeys) -> FPrimaryKeys {
        FPrimaryKeys {
            items: keys
                .iter()
                .map(|i| FPrimaryKeyItem {
                    catalog: to_cstr(&i.catalog),
                    owner: to_cstr(&i.owner),
                    table: to_cstr(&i.table),
                    column: to_cstr(&i.column),
                    key_sequence: i.key_sequence,
                })
                .collect(),
        }
    }
}

impl From<KeyInfo> for FKeyInfo {
    fn from(keys: KeyInfo) -> FKeyInfo {
        FKeyInfo {
            items: keys
                .iter()
                .map(|i| FKeyInfoItem {
                    pk_catalog: to_cstr(&i.pk_catalog),
                    pk_owner: to_cstr(&i.pk_owner),
                    pk_table: to_cstr(&i.pk_table),
                    pk_column: to_cstr(&i.pk_column),
                    fk_catalog: to_cstr(&i.fk_catalog),
                    fk_owner: to_cstr(&i.fk_owner),
                    fk_table: to_cstr(&i.fk_table),
                    fk_column: to_cstr(&i.fk_column),
                    key_sequence: i.key_sequence,
                    update_rule: i.update_rule,
                    delete_rule: i.delete_rule,
                })
                .collect(),
        }
    }
}

impl From<ResultSet> for FResultSet {
    fn from(rs: ResultSet) -> FResultSet {
        FResultSet {
            names: rs.column_names().iter().map(|s| to_cstr(s)).collect(),
            rows: rs
                .data_rows()
                .iter()
                .map(|row| {
                    row.iter()
                        .map(|cell| cell.as_deref().map(to_cstr))
                        .collect()
                })
                .collect(),
        }
    }
}

impl From<SqlMetadata> for FSqlMetadata {
    fn from(meta: SqlMetadata) -> FSqlMetadata {
        FSqlMetadata {
            items: meta
                .iter()
                .map(|i| FSqlMetadataItem {
                    name: to_cstr(&i.name),
                    data_type: to_cstr(&i.data_type),
                    jtype: to_cstr(&i.jtype),
                })
                .collect(),
        }
    }
}

impl From<SupportedCommands> for FSupportedCommands {
    fn from(cmds: SupportedCommands) -> FSupportedCommands {
        FSupportedCommands {
            commands: cmds.iter().map(to_cstr).collect(),
        }
    }
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

/// Sends a DBFUNCS command, parses the response and converts it to the FFI
/// handle type.
fn run_sql<T, R>(
    server: &Server,
    params: &[&str],
    parser_fn: fn(&[u8]) -> Option<T>,
) -> Result<R, Error>
where
    R: From<T>,
{
    let response = server.send_command("DBFUNCS", params)?;
    let parsed = parser_fn(&response).ok_or_else(|| {
        server.set_error(Status::ResponseError, "failed to parse DBFUNCS response")
    })?;
    Ok(R::from(parsed))
}

macro_rules! sql_command {
    ($name:ident, $parser:ident, $handle:ty, $op:literal $(, $arg:ident)*) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(
            server: *mut FfiServer,
            $($arg: *const c_char,)*
            out: *mut *mut $handle,
        ) -> cfrds_status {
            if server.is_null() || out.is_null() {
                return Status::ParamIsNull as cfrds_status;
            }
            // The RDS params are: [connection_or_empty, "<OP>", extra_args...].
            let extra: Vec<String> = vec![ $( cstr_to_str($arg), )* ];
            let mut params: Vec<String> = Vec::with_capacity(extra.len() + 2);
            if extra.is_empty() {
                params.push(String::new());
                params.push($op.to_string());
            } else {
                params.push(extra[0].clone());
                params.push($op.to_string());
                params.extend_from_slice(&extra[1..]);
            }
            let refs: Vec<&str> = params.iter().map(|s| s.as_str()).collect();
            match run_sql::<_, $handle>(&(*server).server, &refs, parser::$parser) {
                Ok(handle) => {
                    *out = Box::into_raw(Box::new(handle));
                    Status::Ok as cfrds_status
                }
                Err(e) => e.status() as cfrds_status,
            }
        }
    };
}

sql_command!(
    cfrds_command_sql_dsninfo,
    buffer_to_sql_dsninfo,
    FSqlDsnInfo,
    "DSNINFO"
);
sql_command!(
    cfrds_command_sql_tableinfo,
    buffer_to_sql_tableinfo,
    FTableInfo,
    "TABLEINFO",
    connection_name
);
sql_command!(
    cfrds_command_sql_columninfo,
    buffer_to_sql_columninfo,
    FColumnInfo,
    "COLUMNINFO",
    connection_name,
    table_name
);
sql_command!(
    cfrds_command_sql_primarykeys,
    buffer_to_sql_primarykeys,
    FPrimaryKeys,
    "PRIMARYKEYS",
    connection_name,
    table_name
);
sql_command!(
    cfrds_command_sql_foreignkeys,
    buffer_to_sql_foreignkeys,
    FKeyInfo,
    "FOREIGNKEYS",
    connection_name,
    table_name
);
sql_command!(
    cfrds_command_sql_importedkeys,
    buffer_to_sql_importedkeys,
    FKeyInfo,
    "IMPORTEDKEYS",
    connection_name,
    table_name
);
sql_command!(
    cfrds_command_sql_exportedkeys,
    buffer_to_sql_exportedkeys,
    FKeyInfo,
    "EXPORTEDKEYS",
    connection_name,
    table_name
);
sql_command!(
    cfrds_command_sql_sqlstmnt,
    buffer_to_sql_sqlstmnt,
    FResultSet,
    "SQLSTMNT",
    connection_name,
    sql
);
sql_command!(
    cfrds_command_sql_sqlmetadata,
    buffer_to_sql_metadata,
    FSqlMetadata,
    "SQLMETADATA",
    connection_name,
    sql
);
sql_command!(
    cfrds_command_sql_getsupportedcommands,
    buffer_to_sql_supportedcommands,
    FSupportedCommands,
    "SUPPORTEDCOMMANDS"
);

/// Retrieves the description/version banner of the database engine for a DSN.
#[no_mangle]
pub unsafe extern "C" fn cfrds_command_sql_dbdescription(
    server: *mut FfiServer,
    connection_name: *const c_char,
    description: *mut *mut c_char,
) -> cfrds_status {
    if server.is_null() || description.is_null() {
        return Status::ParamIsNull as cfrds_status;
    }
    let conn = cstr_to_str(connection_name);
    match (*server).server.sql_dbdescription(&conn) {
        Ok(desc) => {
            *description = cstr_alloc(&desc);
            Status::Ok as cfrds_status
        }
        Err(e) => e.status() as cfrds_status,
    }
}

// ---------------------------------------------------------------------------
// Free / cleanup / count
// ---------------------------------------------------------------------------

macro_rules! free_cleanup {
    ($(($free:ident, $cleanup:ident, $handle:ty)),* $(,)?) => {
        $(
            /// Frees an allocated result structure.
            #[no_mangle]
            pub unsafe extern "C" fn $free(value: *mut $handle) {
                if !value.is_null() {
                    drop(Box::from_raw(value));
                }
            }

            /// Automatically deallocates and nullifies a result pointer.
            #[no_mangle]
            pub unsafe extern "C" fn $cleanup(value: *mut *mut $handle) {
                if !value.is_null() {
                    $free(*value);
                    *value = ptr::null_mut();
                }
            }
        )*
    };
}

free_cleanup!(
    (
        cfrds_sql_dsninfo_free,
        cfrds_sql_dsninfo_cleanup,
        FSqlDsnInfo
    ),
    (
        cfrds_sql_tableinfo_free,
        cfrds_sql_tableinfo_cleanup,
        FTableInfo
    ),
    (
        cfrds_sql_columninfo_free,
        cfrds_sql_columninfo_cleanup,
        FColumnInfo
    ),
    (
        cfrds_sql_primarykeys_free,
        cfrds_sql_primarykeys_cleanup,
        FPrimaryKeys
    ),
    (
        cfrds_sql_foreignkeys_free,
        cfrds_sql_foreignkeys_cleanup,
        FKeyInfo
    ),
    (
        cfrds_sql_importedkeys_free,
        cfrds_sql_importedkeys_cleanup,
        FKeyInfo
    ),
    (
        cfrds_sql_exportedkeys_free,
        cfrds_sql_exportedkeys_cleanup,
        FKeyInfo
    ),
    (
        cfrds_sql_resultset_free,
        cfrds_sql_resultset_cleanup,
        FResultSet
    ),
    (
        cfrds_sql_metadata_free,
        cfrds_sql_metadata_cleanup,
        FSqlMetadata
    ),
    (
        cfrds_sql_supportedcommands_free,
        cfrds_sql_supportedcommands_cleanup,
        FSupportedCommands
    ),
);

macro_rules! count_fn {
    ($(($name:ident, $handle:ty, $len:expr)),* $(,)?) => {
        $(
            /// Returns the count of items in the result.
            #[no_mangle]
            pub unsafe extern "C" fn $name(value: *const $handle) -> usize {
                if value.is_null() {
                    return 0;
                }
                $len(value)
            }
        )*
    };
}

fn dsninfo_len(v: *const FSqlDsnInfo) -> usize {
    unsafe { (*v).names.len() }
}
fn tableinfo_len(v: *const FTableInfo) -> usize {
    unsafe { (*v).items.len() }
}
fn columninfo_len(v: *const FColumnInfo) -> usize {
    unsafe { (*v).items.len() }
}
fn primarykeys_len(v: *const FPrimaryKeys) -> usize {
    unsafe { (*v).items.len() }
}
fn keyinfo_len(v: *const FKeyInfo) -> usize {
    unsafe { (*v).items.len() }
}
fn metadata_len(v: *const FSqlMetadata) -> usize {
    unsafe { (*v).items.len() }
}
fn supportedcommands_len(v: *const FSupportedCommands) -> usize {
    unsafe { (*v).commands.len() }
}

count_fn!(
    (cfrds_sql_dsninfo_count, FSqlDsnInfo, dsninfo_len),
    (cfrds_sql_tableinfo_count, FTableInfo, tableinfo_len),
    (cfrds_sql_columninfo_count, FColumnInfo, columninfo_len),
    (cfrds_sql_primarykeys_count, FPrimaryKeys, primarykeys_len),
    (cfrds_sql_foreignkeys_count, FKeyInfo, keyinfo_len),
    (cfrds_sql_importedkeys_count, FKeyInfo, keyinfo_len),
    (cfrds_sql_exportedkeys_count, FKeyInfo, keyinfo_len),
    (cfrds_sql_metadata_count, FSqlMetadata, metadata_len),
    (
        cfrds_sql_supportedcommands_count,
        FSupportedCommands,
        supportedcommands_len
    ),
);

/// Returns the count of columns returned in the resultset.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_resultset_columns(value: *const FResultSet) -> usize {
    if value.is_null() {
        return 0;
    }
    (*value).names.len()
}

/// Returns the count of rows returned in the resultset.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_resultset_rows(value: *const FResultSet) -> usize {
    if value.is_null() {
        return 0;
    }
    (*value).rows.len()
}

/// Retrieves the column header name for a specific column index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_resultset_column_name(
    value: *const FResultSet,
    column: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    match (&*value).names.get(column) {
        Some(name) => name.as_ptr(),
        None => ptr::null(),
    }
}

/// Retrieves a cell value string at a specific row and column coordinate.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_resultset_value(
    value: *const FResultSet,
    row: usize,
    column: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    match (&*value).rows.get(row).and_then(|r| r.get(column)) {
        Some(Some(cell)) => cell.as_ptr(),
        _ => ptr::null(),
    }
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

macro_rules! string_accessor {
    ($name:ident, $handle:ty, $field:ident, $len_fn:ident) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(value: *const $handle, ndx: usize) -> *const c_char {
            if value.is_null() {
                return ptr::null();
            }
            let items = (*value).items.as_ptr();
            if ndx >= $len_fn(value) {
                return ptr::null();
            }
            (*items.add(ndx)).$field.as_ptr()
        }
    };
}

macro_rules! int_accessor {
    ($name:ident, $handle:ty, $field:ident, $len_fn:ident) => {
        #[no_mangle]
        pub unsafe extern "C" fn $name(value: *const $handle, ndx: usize) -> c_int {
            if value.is_null() {
                return -1;
            }
            let items = (*value).items.as_ptr();
            if ndx >= $len_fn(value) {
                return -1;
            }
            (*items.add(ndx)).$field
        }
    };
}

// --- DSN info ---

/// Retrieves the DSN name string at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_dsninfo_item_get_name(
    value: *const FSqlDsnInfo,
    ndx: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    match (&*value).names.get(ndx) {
        Some(name) => name.as_ptr(),
        None => ptr::null(),
    }
}

// --- Table info ---

string_accessor!(
    cfrds_sql_tableinfo_get_column_unknown,
    FTableInfo,
    unknown,
    tableinfo_len
);
string_accessor!(
    cfrds_sql_tableinfo_get_column_schema,
    FTableInfo,
    schema,
    tableinfo_len
);
string_accessor!(
    cfrds_sql_tableinfo_get_column_name,
    FTableInfo,
    name,
    tableinfo_len
);
string_accessor!(
    cfrds_sql_tableinfo_get_column_type,
    FTableInfo,
    table_type,
    tableinfo_len
);

// --- Column info ---

string_accessor!(
    cfrds_sql_columninfo_get_schema,
    FColumnInfo,
    schema,
    columninfo_len
);
string_accessor!(
    cfrds_sql_columninfo_get_owner,
    FColumnInfo,
    owner,
    columninfo_len
);
string_accessor!(
    cfrds_sql_columninfo_get_table,
    FColumnInfo,
    table,
    columninfo_len
);
string_accessor!(
    cfrds_sql_columninfo_get_name,
    FColumnInfo,
    name,
    columninfo_len
);
string_accessor!(
    cfrds_sql_columninfo_get_typeStr,
    FColumnInfo,
    type_str,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_type,
    FColumnInfo,
    data_type,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_precision,
    FColumnInfo,
    precision,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_length,
    FColumnInfo,
    length,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_scale,
    FColumnInfo,
    scale,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_radix,
    FColumnInfo,
    radix,
    columninfo_len
);
int_accessor!(
    cfrds_sql_columninfo_get_nullable,
    FColumnInfo,
    nullable,
    columninfo_len
);

// --- Primary keys ---

string_accessor!(
    cfrds_sql_primarykeys_get_catalog,
    FPrimaryKeys,
    catalog,
    primarykeys_len
);
string_accessor!(
    cfrds_sql_primarykeys_get_owner,
    FPrimaryKeys,
    owner,
    primarykeys_len
);
string_accessor!(
    cfrds_sql_primarykeys_get_table,
    FPrimaryKeys,
    table,
    primarykeys_len
);
string_accessor!(
    cfrds_sql_primarykeys_get_column,
    FPrimaryKeys,
    column,
    primarykeys_len
);
int_accessor!(
    cfrds_sql_primarykeys_get_key_sequence,
    FPrimaryKeys,
    key_sequence,
    primarykeys_len
);

// --- Foreign keys ---

string_accessor!(
    cfrds_sql_foreignkeys_get_pkcatalog,
    FKeyInfo,
    pk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_pkowner,
    FKeyInfo,
    pk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_pktable,
    FKeyInfo,
    pk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_pkcolumn,
    FKeyInfo,
    pk_column,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_fkcatalog,
    FKeyInfo,
    fk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_fkowner,
    FKeyInfo,
    fk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_fktable,
    FKeyInfo,
    fk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_foreignkeys_get_fkcolumn,
    FKeyInfo,
    fk_column,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_foreignkeys_get_key_sequence,
    FKeyInfo,
    key_sequence,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_foreignkeys_get_updaterule,
    FKeyInfo,
    update_rule,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_foreignkeys_get_deleterule,
    FKeyInfo,
    delete_rule,
    keyinfo_len
);

// --- Imported keys ---

string_accessor!(
    cfrds_sql_importedkeys_get_pkcatalog,
    FKeyInfo,
    pk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_pkowner,
    FKeyInfo,
    pk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_pktable,
    FKeyInfo,
    pk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_pkcolumn,
    FKeyInfo,
    pk_column,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_fkcatalog,
    FKeyInfo,
    fk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_fkowner,
    FKeyInfo,
    fk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_fktable,
    FKeyInfo,
    fk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_importedkeys_get_fkcolumn,
    FKeyInfo,
    fk_column,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_importedkeys_get_key_sequence,
    FKeyInfo,
    key_sequence,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_importedkeys_get_updaterule,
    FKeyInfo,
    update_rule,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_importedkeys_get_deleterule,
    FKeyInfo,
    delete_rule,
    keyinfo_len
);

// --- Exported keys ---

string_accessor!(
    cfrds_sql_exportedkeys_get_pkcatalog,
    FKeyInfo,
    pk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_pkowner,
    FKeyInfo,
    pk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_pktable,
    FKeyInfo,
    pk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_pkcolumn,
    FKeyInfo,
    pk_column,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_fkcatalog,
    FKeyInfo,
    fk_catalog,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_fkowner,
    FKeyInfo,
    fk_owner,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_fktable,
    FKeyInfo,
    fk_table,
    keyinfo_len
);
string_accessor!(
    cfrds_sql_exportedkeys_get_fkcolumn,
    FKeyInfo,
    fk_column,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_exportedkeys_get_key_sequence,
    FKeyInfo,
    key_sequence,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_exportedkeys_get_updaterule,
    FKeyInfo,
    update_rule,
    keyinfo_len
);
int_accessor!(
    cfrds_sql_exportedkeys_get_deleterule,
    FKeyInfo,
    delete_rule,
    keyinfo_len
);

// --- Metadata ---

string_accessor!(
    cfrds_sql_metadata_get_name,
    FSqlMetadata,
    name,
    metadata_len
);
string_accessor!(
    cfrds_sql_metadata_get_type,
    FSqlMetadata,
    data_type,
    metadata_len
);
string_accessor!(
    cfrds_sql_metadata_get_jtype,
    FSqlMetadata,
    jtype,
    metadata_len
);

/// Retrieves the command name string at a specific index.
#[no_mangle]
pub unsafe extern "C" fn cfrds_sql_supportedcommands_get(
    value: *const FSupportedCommands,
    ndx: usize,
) -> *const c_char {
    if value.is_null() {
        return ptr::null();
    }
    match (&*value).commands.get(ndx) {
        Some(cmd) => cmd.as_ptr(),
        None => ptr::null(),
    }
}
