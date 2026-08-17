#pragma once

#include <cassert>
#include <memory>

#ifdef __cplusplus

extern "C" {
#include "cfrds.h"
}

namespace cfrds {

/**
 * @brief RAII resource manager template class for libcfrds C-allocated pointers.
 * 
 * AutoFree wraps an opaque C struct pointer (e.g., `cfrds_server`, `cfrds_browse_dir`)
 * and automates resource deallocation. When an AutoFree instance goes out of scope,
 * its destructor executes the compile-time designated C free function.
 * 
 * @note This class enforces unique ownership semantics: copying is prohibited
 *       to prevent double-free bugs, but moving is supported to transfer resource ownership.
 * 
 * @tparam T The C struct pointer type being managed.
 * @tparam FreeFunc The corresponding C deallocation function pointer.
 */
template<typename T, void (*FreeFunc)(T*)>
class AutoFree {
private:
    T* _ptr;                             ///< Managed raw pointer.

public:
    /**
     * @brief Default constructor creates an empty wrapper initialized to nullptr.
     */
    constexpr AutoFree() noexcept : _ptr(nullptr) {}

    /**
     * @brief Constructs an AutoFree manager taking ownership of a raw pointer.
     * @param ptr Raw pointer to manage. Can be nullptr.
     */
    explicit AutoFree(T* ptr) noexcept : _ptr(ptr) {}
    
    // Disable copy constructor and copy assignment to enforce strict single-owner semantics
    AutoFree(const AutoFree&) = delete;
    AutoFree& operator=(const AutoFree&) = delete;

    /**
     * @brief Move constructor transfers pointer ownership.
     * @param other Rvalue reference to transfer ownership from. Cleared to nullptr.
     */
    AutoFree(AutoFree&& other) noexcept : _ptr(other._ptr) {
        other._ptr = nullptr;
    }

    /**
     * @brief Move assignment operator transfers pointer ownership.
     * @param other Rvalue reference to transfer ownership from. Cleared to nullptr.
     * @return Reference to this object.
     */
    AutoFree& operator=(AutoFree&& other) noexcept {
        if (this != std::addressof(other)) {
            free_internal();
            _ptr = other._ptr;
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
    T* get() const noexcept { return _ptr; }

    /**
     * @brief Address-of operator returns the address of the underlying pointer for out-parameters.
     * @note Preconditions: The wrapper must be empty (nullptr) to prevent memory leaks.
     * @return Address of the internal raw pointer.
     */
    T** operator&() noexcept {
        assert(_ptr == nullptr && "operator& called on non-null AutoFree; must be empty");
        return &_ptr;
    }

    /**
     * @brief Const address-of operator returns pointer to the underlying raw pointer.
     * @return Address of the internal const pointer.
     */
    T* const* operator&() const noexcept {
        return &_ptr;
    }

    /**
     * @brief Implicit conversion operator to the underlying raw pointer.
     *        Allows the wrapper to be passed directly to C functions.
     */
    operator T*() const noexcept { return _ptr; }

    /**
     * @brief Explicit boolean conversion checking if the managed pointer is non-null.
     */
    explicit operator bool() const noexcept { return _ptr != nullptr; }

    /**
     * @brief Releases ownership of the managed pointer without freeing it.
     * @return Raw pointer.
     */
    T* release() noexcept {
        T* tmp = _ptr;
        _ptr = nullptr;
        return tmp;
    }

    /**
     * @brief Resets the managed pointer, freeing any existing pointer.
     * @param ptr New raw pointer to manage.
     */
    void reset(T* ptr = nullptr) noexcept {
        if (_ptr != ptr) {
            free_internal();
            _ptr = ptr;
        }
    }

private:
    /**
     * @brief Executes deallocation if the pointer is active.
     */
    void free_internal() noexcept {
        if (_ptr) {
            FreeFunc(_ptr);
            _ptr = nullptr;
        }
    }
};


// ------------------------------------------------------------------------------------------------
// Typedef Aliases for RAII Managed Pointers
// ------------------------------------------------------------------------------------------------

using Server = AutoFree<::cfrds_server, ::cfrds_server_free>;                          ///< Managed cfrds_server connection instance.
using BrowseDir = AutoFree<::cfrds_browse_dir, ::cfrds_browse_dir_free>;              ///< Managed cfrds_browse_dir directory listing.
using FileContent = AutoFree<::cfrds_file_content, ::cfrds_file_content_free>;          ///< Managed cfrds_file_content file reading buffer.
using SqlDsnInfo = AutoFree<::cfrds_sql_dsninfo, ::cfrds_sql_dsninfo_free>;            ///< Managed cfrds_sql_dsninfo data source names.
using SqlTableInfo = AutoFree<::cfrds_sql_tableinfo, ::cfrds_sql_tableinfo_free>;        ///< Managed cfrds_sql_tableinfo tables list.
using SqlColumnInfo = AutoFree<::cfrds_sql_columninfo, ::cfrds_sql_columninfo_free>;    ///< Managed cfrds_sql_columninfo columns detail.
using SqlPrimaryKeys = AutoFree<::cfrds_sql_primarykeys, ::cfrds_sql_primarykeys_free>;  ///< Managed cfrds_sql_primarykeys table primary keys.
using SqlForeignKeys = AutoFree<::cfrds_sql_foreignkeys, ::cfrds_sql_foreignkeys_free>;  ///< Managed cfrds_sql_foreignkeys table foreign keys.
using SqlImportedKeys = AutoFree<::cfrds_sql_importedkeys, ::cfrds_sql_importedkeys_free>; ///< Managed cfrds_sql_importedkeys table imported foreign keys.
using SqlExportedKeys = AutoFree<::cfrds_sql_exportedkeys, ::cfrds_sql_exportedkeys_free>; ///< Managed cfrds_sql_exportedkeys table exported foreign keys.
using SqlResultSet = AutoFree<::cfrds_sql_resultset, ::cfrds_sql_resultset_free>;        ///< Managed cfrds_sql_resultset query tabular output.
using SqlMetadata = AutoFree<::cfrds_sql_metadata, ::cfrds_sql_metadata_free>;          ///< Managed cfrds_sql_metadata query metadata columns.
using SqlSupportedCommands = AutoFree<::cfrds_sql_supportedcommands, ::cfrds_sql_supportedcommands_free>; ///< Managed cfrds_sql_supportedcommands catalog commands.
using DebuggerEvent = AutoFree<::cfrds_debugger_event, ::cfrds_debugger_event_free>;    ///< Managed cfrds_debugger_event debugger events.
using SecurityAnalyzerResult = AutoFree<::cfrds_security_analyzer_result, ::cfrds_security_analyzer_result_free>; ///< Managed cfrds_security_analyzer_result scanner report.
using AdminApiCustomTagPaths = AutoFree<::cfrds_adminapi_customtagpaths, ::cfrds_adminapi_customtagpaths_free>; ///< Managed cfrds_adminapi_customtagpaths tag paths.
using AdminApiMappings = AutoFree<::cfrds_adminapi_mappings, ::cfrds_adminapi_mappings_free>; ///< Managed cfrds_adminapi_mappings logic path configurations.

} // namespace cfrds

#endif // __cplusplus
