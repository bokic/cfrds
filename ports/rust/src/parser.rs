//! RDS response body parsers.
//!
//! Port of the `cfrds_buffer_to_*` functions from `src/cfrds_buffer.c`.

use crate::buffer::Parser;
use crate::types::*;
use crate::wddx::Wddx;

/// Maximum number of items accepted by the parsers (mirrors `CFRDS_MAX_PARSER_ITEMS`).
const CFRDS_MAX_PARSER_ITEMS: i64 = 1_000_000;

/// Decodes raw response bytes into a lossy UTF-8 string.
fn to_str(bytes: Vec<u8>) -> String {
    String::from_utf8_lossy(&bytes).into_owned()
}

/// Parses a fully-consumed base-10 integer from response bytes.
fn parse_int(bytes: &[u8]) -> Option<i64> {
    let s = std::str::from_utf8(bytes).ok()?;
    if s.is_empty() {
        return None;
    }
    s.parse().ok()
}

/// Parses a file/directory listing response.
pub fn buffer_to_browse_dir(data: &[u8]) -> Option<BrowseDir> {
    let mut p = Parser::new(data);
    let total = p.parse_number().ok()?;
    if total < 0 || total % 5 != 0 || total > CFRDS_MAX_PARSER_ITEMS * 5 {
        return None;
    }

    let item_count = (total / 5) as usize;
    let mut items = Vec::with_capacity(item_count);
    for _ in 0..item_count {
        let str_kind = p.parse_bytes().ok()?;
        let filename = p.parse_bytes().ok()?;
        let str_permissions = p.parse_bytes().ok()?;
        let str_filesize = p.parse_bytes().ok()?;
        let str_timestamp = p.parse_bytes().ok()?;

        let kind = match str_kind.as_slice() {
            b"F:" | b"F" => 'F',
            b"D:" | b"D" => 'D',
            _ => '\0',
        };
        let permissions = parse_int(&str_permissions)?;
        let filesize = parse_int(&str_filesize)?;
        let modified = parse_timestamp(&str_timestamp)?;

        if (kind != 'D' && kind != 'F') || !(0..=0xff).contains(&permissions) || filesize < 0 {
            return None;
        }

        items.push(BrowseDirItem {
            kind,
            name: to_str(filename),
            permissions: permissions as u8,
            size: filesize as usize,
            modified,
        });
    }

    p.expect_end().ok()?;
    Some(BrowseDir::from(items))
}

/// Parses the ColdFusion ticks timestamp `n1,n2` into Unix milliseconds.
fn parse_timestamp(bytes: &[u8]) -> Option<u64> {
    let s = std::str::from_utf8(bytes).ok()?;
    let (n1s, n2s) = s.split_once(',')?;
    let n1: i64 = n1s.parse().ok()?;
    let n2: i64 = n2s.parse().ok()?;

    let num1 = (n1 as u64) as u32;
    let num2 = n2 as u64;
    let mut modified = num1 as u64 + (num2 << 32);
    modified /= 10000;
    modified = modified.wrapping_sub(11644473600000);
    Some(modified)
}

/// Parses a file content response.
pub fn buffer_to_file_content(data: &[u8]) -> Option<FileContent> {
    let mut p = Parser::new(data);
    let total = p.parse_number().ok()?;
    if total != 3 {
        return None;
    }

    let content = p.parse_bytes().ok()?;
    let modified = p.parse_bytes().ok()?;
    let permission = p.parse_bytes().ok()?;
    p.expect_end().ok()?;

    let size = content.len();
    Some(FileContent::from((
        content,
        size,
        to_str(modified),
        to_str(permission),
    )))
}

/// Parses a DSN list response.
pub fn buffer_to_sql_dsninfo(data: &[u8]) -> Option<DsnInfo> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }

    let mut names = Vec::with_capacity(cnt as usize);
    for _ in 0..cnt {
        let item = p.parse_bytes().ok()?;
        let mut item_parser = Parser::new(&item);
        let name = item_parser.parse_string_list_item().ok()?;
        names.push(name);
    }

    p.expect_end().ok()?;
    Some(DsnInfo::from(names))
}

/// Parses a database table metadata response.
pub fn buffer_to_sql_tableinfo(data: &[u8]) -> Option<TableInfo> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }

    let mut items = Vec::with_capacity(cnt as usize);
    for _ in 0..cnt {
        let item = p.parse_bytes().ok()?;
        let mut q = Parser::new(&item);

        let unknown = q.parse_string_list_item().ok()?;
        let schema = q.parse_string_list_item().ok()?;
        let name = q.parse_string_list_item().ok()?;
        let table_type = q.parse_string_list_item().ok()?;
        q.expect_end().ok()?;

        items.push(TableInfoItem {
            unknown,
            schema,
            name,
            table_type,
        });
    }

    p.expect_end().ok()?;
    Some(TableInfo::from(items))
}

