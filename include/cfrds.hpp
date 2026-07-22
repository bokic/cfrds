#pragma once

#include <cassert>

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

/**
 * @brief RAII resource manager template class for libcfrds C-allocated pointers.
 * 
 * AutoFree wraps an opaque C struct pointer (e.g., `cfrds_server`, `cfrds_browse_dir`)
 * and automates resource deallocation. When an AutoFree instance goes out of scope,
 * its destructor executes the designated C free function.
 * 
 * @note This class enforces unique ownership semantics: copying is prohibited
 *       to prevent double-free bugs, but moving is supported to transfer resource ownership.
 * 
 * @tparam T The C struct pointer type being managed.
 */
template<typename T>
class AutoFree {
private:
    T* _ptr;                             ///< Managed raw pointer.
    void (*_free_func)(T*);              ///< Designated deallocation function from the C API.

public:
    /**
     * @brief Constructs an AutoFree manager.
     * @param ptr Raw pointer to manage. Can be nullptr.
     * @param free_func Corresponding deallocation function (e.g., `cfrds_server_free`).
     */
    AutoFree(T* ptr, void (*free_func)(T*)) : _ptr(ptr), _free_func(free_func) {}
    
    // Disable copy constructor and copy assignment to enforce strict single-owner semantics
    AutoFree(const AutoFree&) = delete;
    AutoFree& operator=(const AutoFree&) = delete;

    /**
     * @brief Move constructor transfers pointer ownership.
     * @param other Rvalue reference to transfer ownership from. Cleared to nullptr.
     */
    AutoFree(AutoFree&& other) noexcept : _ptr(other._ptr), _free_func(other._free_func) {
        other._ptr = nullptr;
    }

    /**
     * @brief Move assignment operator transfers pointer ownership.
     * @param other Rvalue reference to transfer ownership from. Cleared to nullptr.
     * @return Reference to this object.
     */
    AutoFree& operator=(AutoFree&& other) noexcept {
        if (this != &other) {
            free_internal();
            _ptr = other._ptr;
            _free_func = other._free_func;
            other._ptr = nullptr;
        }
        return *this;
    }

    /**
     * @brief Destructor automatically invokes the bound free function.
     */
    ~AutoFree() {
        free_internal();
    }

    /**
     * @brief Retrieves the raw pointer managed by this instance.
     * @return Raw pointer. Valid until deallocated.
     */
    T* get() const { return _ptr; }
    
    /**
     * @brief Retrieves a double-pointer reference to populate empty wrappers via out parameters.
     * @note Preconditions: The wrapped pointer must be nullptr to prevent memory leaks.
     * @return Address of the internal raw pointer.
     */
    T** ptr_ref() { 
        assert(_ptr == nullptr && "ptr_ref() called on non-null AutoFree; must be empty");
        return &_ptr; 
    }

