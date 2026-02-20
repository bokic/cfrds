#pragma once

#ifdef __cplusplus

extern "C" {
#include "cfrds.h"
}

// Forward declare the opaque structures from the C library in the global namespace
struct cfrds_server;
struct cfrds_browse_dir;
struct cfrds_file_content;
struct cfrds_sql_dsninfo;
struct cfrds_sql_tableinfo;
struct cfrds_sql_columninfo;
struct cfrds_sql_primarykeys;
struct cfrds_sql_foreignkeys;
struct cfrds_sql_importedkeys;
struct cfrds_sql_exportedkeys;
struct cfrds_sql_resultset;
struct cfrds_sql_metadata;
struct cfrds_sql_supportedcommands;
struct cfrds_debugger_event;
struct cfrds_security_analyzer_result;
struct cfrds_adminapi_customtagpaths;
struct cfrds_adminapi_mappings;

namespace cfrds {

// Base wrapper template to handle the RAII destruction logic automatically
template<typename T>
class AutoFree {
private:
    T* _ptr;
    void (*_free_func)(T*);

public:
    AutoFree(T* ptr, void (*free_func)(T*)) : _ptr(ptr), _free_func(free_func) {}
    
    // Prevent copying to maintain strict RAII ownership
    AutoFree(const AutoFree&) = delete;
    AutoFree& operator=(const AutoFree&) = delete;

    // Allow moving ownership
    AutoFree(AutoFree&& other) noexcept : _ptr(other._ptr), _free_func(other._free_func) {
        other._ptr = nullptr;
    }

    AutoFree& operator=(AutoFree&& other) noexcept {
        if (this != &other) {
            free_internal();
            _ptr = other._ptr;
            _free_func = other._free_func;
            other._ptr = nullptr;
        }
        return *this;
    }

    ~AutoFree() {
        free_internal();
    }

    T* get() const { return _ptr; }
    
    T** ptr_ref() { 
        free_internal(); 
        return &_ptr; 
    }

    operator T*() const { return _ptr; }

private:
    void free_internal() {
        if (_ptr) {
            _free_func(_ptr);
            _ptr = nullptr;
        }
    }
};


// ------------------------------------------------------------------------------------------------
// Typedef Aliases for RAII Managed Pointers
// ------------------------------------------------------------------------------------------------

using Server = AutoFree<struct ::cfrds_server>;
using BrowseDir = AutoFree<struct ::cfrds_browse_dir>;
using FileContent = AutoFree<struct ::cfrds_file_content>;
using SqlDsnInfo = AutoFree<struct ::cfrds_sql_dsninfo>;
using SqlTableInfo = AutoFree<struct ::cfrds_sql_tableinfo>;
using SqlColumnInfo = AutoFree<struct ::cfrds_sql_columninfo>;
using SqlPrimaryKeys = AutoFree<struct ::cfrds_sql_primarykeys>;
using SqlForeignKeys = AutoFree<struct ::cfrds_sql_foreignkeys>;
using SqlImportedKeys = AutoFree<struct ::cfrds_sql_importedkeys>;
using SqlExportedKeys = AutoFree<struct ::cfrds_sql_exportedkeys>;
using SqlResultSet = AutoFree<struct ::cfrds_sql_resultset>;
using SqlMetadata = AutoFree<struct ::cfrds_sql_metadata>;
using SqlSupportedCommands = AutoFree<struct ::cfrds_sql_supportedcommands>;
using DebuggerEvent = AutoFree<struct ::cfrds_debugger_event>;
using SecurityAnalyzerResult = AutoFree<struct ::cfrds_security_analyzer_result>;
using AdminApiCustomTagPaths = AutoFree<struct ::cfrds_adminapi_customtagpaths>;
using AdminApiMappings = AutoFree<struct ::cfrds_adminapi_mappings>;

// ------------------------------------------------------------------------------------------------
// Factory Methods for Easy Initialization
// ------------------------------------------------------------------------------------------------

inline Server make_server() { return Server(nullptr, ::cfrds_server_free); }
inline BrowseDir make_browse_dir() { return BrowseDir(nullptr, ::cfrds_browse_dir_free); }
inline FileContent make_file_content() { return FileContent(nullptr, ::cfrds_file_content_free); }
inline SqlDsnInfo make_sql_dsninfo() { return SqlDsnInfo(nullptr, ::cfrds_sql_dsninfo_free); }
inline SqlTableInfo make_sql_tableinfo() { return SqlTableInfo(nullptr, ::cfrds_sql_tableinfo_free); }
inline SqlColumnInfo make_sql_columninfo() { return SqlColumnInfo(nullptr, ::cfrds_sql_columninfo_free); }
inline SqlPrimaryKeys make_sql_primarykeys() { return SqlPrimaryKeys(nullptr, ::cfrds_sql_primarykeys_free); }
inline SqlForeignKeys make_sql_foreignkeys() { return SqlForeignKeys(nullptr, ::cfrds_sql_foreignkeys_free); }
inline SqlImportedKeys make_sql_importedkeys() { return SqlImportedKeys(nullptr, ::cfrds_sql_importedkeys_free); }
inline SqlExportedKeys make_sql_exportedkeys() { return SqlExportedKeys(nullptr, ::cfrds_sql_exportedkeys_free); }
inline SqlResultSet make_sql_resultset() { return SqlResultSet(nullptr, ::cfrds_sql_resultset_free); }
inline SqlMetadata make_sql_metadata() { return SqlMetadata(nullptr, ::cfrds_sql_metadata_free); }
inline SqlSupportedCommands make_sql_supportedcommands() { return SqlSupportedCommands(nullptr, ::cfrds_sql_supportedcommands_free); }
inline DebuggerEvent make_debugger_event() { return DebuggerEvent(nullptr, ::cfrds_debugger_event_free); }
inline SecurityAnalyzerResult make_security_analyzer_result() { return SecurityAnalyzerResult(nullptr, ::cfrds_security_analyzer_result_free); }
inline AdminApiCustomTagPaths make_adminapi_customtagpaths() { return AdminApiCustomTagPaths(nullptr, ::cfrds_adminapi_customtagpaths_free); }
inline AdminApiMappings make_adminapi_mappings() { return AdminApiMappings(nullptr, ::cfrds_adminapi_mappings_free); }

} // namespace cfrds

#endif // __cplusplus