/// Parses a database column metadata response.
pub fn buffer_to_sql_columninfo(data: &[u8]) -> Option<ColumnInfo> {
    let mut p = Parser::new(data);
    let columns = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&columns) {
        return None;
    }

    let mut items = Vec::with_capacity(columns as usize);
    for _ in 0..columns {
        let row = p.parse_bytes().ok()?;
        let mut q = Parser::new(&row);

        let schema = q.parse_string_list_item().ok()?;
        let owner = q.parse_string_list_item().ok()?;
        let table = q.parse_string_list_item().ok()?;
        let name = q.parse_string_list_item().ok()?;
        let type_str_num = q.parse_string_list_item().ok()?;
        let type_str = q.parse_string_list_item().ok()?;
        let precision = q.parse_string_list_item().ok()?;
        let length = q.parse_string_list_item().ok()?;
        let scale = q.parse_string_list_item().ok()?;
        let radix = q.parse_string_list_item().ok()?;
        let nullable = q.parse_string_list_item().ok()?;

        if q.remaining() > 0 {
            q.parse_string_list_item().ok()?;
        }
        q.expect_end().ok()?;

        items.push(ColumnInfoItem {
            schema,
            owner,
            table,
            name,
            data_type: parse_int(type_str_num.as_bytes()).unwrap_or(0) as i32,
            type_str,
            precision: parse_int(precision.as_bytes()).unwrap_or(0) as i32,
            length: parse_int(length.as_bytes()).unwrap_or(0) as i32,
            scale: parse_int(scale.as_bytes()).unwrap_or(0) as i32,
            radix: parse_int(radix.as_bytes()).unwrap_or(0) as i32,
            nullable: parse_int(nullable.as_bytes()).unwrap_or(0) as i32,
        });
    }

    p.expect_end().ok()?;
    Some(ColumnInfo::from(items))
}

/// Parses a primary keys metadata response.
pub fn buffer_to_sql_primarykeys(data: &[u8]) -> Option<PrimaryKeys> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }

    let mut items = Vec::with_capacity(cnt as usize);
    for _ in 0..cnt {
        let item = p.parse_bytes().ok()?;
        let mut q = Parser::new(&item);

        let catalog = q.parse_string_list_item().ok()?;
        let owner = q.parse_string_list_item().ok()?;
        let table = q.parse_string_list_item().ok()?;
        let column = q.parse_string_list_item().ok()?;
        let key_sequence = q.parse_string_list_item().ok()?;
        q.expect_end().ok()?;

        items.push(PrimaryKeyItem {
            catalog,
            owner,
            table,
            column,
            key_sequence: parse_int(key_sequence.as_bytes()).unwrap_or(0) as i32,
        });
    }

    p.expect_end().ok()?;
    Some(PrimaryKeys::from(items))
}

/// Parses a foreign/imported/exported keys metadata response.
fn buffer_to_sql_keyinfo(data: &[u8]) -> Option<KeyInfo> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }

    let mut items = Vec::with_capacity(cnt as usize);
    for _ in 0..cnt {
        let item = p.parse_bytes().ok()?;
        let mut q = Parser::new(&item);

        let pk_catalog = q.parse_string_list_item().ok()?;
        let pk_owner = q.parse_string_list_item().ok()?;
        let pk_table = q.parse_string_list_item().ok()?;
        let pk_column = q.parse_string_list_item().ok()?;
        let fk_catalog = q.parse_string_list_item().ok()?;
        let fk_owner = q.parse_string_list_item().ok()?;
        let fk_table = q.parse_string_list_item().ok()?;
        let fk_column = q.parse_string_list_item().ok()?;
        let key_sequence = q.parse_string_list_item().ok()?;
        let update_rule = q.parse_string_list_item().ok()?;
        let delete_rule = q.parse_string_list_item().ok()?;
        q.expect_end().ok()?;

        items.push(KeyInfoItem {
            pk_catalog,
            pk_owner,
            pk_table,
            pk_column,
            fk_catalog,
            fk_owner,
            fk_table,
            fk_column,
            key_sequence: parse_int(key_sequence.as_bytes()).unwrap_or(0) as i32,
            update_rule: parse_int(update_rule.as_bytes()).unwrap_or(0) as i32,
            delete_rule: parse_int(delete_rule.as_bytes()).unwrap_or(0) as i32,
        });
    }

    p.expect_end().ok()?;
    Some(KeyInfo::from(items))
}