    /**
     * @brief Implicit conversion operator to the underlying raw pointer.
     *        Allows the wrapper to be passed directly to C functions.
     */
    operator T*() const { return _ptr; }

private:
    /**
     * @brief Executes deallocation if the pointer is active.
     */
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

using Server = AutoFree<struct ::cfrds_server>;                          ///< Managed cfrds_server connection instance.
using BrowseDir = AutoFree<struct ::cfrds_browse_dir>;                    ///< Managed cfrds_browse_dir directory listing.
using FileContent = AutoFree<struct ::cfrds_file_content>;                ///< Managed cfrds_file_content file reading buffer.
using SqlDsnInfo = AutoFree<struct ::cfrds_sql_dsninfo>;                  ///< Managed cfrds_sql_dsninfo data source names.
using SqlTableInfo = AutoFree<struct ::cfrds_sql_tableinfo>;              ///< Managed cfrds_sql_tableinfo tables list.
using SqlColumnInfo = AutoFree<struct ::cfrds_sql_columninfo>;            ///< Managed cfrds_sql_columninfo columns detail.
using SqlPrimaryKeys = AutoFree<struct ::cfrds_sql_primarykeys>;          ///< Managed cfrds_sql_primarykeys table primary keys.
using SqlForeignKeys = AutoFree<struct ::cfrds_sql_foreignkeys>;          ///< Managed cfrds_sql_foreignkeys table foreign keys.
using SqlImportedKeys = AutoFree<struct ::cfrds_sql_importedkeys>;        ///< Managed cfrds_sql_importedkeys table imported foreign keys.
using SqlExportedKeys = AutoFree<struct ::cfrds_sql_exportedkeys>;        ///< Managed cfrds_sql_exportedkeys table exported foreign keys.
using SqlResultSet = AutoFree<struct ::cfrds_sql_resultset>;              ///< Managed cfrds_sql_resultset query tabular output.
using SqlMetadata = AutoFree<struct ::cfrds_sql_metadata>;                ///< Managed cfrds_sql_metadata query metadata columns.
using SqlSupportedCommands = AutoFree<struct ::cfrds_sql_supportedcommands>; ///< Managed cfrds_sql_supportedcommands catalog commands.
using DebuggerEvent = AutoFree<struct ::cfrds_debugger_event>;            ///< Managed cfrds_debugger_event debugger events.
using SecurityAnalyzerResult = AutoFree<struct ::cfrds_security_analyzer_result>; ///< Managed cfrds_security_analyzer_result scanner report.
using AdminApiCustomTagPaths = AutoFree<struct ::cfrds_adminapi_customtagpaths>; ///< Managed cfrds_adminapi_customtagpaths tag paths.
using AdminApiMappings = AutoFree<struct ::cfrds_adminapi_mappings>;      ///< Managed cfrds_adminapi_mappings logic path configurations.

// ------------------------------------------------------------------------------------------------
// Factory Methods for Easy Initialization
// ------------------------------------------------------------------------------------------------

/** @brief Instantiates a Server connection wrapper initialized to nullptr. */
inline Server make_server() { return Server(nullptr, ::cfrds_server_free); }
/** @brief Instantiates a BrowseDir wrapper initialized to nullptr. */
inline BrowseDir make_browse_dir() { return BrowseDir(nullptr, ::cfrds_browse_dir_free); }
/** @brief Instantiates a FileContent wrapper initialized to nullptr. */
inline FileContent make_file_content() { return FileContent(nullptr, ::cfrds_file_content_free); }
/** @brief Instantiates an SqlDsnInfo wrapper initialized to nullptr. */
inline SqlDsnInfo make_sql_dsninfo() { return SqlDsnInfo(nullptr, ::cfrds_sql_dsninfo_free); }
/** @brief Instantiates an SqlTableInfo wrapper initialized to nullptr. */
inline SqlTableInfo make_sql_tableinfo() { return SqlTableInfo(nullptr, ::cfrds_sql_tableinfo_free); }
/** @brief Instantiates an SqlColumnInfo wrapper initialized to nullptr. */
inline SqlColumnInfo make_sql_columninfo() { return SqlColumnInfo(nullptr, ::cfrds_sql_columninfo_free); }
/** @brief Instantiates an SqlPrimaryKeys wrapper initialized to nullptr. */
inline SqlPrimaryKeys make_sql_primarykeys() { return SqlPrimaryKeys(nullptr, ::cfrds_sql_primarykeys_free); }
/** @brief Instantiates an SqlForeignKeys wrapper initialized to nullptr. */
inline SqlForeignKeys make_sql_foreignkeys() { return SqlForeignKeys(nullptr, ::cfrds_sql_foreignkeys_free); }
/** @brief Instantiates an SqlImportedKeys wrapper initialized to nullptr. */
inline SqlImportedKeys make_sql_importedkeys() { return SqlImportedKeys(nullptr, ::cfrds_sql_importedkeys_free); }
/** @brief Instantiates an SqlExportedKeys wrapper initialized to nullptr. */
inline SqlExportedKeys make_sql_exportedkeys() { return SqlExportedKeys(nullptr, ::cfrds_sql_exportedkeys_free); }
/** @brief Instantiates an SqlResultSet wrapper initialized to nullptr. */
inline SqlResultSet make_sql_resultset() { return SqlResultSet(nullptr, ::cfrds_sql_resultset_free); }
/** @brief Instantiates an SqlMetadata wrapper initialized to nullptr. */
inline SqlMetadata make_sql_metadata() { return SqlMetadata(nullptr, ::cfrds_sql_metadata_free); }
/** @brief Instantiates an SqlSupportedCommands wrapper initialized to nullptr. */
inline SqlSupportedCommands make_sql_supportedcommands() { return SqlSupportedCommands(nullptr, ::cfrds_sql_supportedcommands_free); }
/** @brief Instantiates a DebuggerEvent wrapper initialized to nullptr. */
inline DebuggerEvent make_debugger_event() { return DebuggerEvent(nullptr, ::cfrds_debugger_event_free); }
/** @brief Instantiates a SecurityAnalyzerResult wrapper initialized to nullptr. */
inline SecurityAnalyzerResult make_security_analyzer_result() { return SecurityAnalyzerResult(nullptr, ::cfrds_security_analyzer_result_free); }
/** @brief Instantiates an AdminApiCustomTagPaths wrapper initialized to nullptr. */
inline AdminApiCustomTagPaths make_adminapi_customtagpaths() { return AdminApiCustomTagPaths(nullptr, ::cfrds_adminapi_customtagpaths_free); }
/** @brief Instantiates an AdminApiMappings wrapper initialized to nullptr. */
inline AdminApiMappings make_adminapi_mappings() { return AdminApiMappings(nullptr, ::cfrds_adminapi_mappings_free); }

} // namespace cfrds

#endif // __cplusplus
