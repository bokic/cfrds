//! SQL/database commands (port of `src/cfrds_sql.c`).

use crate::error::{Result, Status};
use crate::parser;
use crate::server::Server;
use crate::types::{
    ColumnInfo, DsnInfo, KeyInfo, PrimaryKeys, ResultSet, SqlMetadata, SupportedCommands, TableInfo,
};

impl Server {
    /// Retrieves information about all configured Data Source Names (DSN).
    pub fn sql_dsninfo(&self) -> Result<DsnInfo> {
        execute_sql(self, &["", "DSNINFO"], parser::buffer_to_sql_dsninfo)
    }

    /// Retrieves the database table list metadata for a DSN.
    pub fn sql_tableinfo(&self, connection_name: &str) -> Result<TableInfo> {
        if connection_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "TABLEINFO"],
            parser::buffer_to_sql_tableinfo,
        )
    }

    /// Retrieves column metadata for a database table.
    pub fn sql_columninfo(&self, connection_name: &str, table_name: &str) -> Result<ColumnInfo> {
        if connection_name.is_empty() || table_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "COLUMNINFO", table_name],
            parser::buffer_to_sql_columninfo,
        )
    }

    /// Retrieves primary key information for a database table.
    pub fn sql_primarykeys(&self, connection_name: &str, table_name: &str) -> Result<PrimaryKeys> {
        if table_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "PRIMARYKEYS", table_name],
            parser::buffer_to_sql_primarykeys,
        )
    }

    /// Retrieves foreign key relationships defined on a table.
    pub fn sql_foreignkeys(&self, connection_name: &str, table_name: &str) -> Result<KeyInfo> {
        if table_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "FOREIGNKEYS", table_name],
            parser::buffer_to_sql_foreignkeys,
        )
    }

    /// Retrieves imported foreign keys for a database table.
    pub fn sql_importedkeys(&self, connection_name: &str, table_name: &str) -> Result<KeyInfo> {
        if table_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "IMPORTEDKEYS", table_name],
            parser::buffer_to_sql_importedkeys,
        )
    }

    /// Retrieves exported foreign keys referencing a database table.
    pub fn sql_exportedkeys(&self, connection_name: &str, table_name: &str) -> Result<KeyInfo> {
        if table_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "EXPORTEDKEYS", table_name],
            parser::buffer_to_sql_exportedkeys,
        )
    }

    /// Executes an SQL statement on a DSN and returns the resultset.
    pub fn sql_sqlstmnt(&self, connection_name: &str, sql: &str) -> Result<ResultSet> {
        if connection_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "SQLSTMNT", sql],
            parser::buffer_to_sql_sqlstmnt,
        )
    }

    /// Retrieves query resultset column metadata.
    pub fn sql_sqlmetadata(&self, connection_name: &str, sql: &str) -> Result<SqlMetadata> {
        if connection_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "SQLMETADATA", sql],
            parser::buffer_to_sql_metadata,
        )
    }

    /// Retrieves the supported SQL commands catalog from the database server.
    pub fn sql_getsupportedcommands(&self) -> Result<SupportedCommands> {
        execute_sql(
            self,
            &["", "SUPPORTEDCOMMANDS"],
            parser::buffer_to_sql_supportedcommands,
        )
    }

    /// Retrieves the description/version banner of the database engine for a DSN.
    pub fn sql_dbdescription(&self, connection_name: &str) -> Result<String> {
        if connection_name.is_empty() {
            return Err(Status::ParamIsNull.into());
        }
        execute_sql(
            self,
            &[connection_name, "DBDESCRIPTION"],
            parser::buffer_to_sql_dbdescription,
        )
    }
}

/// Sends a `DBFUNCS` command and parses the response via `parser_fn`.
fn execute_sql<T>(
    server: &Server,
    params: &[&str],
    parser_fn: fn(&[u8]) -> Option<T>,
) -> Result<T> {
    let response = server.send_command("DBFUNCS", params)?;
    parser_fn(&response)
        .ok_or_else(|| server.set_error(Status::ResponseError, "failed to parse DBFUNCS response"))
}