/// Parses a foreign keys metadata response.
pub fn buffer_to_sql_foreignkeys(data: &[u8]) -> Option<KeyInfo> {
    buffer_to_sql_keyinfo(data)
}

/// Parses an imported keys metadata response.
pub fn buffer_to_sql_importedkeys(data: &[u8]) -> Option<KeyInfo> {
    buffer_to_sql_keyinfo(data)
}

/// Parses an exported keys metadata response.
pub fn buffer_to_sql_exportedkeys(data: &[u8]) -> Option<KeyInfo> {
    buffer_to_sql_keyinfo(data)
}

/// Parses a SQL statement resultset response.
pub fn buffer_to_sql_sqlstmnt(data: &[u8]) -> Option<ResultSet> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(1..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }
    let rows = (cnt - 1) as usize;

    let col_row = p.parse_bytes().ok()?;

    let mut counter = Parser::new(&col_row);
    let mut cols = 0usize;
    while counter.remaining() > 0 {
        counter.parse_string_list_item().ok()?;
        cols += 1;
    }
    if cols < 1 {
        return None;
    }

    let mut header = Parser::new(&col_row);
    let mut names = Vec::with_capacity(cols);
    for _ in 0..cols {
        names.push(header.parse_string_list_item().ok()?);
    }

    let mut data_rows = Vec::with_capacity(rows);
    for _ in 0..rows {
        let row = p.parse_bytes().ok()?;
        let mut row_parser = Parser::new(&row);
        let mut cells = Vec::with_capacity(cols);
        for _ in 0..cols {
            cells.push(Some(row_parser.parse_string_list_item().ok()?));
        }
        row_parser.expect_end().ok()?;
        data_rows.push(cells);
    }

    p.expect_end().ok()?;
    Some(ResultSet::from((names, data_rows)))
}

/// Parses a SQL metadata response.
pub fn buffer_to_sql_metadata(data: &[u8]) -> Option<SqlMetadata> {
    let mut p = Parser::new(data);
    let cnt = p.parse_number().ok()?;
    if !(0..=CFRDS_MAX_PARSER_ITEMS).contains(&cnt) {
        return None;
    }

    let mut items = Vec::with_capacity(cnt as usize);
    for _ in 0..cnt {
        let row = p.parse_bytes().ok()?;
        let mut q = Parser::new(&row);

        let name = q.parse_string_list_item().ok()?;
        let data_type = q.parse_string_list_item().ok()?;
        let jtype = q.parse_string_list_item().ok()?;
        q.expect_end().ok()?;

        items.push(SqlMetadataItem {
            name,
            data_type,
            jtype,
        });
    }

    p.expect_end().ok()?;
    Some(SqlMetadata::from(items))
}

/// Parses a supported commands response.
pub fn buffer_to_sql_supportedcommands(data: &[u8]) -> Option<SupportedCommands> {
    let mut p = Parser::new(data);
    let rows = p.parse_number().ok()?;
    if rows != 1 {
        return None;
    }

    let commands_str = p.parse_bytes().ok()?;
    p.expect_end().ok()?;

    let mut counter = Parser::new(&commands_str);
    let mut cnt = 0usize;
    while counter.remaining() > 0 {
        counter.parse_string_list_item().ok()?;
        cnt += 1;
    }

    let mut q = Parser::new(&commands_str);
    let mut commands = Vec::with_capacity(cnt);
    for _ in 0..cnt {
        commands.push(q.parse_string_list_item().ok()?);
    }

    Some(SupportedCommands::from(commands))
}

/// Parses a database description response.
pub fn buffer_to_sql_dbdescription(data: &[u8]) -> Option<String> {
    let mut p = Parser::new(data);
    let rows = p.parse_number().ok()?;
    if rows != 1 {
        return None;
    }

    let row = p.parse_bytes().ok()?;
    p.expect_end().ok()?;

    let mut q = Parser::new(&row);
    let description = q.parse_string_list_item().ok()?;
    q.expect_end().ok()?;

    Some(description)
}

/// Parses a debugger start response (returns the second row as session data).
pub fn buffer_to_debugger_start(data: &[u8]) -> Option<String> {
    let mut p = Parser::new(data);
    let rows = p.parse_number().ok()?;
    if rows != 2 {
        return None;
    }
    p.parse_bytes().ok().map(to_str)
}

