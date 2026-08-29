#pragma once

#include <cassert>
#include <memory>

#ifdef __cplusplus

extern "C" {
#include "cfrds.h"
}

namespace cfrds {

// ------------------------------------------------------------------------------------------------
// Typedef Aliases for RAII Managed Pointers
// ------------------------------------------------------------------------------------------------

using Server = std::unique_ptr<::cfrds_server, decltype([](::cfrds_server* p) { ::cfrds_server_free(p); })>;                          ///< Managed cfrds_server connection instance.
using BrowseDir = std::unique_ptr<::cfrds_browse_dir, decltype([](::cfrds_browse_dir* p) { ::cfrds_browse_dir_free(p); })>;              ///< Managed cfrds_browse_dir directory listing.
using FileContent = std::unique_ptr<::cfrds_file_content, decltype([](::cfrds_file_content* p) { ::cfrds_file_content_free(p); })>;          ///< Managed cfrds_file_content file reading buffer.
using SqlDsnInfo = std::unique_ptr<::cfrds_sql_dsninfo, decltype([](::cfrds_sql_dsninfo* p) { ::cfrds_sql_dsninfo_free(p); })>;            ///< Managed cfrds_sql_dsninfo data source names.
using SqlTableInfo = std::unique_ptr<::cfrds_sql_tableinfo, decltype([](::cfrds_sql_tableinfo* p) { ::cfrds_sql_tableinfo_free(p); })>;        ///< Managed cfrds_sql_tableinfo tables list.
using SqlColumnInfo = std::unique_ptr<::cfrds_sql_columninfo, decltype([](::cfrds_sql_columninfo* p) { ::cfrds_sql_columninfo_free(p); })>;    ///< Managed cfrds_sql_columninfo columns detail.
using SqlPrimaryKeys = std::unique_ptr<::cfrds_sql_primarykeys, decltype([](::cfrds_sql_primarykeys* p) { ::cfrds_sql_primarykeys_free(p); })>;  ///< Managed cfrds_sql_primarykeys table primary keys.
using SqlForeignKeys = std::unique_ptr<::cfrds_sql_foreignkeys, decltype([](::cfrds_sql_foreignkeys* p) { ::cfrds_sql_foreignkeys_free(p); })>;  ///< Managed cfrds_sql_foreignkeys table foreign keys.
using SqlImportedKeys = std::unique_ptr<::cfrds_sql_importedkeys, decltype([](::cfrds_sql_importedkeys* p) { ::cfrds_sql_importedkeys_free(p); })>; ///< Managed cfrds_sql_importedkeys table imported foreign keys.
using SqlExportedKeys = std::unique_ptr<::cfrds_sql_exportedkeys, decltype([](::cfrds_sql_exportedkeys* p) { ::cfrds_sql_exportedkeys_free(p); })>; ///< Managed cfrds_sql_exportedkeys table exported foreign keys.
using SqlResultSet = std::unique_ptr<::cfrds_sql_resultset, decltype([](::cfrds_sql_resultset* p) { ::cfrds_sql_resultset_free(p); })>;        ///< Managed cfrds_sql_resultset query tabular output.
using SqlMetadata = std::unique_ptr<::cfrds_sql_metadata, decltype([](::cfrds_sql_metadata* p) { ::cfrds_sql_metadata_free(p); })>;          ///< Managed cfrds_sql_metadata query metadata columns.
using SqlSupportedCommands = std::unique_ptr<::cfrds_sql_supportedcommands, decltype([](::cfrds_sql_supportedcommands* p) { ::cfrds_sql_supportedcommands_free(p); })>; ///< Managed cfrds_sql_supportedcommands catalog commands.
using DebuggerEvent = std::unique_ptr<::cfrds_debugger_event, decltype([](::cfrds_debugger_event* p) { ::cfrds_debugger_event_free(p); })>;    ///< Managed cfrds_debugger_event debugger events.
using SecurityAnalyzerResult = std::unique_ptr<::cfrds_security_analyzer_result, decltype([](::cfrds_security_analyzer_result* p) { ::cfrds_security_analyzer_result_free(p); })>; ///< Managed cfrds_security_analyzer_result scanner report.
using AdminApiCustomTagPaths = std::unique_ptr<::cfrds_adminapi_customtagpaths, decltype([](::cfrds_adminapi_customtagpaths* p) { ::cfrds_adminapi_customtagpaths_free(p); })>; ///< Managed cfrds_adminapi_customtagpaths tag paths.
using AdminApiMappings = std::unique_ptr<::cfrds_adminapi_mappings, decltype([](::cfrds_adminapi_mappings* p) { ::cfrds_adminapi_mappings_free(p); })>; ///< Managed cfrds_adminapi_mappings logic path configurations.

} // namespace cfrds

#endif // __cplusplus
