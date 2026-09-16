//! Public result types returned by RDS command functions.
//!
//! These mirror the result structs from the C library (`include/cfrds.h` and
//! `include/internal/cfrds_buffer.h`).

/// A single entry in a remote directory listing.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BrowseDirItem {
    /// Item kind: `'F'` for a file, `'D'` for a directory.
    pub kind: char,
    /// Item name.
    pub name: String,
    /// Permission bitmask (maps to FILE_ATTRIBUTE_* values).
    pub permissions: u8,
    /// File size in bytes.
    pub size: usize,
    /// Last modified time as Unix milliseconds since the epoch.
    pub modified: u64,
}

/// A remote directory listing result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BrowseDir {
    items: Vec<BrowseDirItem>,
}

impl BrowseDir {
    /// Returns the number of items in the listing.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the item at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&BrowseDirItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the listing items.
    pub fn iter(&self) -> impl Iterator<Item = &BrowseDirItem> {
        self.items.iter()
    }

    /// Returns the backing slice of items.
    pub fn items(&self) -> &[BrowseDirItem] {
        &self.items
    }
}

impl From<Vec<BrowseDirItem>> for BrowseDir {
    fn from(items: Vec<BrowseDirItem>) -> BrowseDir {
        BrowseDir { items }
    }
}

/// The content of a remote file.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FileContent {
    data: Vec<u8>,
    size: usize,
    modified: String,
    permission: String,
}

impl FileContent {
    /// Returns the raw file bytes.
    pub fn data(&self) -> &[u8] {
        &self.data
    }

    /// Returns the file size in bytes.
    pub fn size(&self) -> usize {
        self.size
    }

    /// Returns the remote modification timestamp string.
    pub fn modified(&self) -> &str {
        &self.modified
    }

    /// Returns the remote permission string.
    pub fn permission(&self) -> &str {
        &self.permission
    }

    /// Converts the raw bytes to a UTF-8 string (lossy).
    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(&self.data).into_owned()
    }
}

impl From<(Vec<u8>, usize, String, String)> for FileContent {
    fn from(v: (Vec<u8>, usize, String, String)) -> FileContent {
        FileContent {
            data: v.0,
            size: v.1,
            modified: v.2,
            permission: v.3,
        }
    }
}

/// The list of configured Data Source Names (DSN).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DsnInfo {
    names: Vec<String>,
}

impl DsnInfo {
    /// Returns the number of DSNs.
    pub fn count(&self) -> usize {
        self.names.len()
    }

    /// Returns the DSN name at `ndx`, or `None` if out of bounds.
    pub fn name(&self, ndx: usize) -> Option<&str> {
        self.names.get(ndx).map(|s| s.as_str())
    }

    /// Returns an iterator over the DSN names.
    pub fn iter(&self) -> impl Iterator<Item = &str> {
        self.names.iter().map(|s| s.as_str())
    }
}

impl From<Vec<String>> for DsnInfo {
    fn from(names: Vec<String>) -> DsnInfo {
        DsnInfo { names }
    }
}

/// A single table metadata row.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableInfoItem {
    /// Unknown first column attribute.
    pub unknown: String,
    /// Schema name.
    pub schema: String,
    /// Table name.
    pub name: String,
    /// Table type (e.g. "TABLE", "VIEW").
    pub table_type: String,
}

/// A database table listing result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TableInfo {
    items: Vec<TableInfoItem>,
}

impl TableInfo {
    /// Returns the number of tables.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the table metadata at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&TableInfoItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the tables.
    pub fn iter(&self) -> impl Iterator<Item = &TableInfoItem> {
        self.items.iter()
    }
}

impl From<Vec<TableInfoItem>> for TableInfo {
    fn from(items: Vec<TableInfoItem>) -> TableInfo {
        TableInfo { items }
    }
}