/// Parses a debugger stop response (returns `true` when status is `RDS_OK`).
pub fn buffer_to_debugger_stop(data: &[u8]) -> bool {
    let mut p = Parser::new(data);
    let rows = match p.parse_number() {
        Ok(n) => n,
        Err(_) => return false,
    };
    if rows != 1 {
        return false;
    }
    let xml = match p.parse_bytes() {
        Ok(b) => b,
        Err(_) => return false,
    };
    let xml = to_str(xml);
    let wddx = match Wddx::from_xml(&xml) {
        Some(w) => w,
        None => return false,
    };
    match wddx.get_string("0,STATUS") {
        Some(status) => status == "RDS_OK",
        None => false,
    }
}

/// Parses a debugger server info response (returns the debug port).
pub fn buffer_to_debugger_info(data: &[u8]) -> Option<i32> {
    let mut p = Parser::new(data);
    let rows = p.parse_number().ok()?;
    if rows != 1 {
        return None;
    }
    let xml = p.parse_bytes().ok()?;
    let xml = to_str(xml);
    let wddx = Wddx::from_xml(&xml)?;
    let status = wddx.get_string("0,STATUS")?;
    if status != "RDS_OK" {
        return None;
    }
    let port = wddx.get_number("0,DEBUG_SERVER_PORT")?;
    Some(port as i32)
}