/// A single column metadata row.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ColumnInfoItem {
    /// Schema name.
    pub schema: String,
    /// Owner name.
    pub owner: String,
    /// Table name.
    pub table: String,
    /// Column name.
    pub name: String,
    /// JDBC data type integer.
    pub data_type: i32,
    /// Data type name string (e.g. "VARCHAR").
    pub type_str: String,
    /// Numeric precision.
    pub precision: i32,
    /// Length limit.
    pub length: i32,
    /// Decimal scale.
    pub scale: i32,
    /// Numeric radix.
    pub radix: i32,
    /// Nullability (1 if nullable, 0 otherwise).
    pub nullable: i32,
}

/// A database column metadata result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ColumnInfo {
    items: Vec<ColumnInfoItem>,
}

impl ColumnInfo {
    /// Returns the number of columns.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the column metadata at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&ColumnInfoItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the columns.
    pub fn iter(&self) -> impl Iterator<Item = &ColumnInfoItem> {
        self.items.iter()
    }
}

impl From<Vec<ColumnInfoItem>> for ColumnInfo {
    fn from(items: Vec<ColumnInfoItem>) -> ColumnInfo {
        ColumnInfo { items }
    }
}

/// A single primary key column descriptor.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PrimaryKeyItem {
    /// Catalog name.
    pub catalog: String,
    /// Owner name.
    pub owner: String,
    /// Table name.
    pub table: String,
    /// Column name.
    pub column: String,
    /// Key sequence inside a composite key.
    pub key_sequence: i32,
}

/// A primary key result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PrimaryKeys {
    items: Vec<PrimaryKeyItem>,
}

impl PrimaryKeys {
    /// Returns the number of primary keys.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the primary key at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&PrimaryKeyItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the primary keys.
    pub fn iter(&self) -> impl Iterator<Item = &PrimaryKeyItem> {
        self.items.iter()
    }
}

impl From<Vec<PrimaryKeyItem>> for PrimaryKeys {
    fn from(items: Vec<PrimaryKeyItem>) -> PrimaryKeys {
        PrimaryKeys { items }
    }
}

/// A single foreign/imported/exported key descriptor.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct KeyInfoItem {
    /// Referenced (primary) key catalog name.
    pub pk_catalog: String,
    /// Referenced (primary) key owner name.
    pub pk_owner: String,
    /// Referenced (primary) key table name.
    pub pk_table: String,
    /// Referenced (primary) key column name.
    pub pk_column: String,
    /// Foreign key catalog name.
    pub fk_catalog: String,
    /// Foreign key owner name.
    pub fk_owner: String,
    /// Foreign key table name.
    pub fk_table: String,
    /// Foreign key column name.
    pub fk_column: String,
    /// Key sequence inside a composite key.
    pub key_sequence: i32,
    /// Update rule value.
    pub update_rule: i32,
    /// Delete rule value.
    pub delete_rule: i32,
}

/// A foreign/imported/exported key result.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct KeyInfo {
    items: Vec<KeyInfoItem>,
}

impl KeyInfo {
    /// Returns the number of keys.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the key at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&KeyInfoItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the keys.
    pub fn iter(&self) -> impl Iterator<Item = &KeyInfoItem> {
        self.items.iter()
    }
}

impl From<Vec<KeyInfoItem>> for KeyInfo {
    fn from(items: Vec<KeyInfoItem>) -> KeyInfo {
        KeyInfo { items }
    }
}

/// A SQL resultset grid.
///
/// `rows` holds data rows; cells are `Option<String>` (`None` represents a SQL
/// NULL value). Column names are available via `column_names()`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ResultSet {
    names: Vec<String>,
    rows: Vec<Vec<Option<String>>>,
}

impl ResultSet {
    /// Returns the number of columns.
    pub fn columns(&self) -> usize {
        self.names.len()
    }

    /// Returns the number of data rows.
    pub fn rows(&self) -> usize {
        self.rows.len()
    }

    /// Returns the column name at `column`, or `None` if out of bounds.
    pub fn column_name(&self, column: usize) -> Option<&str> {
        self.names.get(column).map(|s| s.as_str())
    }

    /// Returns the column names.
    pub fn column_names(&self) -> &[String] {
        &self.names
    }