/// Parses a debugger events response into a `Wddx` packet.
pub fn buffer_to_debugger_event(data: &[u8]) -> Option<Wddx> {
    let mut p = Parser::new(data);
    let rows = p.parse_number().ok()?;
    if rows != 1 {
        return None;
    }
    let xml = p.parse_bytes().ok()?;
    let xml = to_str(xml);
    Wddx::from_xml(&xml)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Builds an RDS response body: `<count>:` followed by concatenated
    /// `<len>:<data>` fields (the wire format used by the server).
    fn rds(fields: &[&str]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(format!("{}:", fields.len()).as_bytes());
        for f in fields {
            out.extend_from_slice(format!("{}:", f.len()).as_bytes());
            out.extend_from_slice(f.as_bytes());
        }
        out
    }

    #[test]
    fn browse_dir_response() {
        // ticks = 133485427200000000 -> 2024-01-01 (Unix ms 1704069120000)
        let data = rds(&["F", "etc", "32", "2048", "4008869888,31079497"]);
        let dir = buffer_to_browse_dir(&data).expect("should parse");
        assert_eq!(dir.count(), 1);
        let item = dir.get(0).unwrap();
        assert_eq!(item.kind, 'F');
        assert_eq!(item.name, "etc");
        assert_eq!(item.permissions, 32);
        assert_eq!(item.size, 2048);
        assert_eq!(item.modified, 1704069120000);
    }

    #[test]
    fn browse_dir_accepts_colon_suffixed_kind() {
        let data = rds(&["D:", "sub", "16", "0", "0,0"]);
        let dir = buffer_to_browse_dir(&data).expect("should parse");
        assert_eq!(dir.get(0).unwrap().kind, 'D');
        assert_eq!(dir.get(0).unwrap().name, "sub");
    }

    #[test]
    fn file_content_response() {
        let data = rds(&["hello", "mod", "rw-rw-rw-"]);
        let fc = buffer_to_file_content(&data).expect("should parse");
        assert_eq!(fc.data(), b"hello");
        assert_eq!(fc.size(), 5);
        assert_eq!(fc.modified(), "mod");
        assert_eq!(fc.permission(), "rw-rw-rw-");
    }

    #[test]
    fn dsninfo_response() {
        let data = rds(&["\"dsn1\",", "\"dsn2\","]);
        let info = buffer_to_sql_dsninfo(&data).expect("should parse");
        assert_eq!(info.count(), 2);
        assert_eq!(info.name(0), Some("dsn1"));
        assert_eq!(info.name(1), Some("dsn2"));
    }

    #[test]
    fn tableinfo_response() {
        let data = rds(&["\"\",\"public\",\"users\",\"TABLE\""]);
        let info = buffer_to_sql_tableinfo(&data).expect("should parse");
        assert_eq!(info.count(), 1);
        let item = info.get(0).unwrap();
        assert_eq!(item.schema, "public");
        assert_eq!(item.name, "users");
        assert_eq!(item.table_type, "TABLE");
    }

    #[test]
    fn columninfo_response() {
        let data =
            rds(&["\"\",\"\",\"users\",\"id\",\"4\",\"INTEGER\",\"10\",\"10\",\"0\",\"10\",\"0\""]);
        let info = buffer_to_sql_columninfo(&data).expect("should parse");
        assert_eq!(info.count(), 1);
        let item = info.get(0).unwrap();
        assert_eq!(item.name, "id");
        assert_eq!(item.data_type, 4);
        assert_eq!(item.type_str, "INTEGER");
        assert_eq!(item.precision, 10);
    }

    #[test]
    fn primarykeys_response() {
        let data = rds(&["\"\",\"\",\"users\",\"id\",\"1\""]);
        let info = buffer_to_sql_primarykeys(&data).expect("should parse");
        assert_eq!(info.count(), 1);
        let item = info.get(0).unwrap();
        assert_eq!(item.table, "users");
        assert_eq!(item.column, "id");
        assert_eq!(item.key_sequence, 1);
    }

    #[test]
    fn keyinfo_response() {
        let data =
            rds(&["\"\",\"\",\"users\",\"id\",\"\",\"\",\"orders\",\"user_id\",\"1\",\"1\",\"3\""]);
        let info = buffer_to_sql_foreignkeys(&data).expect("should parse");
        assert_eq!(info.count(), 1);
        let item = info.get(0).unwrap();
        assert_eq!(item.pk_table, "users");
        assert_eq!(item.fk_table, "orders");
        assert_eq!(item.fk_column, "user_id");
        assert_eq!(item.update_rule, 1);
        assert_eq!(item.delete_rule, 3);
    }

    #[test]
    fn sql_resultset_response() {
        // Quoted values, as real server responses use.
        let data = rds(&["\"a\",\"b\"", "\"1\",\"2\"", "\"3\",\"4\""]);
        let rs = buffer_to_sql_sqlstmnt(&data).expect("should parse");
        assert_eq!(rs.columns(), 2);
        assert_eq!(rs.rows(), 2);
        assert_eq!(rs.column_name(0), Some("a"));
        assert_eq!(rs.column_name(1), Some("b"));
        assert_eq!(rs.value(0, 0), Some(Some("1")));
        assert_eq!(rs.value(0, 1), Some(Some("2")));
        assert_eq!(rs.value(1, 0), Some(Some("3")));
        assert_eq!(rs.value(1, 1), Some(Some("4")));
    }

    #[test]
    fn sql_metadata_response() {
        let data = rds(&["\"id\",\"INTEGER\",\"java.lang.Integer\""]);
        let meta = buffer_to_sql_metadata(&data).expect("should parse");
        assert_eq!(meta.count(), 1);
        let item = meta.get(0).unwrap();
        assert_eq!(item.name, "id");
        assert_eq!(item.data_type, "INTEGER");
        assert_eq!(item.jtype, "java.lang.Integer");
    }

    #[test]
    fn supported_commands_response() {
        let data = rds(&["SELECT,INSERT,UPDATE,"]);
        let sc = buffer_to_sql_supportedcommands(&data).expect("should parse");
        assert_eq!(sc.count(), 3);
        assert_eq!(sc.get(0), Some("SELECT"));
        assert_eq!(sc.get(2), Some("UPDATE"));
    }

    #[test]
    fn dbdescription_response() {
        let data = rds(&["\"PostgreSQL\""]);
        let desc = buffer_to_sql_dbdescription(&data).expect("should parse");
        assert_eq!(desc, "PostgreSQL");
    }

    #[test]
    fn debugger_stop_rds_ok() {
        let xml = "<wddxPacket version=\"1.0\"><header/><data><array length=\"1\"><struct type=\"java.util.HashMap\"><var name=\"STATUS\"><string>RDS_OK</string></var></struct></array></data></wddxPacket>";
        let data = rds(&[xml]);
        assert!(buffer_to_debugger_stop(&data));
    }

    #[test]
    fn debugger_info_returns_port() {
        let xml = "<wddxPacket version=\"1.0\"><header/><data><array length=\"1\"><struct type=\"java.util.HashMap\"><var name=\"STATUS\"><string>RDS_OK</string></var><var name=\"DEBUG_SERVER_PORT\"><number>8501</number></var></struct></array></data></wddxPacket>";
        let data = rds(&[xml]);
        assert_eq!(buffer_to_debugger_info(&data), Some(8501));
    }

    #[test]
    fn debugger_event_response() {
        let xml = "<wddxPacket version=\"1.0\"><header/><data><array length=\"1\"><struct type=\"java.util.HashMap\"><var name=\"EVENT\"><string>BREAKPOINT</string></var><var name=\"SOURCE\"><string>/app/index.cfm</string></var><var name=\"LINE\"><number>42</number></var></struct></array></data></wddxPacket>";
        let data = rds(&[xml]);
        let wddx = buffer_to_debugger_event(&data).expect("should parse");
        assert_eq!(wddx.get_string("0,EVENT"), Some("BREAKPOINT"));
        assert_eq!(wddx.get_string("0,SOURCE"), Some("/app/index.cfm"));
        assert_eq!(wddx.get_number("0,LINE"), Some(42.0));
    }
}