    /// Returns the cell at `(row, column)`, or `None` if out of bounds.
    pub fn value(&self, row: usize, column: usize) -> Option<Option<&str>> {
        self.rows
            .get(row)
            .and_then(|r| r.get(column))
            .map(|v| v.as_deref())
    }

    /// Returns the data rows.
    pub fn data_rows(&self) -> &[Vec<Option<String>>] {
        &self.rows
    }
}

impl From<(Vec<String>, Vec<Vec<Option<String>>>)> for ResultSet {
    fn from(v: (Vec<String>, Vec<Vec<Option<String>>>)) -> ResultSet {
        ResultSet {
            names: v.0,
            rows: v.1,
        }
    }
}

/// A single SQL metadata column descriptor.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SqlMetadataItem {
    /// Column name.
    pub name: String,
    /// Database type name.
    pub data_type: String,
    /// Java class name.
    pub jtype: String,
}

/// SQL statement column metadata.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SqlMetadata {
    items: Vec<SqlMetadataItem>,
}

impl SqlMetadata {
    /// Returns the number of described columns.
    pub fn count(&self) -> usize {
        self.items.len()
    }

    /// Returns the metadata at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&SqlMetadataItem> {
        self.items.get(ndx)
    }

    /// Returns an iterator over the metadata.
    pub fn iter(&self) -> impl Iterator<Item = &SqlMetadataItem> {
        self.items.iter()
    }
}

impl From<Vec<SqlMetadataItem>> for SqlMetadata {
    fn from(items: Vec<SqlMetadataItem>) -> SqlMetadata {
        SqlMetadata { items }
    }
}

/// The list of SQL commands supported by a database server.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SupportedCommands {
    commands: Vec<String>,
}

impl SupportedCommands {
    /// Returns the number of supported commands.
    pub fn count(&self) -> usize {
        self.commands.len()
    }

    /// Returns the command at `ndx`, or `None` if out of bounds.
    pub fn get(&self, ndx: usize) -> Option<&str> {
        self.commands.get(ndx).map(|s| s.as_str())
    }

    /// Returns an iterator over the commands.
    pub fn iter(&self) -> impl Iterator<Item = &str> {
        self.commands.iter().map(|s| s.as_str())
    }
}

impl From<Vec<String>> for SupportedCommands {
    fn from(commands: Vec<String>) -> SupportedCommands {
        SupportedCommands { commands }
    }
}

/// A debugger event type classification (mirrors `cfrds_debugger_type`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum DebuggerEventType {
    BreakpointSet = 0,
    Breakpoint = 1,
    Step = 2,
    Unknown = 3,
}

/// A debugger session event. Exposes typed accessors over the underlying WDDX
/// payload returned by the server.
#[derive(Debug, Clone, PartialEq)]
pub struct DebuggerEvent {
    pub(crate) wddx: crate::wddx::Wddx,
}

impl DebuggerEvent {
    /// Returns the raw WDDX packet backing this event.
    pub fn wddx(&self) -> &crate::wddx::Wddx {
        &self.wddx
    }
}

impl From<crate::wddx::Wddx> for DebuggerEvent {
    fn from(wddx: crate::wddx::Wddx) -> DebuggerEvent {
        DebuggerEvent { wddx }
    }
}

/// The security analyzer scan result (JSON payload from the server).
#[derive(Debug, Clone, PartialEq)]
pub struct SecurityAnalyzerResult {
    pub(crate) json: serde_json::Value,
}

impl SecurityAnalyzerResult {
    /// Returns the underlying JSON value.
    pub fn json(&self) -> &serde_json::Value {
        &self.json
    }
}

impl From<serde_json::Value> for SecurityAnalyzerResult {
    fn from(json: serde_json::Value) -> SecurityAnalyzerResult {
        SecurityAnalyzerResult { json }
    }
}

/// The result of an `ide_default` handshake query.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct IdeDefaultResult {
    /// Handshake check integer 1.
    pub num1: i32,
    /// Server product description.
    pub server_version: String,
    /// Client API version details.
    pub client_version: String,
    /// Handshake check integer 2.
    pub num2: i32,
    /// Handshake check integer 3.
    pub num3: i32,
}

/// The progress/status result of a security analyzer scan.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SecurityAnalyzerStatus {
    /// Total files in scope.
    pub totalfiles: i64,
    /// Completed files count.
    pub filesvisitedcount: i64,
    /// Progress percentage.
    pub percentage: i64,
    /// Last state update timestamp.
    pub lastupdated: i64,
}

/// Custom tag paths configured on the ColdFusion server.
#[derive(Debug, Clone, PartialEq)]
pub struct AdminApiCustomTagPaths {
    pub(crate) wddx: crate::wddx::Wddx,
}

impl AdminApiCustomTagPaths {
    /// Returns the count of custom tag paths.
    pub fn count(&self) -> usize {
        use crate::wddx::WddxType;
        match self.wddx.data() {
            Some(node) if node.node_type() == WddxType::Array => node.node_array_size(),
            _ => 0,
        }
    }

    /// Returns the path at `ndx`, or `None` if out of bounds or not a string.
    pub fn get(&self, ndx: usize) -> Option<&str> {
        use crate::wddx::WddxType;
        let data = self.wddx.data()?;
        if data.node_type() != WddxType::Array {
            return None;
        }
        let item = data.node_array_at(ndx)?;
        item.node_string()
    }

    /// Returns an iterator over the paths.
    pub fn iter(&self) -> impl Iterator<Item = &str> {
        use crate::wddx::WddxType;
        let data = self.wddx.data();
        let items: Vec<&str> = match data {
            Some(node) if node.node_type() == WddxType::Array => (0..node.node_array_size())
                .filter_map(|i| node.node_array_at(i).and_then(|n| n.node_string()))
                .collect(),
            _ => Vec::new(),
        };
        items.into_iter()
    }
}

impl From<crate::wddx::Wddx> for AdminApiCustomTagPaths {
    fn from(wddx: crate::wddx::Wddx) -> AdminApiCustomTagPaths {
        AdminApiCustomTagPaths { wddx }
    }
}

/// Logical mappings configured on the ColdFusion server.
#[derive(Debug, Clone, PartialEq)]
pub struct AdminApiMappings {
    pub(crate) wddx: crate::wddx::Wddx,
}

impl AdminApiMappings {
    /// Returns the count of mappings.
    pub fn count(&self) -> usize {
        use crate::wddx::WddxType;
        match self.wddx.data() {
            Some(node) if node.node_type() == WddxType::Struct => node.node_struct_size(),
            _ => 0,
        }
    }

    /// Returns the logical mapping key at `ndx`, or `None` if out of bounds.
    pub fn key(&self, ndx: usize) -> Option<&str> {
        use crate::wddx::WddxType;
        let data = self.wddx.data()?;
        if data.node_type() != WddxType::Struct {
            return None;
        }
        data.node_struct_at(ndx).map(|(k, _)| k)
    }

    /// Returns the physical path value at `ndx`, or `None` if out of bounds.
    pub fn value(&self, ndx: usize) -> Option<&str> {
        use crate::wddx::WddxType;
        let data = self.wddx.data()?;
        if data.node_type() != WddxType::Struct {
            return None;
        }
        let (_, v) = data.node_struct_at(ndx)?;
        v.node_string()
    }

    /// Returns an iterator over `(key, value)` mapping pairs.
    pub fn iter(&self) -> impl Iterator<Item = (&str, &str)> {
        use crate::wddx::WddxType;
        let data = self.wddx.data();
        let pairs: Vec<(&str, &str)> = match data {
            Some(node) if node.node_type() == WddxType::Struct => (0..node.node_struct_size())
                .filter_map(|i| {
                    node.node_struct_at(i)
                        .and_then(|(k, v)| v.node_string().map(|vs| (k, vs)))
                })
                .collect(),
            _ => Vec::new(),
        };
        pairs.into_iter()
    }
}

impl From<crate::wddx::Wddx> for AdminApiMappings {
    fn from(wddx: crate::wddx::Wddx) -> AdminApiMappings {
        AdminApiMappings { wddx }
    }
}
