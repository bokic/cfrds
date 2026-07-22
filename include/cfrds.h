#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @defgroup version Version Macros
 * @brief Compile-time version information, injected by CMake from the git tag (X.Y.Z).
 *
 * Usage example (require at least v1.2.0 at compile time):
 * @code
 * #if CFRDS_VERSION_INT < 10200
 * #error "libcfrds 1.2.0 or newer is required"
 * #endif
 * @endcode
 *
 * @def CFRDS_VERSION         Full version string, e.g. "1.0.5" or "1.0.5-3-gabcdef-dirty"
 * @def CFRDS_VERSION_MAJOR   Major version number (integer)
 * @def CFRDS_VERSION_MINOR   Minor version number (integer)
 * @def CFRDS_VERSION_PATCH   Patch version number (integer)
 * @def CFRDS_VERSION_INT     Combined integer: MAJOR*10000 + MINOR*100 + PATCH
 */
#include "version.h"

#ifdef libcfrds_EXPORTS
 #if defined(_WIN32) || defined(__CYGWIN__)
  #define EXPORT_CFRDS __declspec(dllexport)
 #else
  #define EXPORT_CFRDS __attribute__((visibility("default")))
 #endif
#else
 #if defined(_WIN32) || defined(__CYGWIN__)
  #define EXPORT_CFRDS __declspec(dllimport)
 #else
  #define EXPORT_CFRDS
 #endif
#endif

typedef char* cfrds_str;
typedef struct cfrds_buffer cfrds_buffer;
typedef struct cfrds_file_content cfrds_file_content;
typedef struct cfrds_server cfrds_server;
typedef struct WDDX_NODE cfrds_variable;
typedef struct cfrds_browse_dir cfrds_browse_dir;
typedef struct cfrds_sql_dsninfo cfrds_sql_dsninfo;
typedef struct cfrds_sql_tableinfo cfrds_sql_tableinfo;
typedef struct cfrds_sql_columninfo cfrds_sql_columninfo;
typedef struct cfrds_sql_primarykeys cfrds_sql_primarykeys;
typedef struct cfrds_sql_foreignkeys cfrds_sql_foreignkeys;
typedef struct cfrds_sql_importedkeys cfrds_sql_importedkeys;
typedef struct cfrds_sql_exportedkeys cfrds_sql_exportedkeys;
typedef struct cfrds_sql_resultset cfrds_sql_resultset;
typedef struct cfrds_sql_metadata cfrds_sql_metadata;
typedef struct cfrds_sql_supportedcommands cfrds_sql_supportedcommands;
typedef struct WDDX cfrds_debugger_event;
typedef char cfrds_security_analyzer_result;
typedef struct WDDX cfrds_adminapi_customtagpaths;
typedef struct WDDX cfrds_adminapi_mappings;

typedef enum {
    CFRDS_STATUS_OK,
    CFRDS_STATUS_MEMORY_ERROR,
    CFRDS_STATUS_PARAM_IS_NULL,
    CFRDS_STATUS_SERVER_IS_NULL,
    CFRDS_STATUS_INVALID_INPUT_PARAMETER,
    CFRDS_STATUS_INDEX_OUT_OF_BOUNDS,
    CFRDS_STATUS_COMMAND_FAILED,
    CFRDS_STATUS_RESPONSE_ERROR,
    CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND,
    CFRDS_STATUS_DIR_ALREADY_EXISTS,
    CFRDS_STATUS_SOCKET_HOST_NOT_FOUND,
    CFRDS_STATUS_SOCKET_CREATION_FAILED,
    CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED,
    CFRDS_STATUS_WRITING_TO_SOCKET_FAILED,
    CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET,
    CFRDS_STATUS_READING_FROM_SOCKET_FAILED,
    CFRDS_STATUS_RESPONSE_TOO_LARGE,
} cfrds_status;

typedef enum {
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET,
    CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT,
    CFRDS_DEBUGGER_EVENT_TYPE_STEP,
    CFRDS_DEBUGGER_EVENT_UNKNOWN,
} cfrds_debugger_type;

#if defined(__GNUC__) || defined(__clang__)
#define cfrds_buffer_defer(var) cfrds_buffer* var __attribute__((cleanup(cfrds_buffer_cleanup))) = NULL
#define cfrds_file_content_defer(var) cfrds_file_content* var __attribute__((cleanup(cfrds_file_content_cleanup))) = NULL
#define cfrds_str_defer(var) cfrds_str var __attribute__((cleanup(cfrds_str_cleanup))) = NULL
#endif

#if defined(__GNUC__) || defined(__clang__)
#define cfrds_server_defer(var) cfrds_server* var __attribute__((cleanup(cfrds_server_cleanup))) = NULL
#define cfrds_browse_dir_defer(var) cfrds_browse_dir* var __attribute__((cleanup(cfrds_browse_dir_cleanup))) = NULL
#define cfrds_sql_dsninfo_defer(var) cfrds_sql_dsninfo* var __attribute__((cleanup(cfrds_sql_dsninfo_cleanup))) = NULL
#define cfrds_sql_tableinfo_defer(var) cfrds_sql_tableinfo* var __attribute__((cleanup(cfrds_sql_tableinfo_cleanup))) = NULL
#define cfrds_sql_columninfo_defer(var) cfrds_sql_columninfo* var __attribute__((cleanup(cfrds_sql_columninfo_cleanup))) = NULL
#define cfrds_sql_primarykeys_defer(var) cfrds_sql_primarykeys* var __attribute__((cleanup(cfrds_sql_primarykeys_cleanup))) = NULL
#define cfrds_sql_foreignkeys_defer(var) cfrds_sql_foreignkeys* var __attribute__((cleanup(cfrds_sql_foreignkeys_cleanup))) = NULL
#define cfrds_sql_importedkeys_defer(var) cfrds_sql_importedkeys* var __attribute__((cleanup(cfrds_sql_importedkeys_cleanup))) = NULL
#define cfrds_sql_exportedkeys_defer(var) cfrds_sql_exportedkeys* var __attribute__((cleanup(cfrds_sql_exportedkeys_cleanup))) = NULL
#define cfrds_sql_resultset_defer(var) cfrds_sql_resultset* var __attribute__((cleanup(cfrds_sql_resultset_cleanup))) = NULL
#define cfrds_sql_metadata_defer(var) cfrds_sql_metadata* var __attribute__((cleanup(cfrds_sql_metadata_cleanup))) = NULL
#define cfrds_sql_supportedcommands_defer(var) cfrds_sql_supportedcommands* var __attribute__((cleanup(cfrds_sql_supportedcommands_cleanup))) = NULL
#define cfrds_debugger_event_defer(var) cfrds_debugger_event* var __attribute__((cleanup(cfrds_debugger_event_cleanup))) = NULL
#define cfrds_security_analyzer_result_defer(var) cfrds_security_analyzer_result* var __attribute__((cleanup(cfrds_security_analyzer_result_cleanup))) = NULL
#define cfrds_adminapi_customtagpaths_defer(var) cfrds_adminapi_customtagpaths* var __attribute__((cleanup(cfrds_adminapi_customtagpaths_cleanup))) = NULL
#define cfrds_adminapi_mappings_defer(var) cfrds_adminapi_mappings* var __attribute__((cleanup(cfrds_adminapi_mappings_cleanup))) = NULL
#endif

/**
 * @note Thread Safety
 *
 * This library is NOT thread-safe. A `cfrds_server` instance must not be used
 * concurrently from multiple threads without external synchronisation.
 *
 * Each thread must create and manage its own `cfrds_server` instance.
 * Result types (`cfrds_browse_dir`, `cfrds_file_content`, `cfrds_sql_resultset`,
 * etc.) are likewise not safe to share across threads without external locking.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Automatically deallocates and nullifies a cfrds_buffer pointer.
 * @param buf Double pointer to the buffer instance. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_buffer_cleanup(cfrds_buffer **buf);

/**
 * @brief Retrieves the raw character data pointer of the buffer.
 * @param buf Pointer to the buffer.
 * @return Character array pointer. Valid until buffer is freed or modified.
 */
EXPORT_CFRDS char *cfrds_buffer_data(cfrds_buffer *buf);

/**
 * @brief Returns the active data size in bytes of the buffer.
 * @param buf Pointer to the buffer.
 * @return Size of data in bytes.
 */
EXPORT_CFRDS size_t cfrds_buffer_data_size(cfrds_buffer *buf);

/**
 * @brief Automatically deallocates and nullifies a cfrds_file_content pointer.
 * @param buf Double pointer to file content instance. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_file_content_cleanup(cfrds_file_content **buf);

/**
 * @brief Automatically deallocates and nullifies a cfrds_str pointer.
 * @param str Double pointer to the string. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_str_cleanup(cfrds_str *str);

/**
 * @brief Initializes a cfrds_server connection instance.
 * @param server Output pointer to the initialized server structure. Must be freed with cfrds_server_free.
 * @param host The hostname or IP address of the ColdFusion server.
 * @param port The port number of the server (typically 8500 or 80).
 * @param username Username for RDS authentication. Can be NULL or empty if not required.
 * @param password Password for RDS authentication. Can be NULL or empty.
 * @return true on success, false on initialization or allocation failure.
 */
EXPORT_CFRDS bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password);

/**
 * @brief Deallocates all resources associated with a cfrds_server instance.
 * @param server Server instance to free. Safe to call if NULL.
 */
EXPORT_CFRDS void cfrds_server_free(cfrds_server *server);

/**
 * @brief Automatically deallocates and nullifies a cfrds_server pointer.
 * @param server Double pointer to the server instance. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_server_cleanup(cfrds_server **server);

/**
 * @brief Clears the internal error status of the cfrds_server instance.
 * @param server Server instance.
 */
EXPORT_CFRDS void cfrds_server_clear_error(cfrds_server *server);

/**
 * @brief Retrieves the last recorded error message for the server.
 * @param server Server instance.
 * @return Const pointer to the error string, or NULL if no error has occurred.
 */
EXPORT_CFRDS const char *cfrds_server_get_error(const cfrds_server *server);

/**
 * @brief Retrieves the server host address configuration.
 * @param server Server instance.
 * @return Const pointer to the host string.
 */
EXPORT_CFRDS const char *cfrds_server_get_host(const cfrds_server *server);

/**
 * @brief Retrieves the server port configuration.
 * @param server Server instance.
 * @return The port number.
 */
EXPORT_CFRDS uint16_t cfrds_server_get_port(const cfrds_server *server);

/**
 * @brief Retrieves the server username configuration.
 * @param server Server instance.
 * @return Const pointer to the username string.
 */
EXPORT_CFRDS const char *cfrds_server_get_username(const cfrds_server *server);

/**
 * @brief Retrieves the server password configuration.
 * @param server Server instance.
 * @return Const pointer to the password string.
 */
EXPORT_CFRDS const char *cfrds_server_get_password(const cfrds_server *server);

/**
 * @brief Lists files and folders in a remote directory on the server.
 * @param server Initialized server connection.
 * @param path Remote path to list.
 * @param out Output pointer to the allocated directory structure. Must be freed with cfrds_browse_dir_free.
 * @return Status code indicating success (CFRDS_STATUS_OK) or failure.
 */
EXPORT_CFRDS cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out);

/**
 * @brief Frees an allocated cfrds_browse_dir directory listing structure.
 * @param value Directory listing instance to free.
 */
EXPORT_CFRDS void cfrds_browse_dir_free(cfrds_browse_dir *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_browse_dir pointer.
 * @param buf Double pointer to directory listing. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf);

/**
 * @brief Returns the number of items returned in the directory listing.
 * @param value Directory listing structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_browse_dir_count(const cfrds_browse_dir *value);

/**
 * @brief Retrieves the item type at a specific index.
 * @param value Directory listing structure.
 * @param ndx 0-based item index.
 * @return Item kind character: 'F' for file, 'D' for directory, or '\0' if invalid.
 */
EXPORT_CFRDS char cfrds_browse_dir_item_get_kind(const cfrds_browse_dir *value, size_t ndx);

/**
 * @brief Retrieves the item name at a specific index.
 * @param value Directory listing structure.
 * @param ndx 0-based item index.
 * @return Const pointer to the item name string.
 */
EXPORT_CFRDS const char *cfrds_browse_dir_item_get_name(const cfrds_browse_dir *value, size_t ndx);

/**
 * @brief Retrieves the access permissions of the item at a specific index.
 * @param value Directory listing structure.
 * @param ndx 0-based item index.
 * @return Permission byte representation. The returned byte is a bitmask:
 *         - 0x01: Read-only (maps to FILE_ATTRIBUTE_READONLY)
 *         - 0x02: Hidden (maps to FILE_ATTRIBUTE_HIDDEN)
 *         - 0x04: System (maps to FILE_ATTRIBUTE_SYSTEM)
 *         - 0x10: Directory (maps to FILE_ATTRIBUTE_DIRECTORY)
 *         - 0x20: Archive (maps to FILE_ATTRIBUTE_ARCHIVE)
 *         - 0x80: Normal (maps to FILE_ATTRIBUTE_NORMAL)
 */
EXPORT_CFRDS uint8_t cfrds_browse_dir_item_get_permissions(const cfrds_browse_dir *value, size_t ndx);

/**
 * @brief Retrieves the file size in bytes of the item at a specific index.
 * @param value Directory listing structure.
 * @param ndx 0-based item index.
 * @return Size of file in bytes (directories return 0 or metadata size).
 */
EXPORT_CFRDS size_t cfrds_browse_dir_item_get_size(const cfrds_browse_dir *value, size_t ndx);

/**
 * @brief Retrieves the last modified timestamp of the item at a specific index.
 * @param value Directory listing structure.
 * @param ndx 0-based item index.
 * @return Unix epoch timestamp of modification time in seconds.
 */
EXPORT_CFRDS uint64_t cfrds_browse_dir_item_get_modified(const cfrds_browse_dir *value, size_t ndx);

/**
 * @brief Reads the contents of a remote file from the server.
 * @param server Initialized server connection.
 * @param pathname Path to the file on the remote server.
 * @param out Output pointer to the allocated file content structure. Must be freed with cfrds_file_content_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_read(cfrds_server *server, const char *pathname, cfrds_file_content **out);

/**
 * @brief Frees an allocated cfrds_file_content structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_file_content_free(cfrds_file_content *value);

/**
 * @brief Retrieves the raw file data content.
 * @param value File content structure.
 * @return Pointer to file content string.
 */
EXPORT_CFRDS const char *cfrds_file_content_get_data(const cfrds_file_content *value);

/**
 * @brief Retrieves the file size of the read file.
 * @param value File content structure.
 * @return Size of data in bytes.
 */
EXPORT_CFRDS size_t cfrds_file_content_get_size(const cfrds_file_content *value);

/**
 * @brief Retrieves the remote modification timestamp string.
 * @param value File content structure.
 * @return Last modified timestamp string.
 */
EXPORT_CFRDS const char *cfrds_file_content_get_modified(const cfrds_file_content *value);

/**
 * @brief Retrieves the remote permission string of the file.
 * @param value File content structure.
 * @return String description of permissions.
 */
EXPORT_CFRDS const char *cfrds_file_content_get_permission(const cfrds_file_content *value);

/**
 * @brief Writes data bytes to a remote file path on the server.
 * @param server Initialized server connection.
 * @param pathname Target file path.
 * @param data Pointer to the bytes to write.
 * @param length Number of bytes to write.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length);

/**
 * @brief Renames or moves a remote file or folder on the server.
 * @param server Initialized server connection.
 * @param current_pathname Current remote path.
 * @param new_pathname Destination remote path.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname);

/**
 * @brief Deletes a file on the remote server.
 * @param server Initialized server connection.
 * @param pathname Path to remote file.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname);

/**
 * @brief Deletes a directory on the remote server.
 * @param server Initialized server connection.
 * @param path Path to remote directory.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path);

/**
 * @brief Checks if a file or directory exists on the remote server.
 * @param server Initialized server connection.
 * @param pathname Path to remote resource.
 * @param out Output pointer where existence flag is stored.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out);

/**
 * @brief Creates a directory folder on the remote server.
 * @param server Initialized server connection.
 * @param path Path of directory to create.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path);

/**
 * @brief Retrieves the root directory path of the ColdFusion installation on the server.
 * @param server Initialized server connection.
 * @param out Output pointer where the allocated root path string is returned. Must be freed with cfrds_str_cleanup.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, cfrds_str *out);

/**
 * @brief Retrieves information about all configured Data Source Names (DSN) on the server.
 * @param server Initialized server connection.
 * @param dsninfo Output pointer to the allocated DSN info structure. Must be freed with cfrds_sql_dsninfo_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo);

/**
 * @brief Frees an allocated cfrds_sql_dsninfo structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_dsninfo_free(cfrds_sql_dsninfo *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_dsninfo pointer.
 * @param buf Double pointer to DSN info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_dsninfo_cleanup(cfrds_sql_dsninfo **buf);

/**
 * @brief Returns the count of configured DSNs.
 * @param value DSN info structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_dsninfo_count(const cfrds_sql_dsninfo *value);

/**
 * @brief Retrieves the DSN name string at a specific index.
 * @param value DSN info structure.
 * @param ndx 0-based index.
 * @return Const pointer to the DSN name.
 */
EXPORT_CFRDS const char *cfrds_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx);

/**
 * @brief Retrieves the database tables list metadata for a specific DSN.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param tableinfo Output pointer to the allocated table info structure. Must be freed with cfrds_sql_tableinfo_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo);

/**
 * @brief Frees an allocated cfrds_sql_tableinfo structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_tableinfo_free(cfrds_sql_tableinfo *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_tableinfo pointer.
 * @param buf Double pointer to table info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_tableinfo_cleanup(cfrds_sql_tableinfo **buf);

/**
 * @brief Returns the count of tables in the info list.
 * @param value Table info structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_tableinfo_count(const cfrds_sql_tableinfo *value);

/**
 * @brief Retrieves an unknown/first column attribute value for a table.
 * @param value Table info structure.
 * @param column 0-based table index.
 * @return Const attribute string.
 */
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_unknown(const cfrds_sql_tableinfo *value, size_t column);

/**
 * @brief Retrieves the schema name attribute for a table.
 * @param value Table info structure.
 * @param column 0-based table index.
 * @return Schema name string.
 */
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_schema(const cfrds_sql_tableinfo *value, size_t column);

/**
 * @brief Retrieves the table name attribute.
 * @param value Table info structure.
 * @param column 0-based table index.
 * @return Table name string.
 */
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_name(const cfrds_sql_tableinfo *value, size_t column);

/**
 * @brief Retrieves the table type attribute (e.g. "TABLE", "VIEW").
 * @param value Table info structure.
 * @param column 0-based table index.
 * @return Table type string.
 */
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_type(const cfrds_sql_tableinfo *value, size_t column);

/**
 * @brief Retrieves column metadata definitions for a database table.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param table_name Target table name.
 * @param columninfo Output pointer to the allocated column info. Must be freed with cfrds_sql_columninfo_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo);

/**
 * @brief Frees an allocated cfrds_sql_columninfo structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_columninfo_free(cfrds_sql_columninfo *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_columninfo pointer.
 * @param buf Double pointer to column info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_columninfo_cleanup(cfrds_sql_columninfo **buf);

/**
 * @brief Returns the count of columns in the column list.
 * @param value Column info structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_columninfo_count(const cfrds_sql_columninfo *value);

/**
 * @brief Retrieves the schema name attribute for a column index.
 * @param value Column info structure.
 * @param column 0-based column index.
 * @return Schema name string.
 */
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_schema(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the owner name attribute for a column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Owner name string.
 */
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_owner(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the table name attribute for a column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Table name string.
 */
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_table(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the column name attribute.
 * @param value Column info.
 * @param column 0-based index.
 * @return Column name string.
 */
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_name(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the JDBC database data type integer for the column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Type integer.
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_type(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the string representation of the column data type (e.g. "VARCHAR").
 * @param value Column info.
 * @param column 0-based index.
 * @return Type name string.
 */
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_typeStr(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the numeric precision attribute for the column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Precision integer.
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_precision(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the length limit attribute for the column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Length integer.
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_length(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the scale attribute for decimal types.
 * @param value Column info.
 * @param column 0-based index.
 * @return Scale integer.
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_scale(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves the numeric radix attribute for the column.
 * @param value Column info.
 * @param column 0-based index.
 * @return Radix integer.
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_radix(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Checks if the column is nullable (allows NULL).
 * @param value Column info.
 * @param column 0-based index.
 * @return Nullability code (1 if nullable, 0 if not).
 */
EXPORT_CFRDS int cfrds_sql_columninfo_get_nullable(const cfrds_sql_columninfo *value, size_t column);

/**
 * @brief Retrieves primary key information for a database table.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param table_name Target table name.
 * @param primarykeys Output pointer to the primary keys info. Must be freed with cfrds_sql_primarykeys_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys);

/**
 * @brief Frees an allocated cfrds_sql_primarykeys structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_primarykeys_free(cfrds_sql_primarykeys *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_primarykeys pointer.
 * @param buf Double pointer to primary keys info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_primarykeys_cleanup(cfrds_sql_primarykeys **buf);

/**
 * @brief Returns the count of primary keys.
 * @param value Primary keys structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_primarykeys_count(const cfrds_sql_primarykeys *value);

/**
 * @brief Retrieves catalog name of the primary key.
 * @param value Primary keys structure.
 * @param ndx 0-based index.
 * @return Catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_catalog(const cfrds_sql_primarykeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the primary key.
 * @param value Primary keys structure.
 * @param ndx 0-based index.
 * @return Owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_owner(const cfrds_sql_primarykeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the primary key.
 * @param value Primary keys structure.
 * @param ndx 0-based index.
 * @return Table name.
 */
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_table(const cfrds_sql_primarykeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the primary key.
 * @param value Primary keys structure.
 * @param ndx 0-based index.
 * @return Column name.
 */
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_column(const cfrds_sql_primarykeys *value, size_t ndx);

/**
 * @brief Retrieves key sequence number inside multi-column keys.
 * @param value Primary keys structure.
 * @param ndx 0-based index.
 * @return Key sequence index.
 */
EXPORT_CFRDS int cfrds_sql_primarykeys_get_key_sequence(const cfrds_sql_primarykeys *value, size_t ndx);

/**
 * @brief Retrieves foreign key relationships defined on a table.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param table_name Target table name.
 * @param foreignkeys Output pointer to foreign keys info. Must be freed with cfrds_sql_foreignkeys_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_foreignkeys **foreignkeys);

/**
 * @brief Frees an allocated cfrds_sql_foreignkeys structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_foreignkeys_free(cfrds_sql_foreignkeys *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_foreignkeys pointer.
 * @param buf Double pointer to foreign keys info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_foreignkeys_cleanup(cfrds_sql_foreignkeys **buf);

/**
 * @brief Returns the count of foreign keys.
 * @param value Foreign keys structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_foreignkeys_count(const cfrds_sql_foreignkeys *value);

/**
 * @brief Retrieves catalog name of the referenced primary key.
 * @param value Foreign keys structure.
 * @param ndx 0-based index.
 * @return Primary key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the referenced primary key.
 * @param value Foreign keys structure.
 * @param ndx 0-based index.
 * @return Primary key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkowner(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the referenced primary key.
 * @param value Foreign keys structure.
 * @param ndx 0-based index.
 * @return Primary key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pktable(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the referenced primary key.
 * @param value Foreign keys structure.
 * @param ndx 0-based index.
 * @return Primary key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves catalog name of the foreign key table.
 * @param value Foreign keys structure.
 * @param ndx 0-based index.
 * @return Foreign key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the foreign key table.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Foreign key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkowner(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the foreign key.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Foreign key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fktable(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the foreign key.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Foreign key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves sequence order of the foreign key inside composite keys.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Key sequence index.
 */
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_key_sequence(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on update constraint.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_updaterule(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on delete constraint.
 * @param value Foreign keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_deleterule(const cfrds_sql_foreignkeys *value, size_t ndx);

/**
 * @brief Retrieves imported foreign keys definitions in a database table.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param table_name Target table name.
 * @param importedkeys Output pointer to imported keys. Must be freed with cfrds_sql_importedkeys_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_importedkeys **importedkeys);

/**
 * @brief Frees an allocated cfrds_sql_importedkeys structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_importedkeys_free(cfrds_sql_importedkeys *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_importedkeys pointer.
 * @param buf Double pointer to imported keys info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_importedkeys_cleanup(cfrds_sql_importedkeys **buf);

/**
 * @brief Returns the count of imported keys.
 * @param value Imported keys structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_importedkeys_count(const cfrds_sql_importedkeys *value);

/**
 * @brief Retrieves catalog name of the referenced primary key.
 * @param value Imported keys structure.
 * @param ndx 0-based index.
 * @return Primary key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkcatalog(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the referenced primary key.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Primary key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkowner(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the referenced primary key.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Primary key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pktable(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the referenced primary key.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Primary key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkcolumn(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves catalog name of the foreign key table.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Foreign key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkcatalog(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the foreign key table.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Foreign key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkowner(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the foreign key.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Foreign key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fktable(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the foreign key.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Foreign key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkcolumn(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves sequence order of the imported key inside composite keys.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Key sequence index.
 */
EXPORT_CFRDS int cfrds_sql_importedkeys_get_key_sequence(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on update constraint.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_importedkeys_get_updaterule(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on delete constraint.
 * @param value Imported keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_importedkeys_get_deleterule(const cfrds_sql_importedkeys *value, size_t ndx);

/**
 * @brief Retrieves exported foreign keys definitions referencing a database table.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param table_name Target table name.
 * @param exportedkeys Output pointer to exported keys. Must be freed with cfrds_sql_exportedkeys_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_exportedkeys **exportedkeys);

/**
 * @brief Frees an allocated cfrds_sql_exportedkeys structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_exportedkeys_free(cfrds_sql_exportedkeys *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_exportedkeys pointer.
 * @param buf Double pointer to exported keys info. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_exportedkeys_cleanup(cfrds_sql_exportedkeys **buf);

/**
 * @brief Returns the count of exported keys.
 * @param value Exported keys structure.
 * @return Count of items.
 */
EXPORT_CFRDS size_t cfrds_sql_exportedkeys_count(const cfrds_sql_exportedkeys *value);

/**
 * @brief Retrieves catalog name of the primary key.
 * @param value Exported keys structure.
 * @param ndx 0-based index.
 * @return Primary key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the primary key.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Primary key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkowner(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the primary key.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Primary key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pktable(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the primary key.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Primary key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves catalog name of the foreign key table referencing it.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Foreign key catalog name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves owner name of the foreign key table.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Foreign key owner name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkowner(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves table name of the foreign key.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Foreign key table name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fktable(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves column name of the foreign key.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Foreign key column name.
 */
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves sequence order of the exported key inside composite keys.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Key sequence index.
 */
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_key_sequence(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on update constraint.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_updaterule(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Retrieves rule value indicating action on delete constraint.
 * @param value Exported keys.
 * @param ndx 0-based index.
 * @return Rule value identifier.
 */
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_deleterule(const cfrds_sql_exportedkeys *value, size_t ndx);

/**
 * @brief Executes an SQL query or statement on the target database DSN and returns a resultset.
 * @param server Initialized server connection.
 * @param connection_name The DSN connection name.
 * @param sql The SQL command string to execute.
 * @param resultset Output pointer to the allocated resultset structure. Must be freed with cfrds_sql_resultset_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_resultset **resultset);

/**
 * @brief Frees an allocated cfrds_sql_resultset structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_resultset_free(cfrds_sql_resultset *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_resultset pointer.
 * @param buf Double pointer to resultset. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_resultset_cleanup(cfrds_sql_resultset **buf);

/**
 * @brief Returns the count of columns returned in the resultset.
 * @param value Resultset structure.
 * @return Count of columns.
 */
EXPORT_CFRDS size_t cfrds_sql_resultset_columns(const cfrds_sql_resultset *value);

/**
 * @brief Returns the count of rows returned in the resultset.
 * @param value Resultset structure.
 * @return Count of rows.
 */
EXPORT_CFRDS size_t cfrds_sql_resultset_rows(const cfrds_sql_resultset *value);

/**
 * @brief Retrieves column header name for a specific column index.
 * @param value Resultset structure.
 * @param column 0-based column index.
 * @return Column header name.
 */
EXPORT_CFRDS const char *cfrds_sql_resultset_column_name(const cfrds_sql_resultset *value, size_t column);

/**
 * @brief Retrieves cell value string at a specific row and column grid coordinate.
 * @param value Resultset structure.
 * @param row 0-based row index.
 * @param column 0-based column index.
 * @return String value representing the cell content. Can be NULL if NULL in database.
 */
EXPORT_CFRDS const char *cfrds_sql_resultset_value(const cfrds_sql_resultset *value, size_t row, size_t column);

/**
 * @brief Retrieves query resultset metadata (columns detail).
 * @param server Initialized server connection.
 * @param connection_name DSN name.
 * @param sql The query string.
 * @param metadata Output pointer to allocated metadata. Must be freed with cfrds_sql_metadata_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_metadata **metadata);

/**
 * @brief Frees an allocated cfrds_sql_metadata structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_metadata_free(cfrds_sql_metadata *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_metadata pointer.
 * @param buf Double pointer to metadata. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_metadata_cleanup(cfrds_sql_metadata **buf);

/**
 * @brief Returns the count of columns described in metadata.
 * @param value Metadata structure.
 * @return Count of columns.
 */
EXPORT_CFRDS size_t cfrds_sql_metadata_count(const cfrds_sql_metadata *value);

/**
 * @brief Retrieves column name at a specific index.
 * @param value Metadata structure.
 * @param ndx 0-based column index.
 * @return Column name string.
 */
EXPORT_CFRDS const char *cfrds_sql_metadata_get_name(const cfrds_sql_metadata *value, size_t ndx);

/**
 * @brief Retrieves column database type name at a specific index.
 * @param value Metadata structure.
 * @param ndx 0-based column index.
 * @return Database type name string.
 */
EXPORT_CFRDS const char *cfrds_sql_metadata_get_type(const cfrds_sql_metadata *value, size_t ndx);

/**
 * @brief Retrieves column Java type name at a specific index.
 * @param value Metadata structure.
 * @param ndx 0-based column index.
 * @return Java class name string.
 */
EXPORT_CFRDS const char *cfrds_sql_metadata_get_jtype(const cfrds_sql_metadata *value, size_t ndx);

/**
 * @brief Retrieves supported SQL commands catalog from the database server.
 * @param server Initialized server connection.
 * @param supportedcommands Output pointer to supported commands structure. Must be freed with cfrds_sql_supportedcommands_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands);

/**
 * @brief Frees an allocated cfrds_sql_supportedcommands structure.
 * @param value Structure to free.
 */
EXPORT_CFRDS void cfrds_sql_supportedcommands_free(cfrds_sql_supportedcommands *value);

/**
 * @brief Automatically deallocates and nullifies a cfrds_sql_supportedcommands pointer.
 * @param buf Double pointer to supported commands. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_sql_supportedcommands_cleanup(cfrds_sql_supportedcommands **buf);

/**
 * @brief Returns the count of supported commands.
 * @param value Supported commands structure.
 * @return Count of commands.
 */
EXPORT_CFRDS size_t cfrds_sql_supportedcommands_count(const cfrds_sql_supportedcommands *value);

/**
 * @brief Retrieves command name string at a specific index.
 * @param value Supported commands structure.
 * @param ndx 0-based index.
 * @return Command name.
 */
EXPORT_CFRDS const char *cfrds_sql_supportedcommands_get(const cfrds_sql_supportedcommands *value, size_t ndx);

/**
 * @brief Retrieves the description/version banner of the database engine for a DSN.
 * @param server Initialized server connection.
 * @param connection_name The DSN name.
 * @param description Output pointer to the allocated description string. Must be freed with cfrds_str_cleanup.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, cfrds_str *description);

/**
 * @brief Starts a ColdFusion debugging session on the server.
 * @param server Initialized server connection.
 * @param session_id Output pointer to the allocated session ID string. Must be freed with cfrds_str_cleanup.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_start(cfrds_server *server, cfrds_str *session_id);

/**
 * @brief Stops a ColdFusion debugging session on the server.
 * @param server Initialized server connection.
 * @param session_id Active session ID string to stop.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_stop(cfrds_server *server, const char *session_id);

/**
 * @brief Retrieves debugger server host connection port configuration.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param port Output pointer to store the port integer.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_server_info(cfrds_server *server, const char *session_id, uint16_t *port);

/**
 * @brief Configures whether to trigger breakpoint traps on unhandled exception errors.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param value True to enable exception trapping, false to disable.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_breakpoint_on_exception(cfrds_server *server, const char *session_id, bool value);

/**
 * @brief Configures a breakpoint at a remote file path line location.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param filepath Remote file path.
 * @param line Line number location.
 * @param enable True to enable/set breakpoint, false to clear/disable.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_breakpoint(cfrds_server *server, const char *session_id, const char *filepath, int line, bool enable);

/**
 * @brief Clears all currently configured breakpoints in the debugger session.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_clear_all_breakpoints(cfrds_server *server, const char *session_id);

/**
 * @brief Long-polls the ColdFusion server to block and retrieve debugging events.
 * @note This is a blocking request on the server that waits until a debugger event occurs or times out.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param event Output pointer to the allocated debugger event structure. Must be freed with cfrds_debugger_event_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_debug_events(cfrds_server *server, const char *session_id, cfrds_debugger_event **event);

/**
 * @brief Long-polls server to retrieve debugging events with comprehensive detail fetch configurations enabled.
 * @note This is a blocking request on the server that waits until a debugger event occurs or times out.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param threads Retrieve thread list details.
 * @param watch Retrieve watch variables state.
 * @param scopes Retrieve variable scopes state.
 * @param cf_trace Retrieve CF trace details.
 * @param java_trace Retrieve Java trace details.
 * @param event Output pointer to the allocated event. Must be freed with cfrds_debugger_event_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_all_fetch_flags_enabled(cfrds_server *server, const char *session_id, bool threads, bool watch, bool scopes, bool cf_trace, bool java_trace, cfrds_debugger_event **event);

/**
 * @brief Frees an allocated cfrds_debugger_event structure.
 * @param event Structure to free.
 */
EXPORT_CFRDS void cfrds_debugger_event_free(cfrds_debugger_event *event);

/**
 * @brief Automatically deallocates and nullifies a cfrds_debugger_event pointer.
 * @param buf Double pointer to debugger event. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_debugger_event_cleanup(cfrds_debugger_event **buf);

/**
 * @brief Retrieves the type classification of a debugging event.
 * @param event Debugger event structure.
 * @return Type enum (e.g. BREAKPOINT, STEP, etc.).
 */
EXPORT_CFRDS cfrds_debugger_type cfrds_debugger_event_get_type(const cfrds_debugger_event *event);

/**
 * @brief Retrieves the source file path associated with a breakpoint event.
 * @param event Debugger event.
 * @return File path string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_get_source(const cfrds_debugger_event *event);

/**
 * @brief Retrieves the line number location of a breakpoint event.
 * @param event Debugger event.
 * @return Line number integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_get_line(const cfrds_debugger_event *event);

/**
 * @brief Retrieves raw WDDX node scopes variable associated with breakpoint context.
 * @param event Debugger event.
 * @return Const pointer to variable node.
 */
EXPORT_CFRDS const cfrds_variable *cfrds_debugger_event_breakpoint_get_scopes(const cfrds_debugger_event *event);

/**
 * @brief Retrieves the execution thread name of the breakpoint event.
 * @param event Debugger event.
 * @return Thread name string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_get_thread_name(const cfrds_debugger_event *event);

/**
 * @brief Retrieves the target file path where a breakpoint was confirmed or set.
 * @param event Debugger event.
 * @return File path string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_set_get_pathname(const cfrds_debugger_event *event);

/**
 * @brief Retrieves requested line number where breakpoint creation was requested.
 * @param event Debugger event.
 * @return Requested line integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_set_get_req_line(const cfrds_debugger_event *event);

/**
 * @brief Retrieves actual line number where breakpoint was bound on the server.
 * @param event Debugger event.
 * @return Actual line integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_set_get_act_line(const cfrds_debugger_event *event);

/**
 * @brief Returns scopes variable counts in event context.
 * @param event Debugger event.
 * @return Count integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_get_scopes_count(const cfrds_debugger_event *event);

/**
 * @brief Retrieves serialized scope variable detail item.
 * @param event Debugger event.
 * @param ndx 0-based index.
 * @return Detail string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_get_scopes_item(const cfrds_debugger_event *event, size_t ndx);

/**
 * @brief Returns execution thread counts inside debugger event.
 * @param event Debugger event.
 * @return Count integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_get_threads_count(const cfrds_debugger_event *event);

/**
 * @brief Retrieves execution thread identifier name string at an index.
 * @param event Debugger event.
 * @param ndx 0-based index.
 * @return Thread name string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_get_threads_item(const cfrds_debugger_event *event, size_t ndx);

/**
 * @brief Returns counts of watch variables tracked in event context.
 * @param event Debugger event.
 * @return Count integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_get_watch_count(const cfrds_debugger_event *event);

/**
 * @brief Retrieves details for a specific watch variable index.
 * @param event Debugger event.
 * @param ndx 0-based index.
 * @return Watch variable detail string.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_get_watch_item(const cfrds_debugger_event *event, size_t ndx);

/**
 * @brief Returns trace logs counts in ColdFusion event context.
 * @param event Debugger event.
 * @return Count integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_get_cf_trace_count(const cfrds_debugger_event *event);

/**
 * @brief Retrieves CF trace item detail at an index.
 * @param event Debugger event.
 * @param ndx 0-based index.
 * @return Trace log detail.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_get_cf_trace_item(const cfrds_debugger_event *event, size_t ndx);

/**
 * @brief Returns trace logs counts for Java engine context in event.
 * @param event Debugger event.
 * @return Count integer.
 */
EXPORT_CFRDS int cfrds_debugger_event_get_java_trace_count(const cfrds_debugger_event *event);

/**
 * @brief Retrieves Java trace item detail at an index.
 * @param event Debugger event.
 * @param ndx 0-based index.
 * @return Trace detail.
 */
EXPORT_CFRDS const char *cfrds_debugger_event_get_java_trace_item(const cfrds_debugger_event *event, size_t ndx);

/**
 * @brief Executes step-into debugger command on a target execution thread.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param thread_name Execution thread name identifier.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_in(cfrds_server *server, const char *session_id, const char *thread_name);

/**
 * @brief Executes step-over debugger command on a target execution thread.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param thread_name Execution thread name identifier.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_over(cfrds_server *server, const char *session_id, const char *thread_name);

/**
 * @brief Executes step-out debugger command on a target execution thread.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param thread_name Execution thread name identifier.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_out(cfrds_server *server, const char *session_id, const char *thread_name);

/**
 * @brief Resumes thread execution in a paused debugger session.
 * @param server Initialized server connection.
 * @param session_id Active session ID string.
 * @param thread_name Execution thread name identifier.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_continue(cfrds_server *server, const char *session_id, const char *thread_name);

/**
 * @brief Configures watch expression evaluation for a debugger execution context.
 * @param server Initialized server connection.
 * @param session_id Active session ID.
 * @param thread_name Thread context name.
 * @param expression Code expression string to watch/evaluate.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_watch_expression(cfrds_server *server, const char *session_id, const char *thread_name, const char *expression);

/**
 * @brief Dynamically changes variable value state in active execution context.
 * @param server Initialized server connection.
 * @param session_id Active session ID.
 * @param thread_name Thread context name.
 * @param variable Variable name path.
 * @param value New value literal to assign.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_set_variable(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable, const char *value);

/**
 * @brief Submits a comma-separated list of variables to watch in a debugger session.
 * @param server Initialized server connection.
 * @param session_id Active session ID.
 * @param variables Comma-separated variables string.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_watch_variables(cfrds_server *server, const char *session_id, const char *variables);

/**
 * @brief Retrieves standard output generated in thread execution context.
 * @param server Initialized server connection.
 * @param session_id Active session ID.
 * @param thread_name Thread context name.
 * @param output Output pointer where retrieved output text is returned.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_output(cfrds_server *server, const char *session_id, const char *thread_name, cfrds_str *output);

/**
 * @brief Configures variable scopes detail filter rules.
 * @param server Initialized server connection.
 * @param session_id Active session ID.
 * @param filter Scope filter layout settings.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_debugger_set_scope_filter(cfrds_server *server, const char *session_id, const char *filter);

/**
 * @brief Submits file path list for remote vulnerability/security scan analysis.
 * @param server Initialized server connection.
 * @param pathnames Comma-separated list of paths to scan.
 * @param recursively Scan directories recursively.
 * @param cores Parallel processing cores allocation limit.
 * @param command_id Output pointer where assigned command tracking ID is returned.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_scan(cfrds_server *server, const char *pathnames, bool recursively, int cores, int *command_id);

/**
 * @brief Cancels an active security scan task.
 * @param server Initialized server connection.
 * @param command_id Command tracking ID assigned during scan trigger.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_cancel(cfrds_server *server, int command_id);

/**
 * @brief Queries progress and metadata state of an active scan task.
 * @param server Initialized server connection.
 * @param command_id Command tracking ID.
 * @param totalfiles Output pointer for total scope files count.
 * @param filesvisitedcount Output pointer for completed files count.
 * @param percentage Output pointer for progress percentage integer.
 * @param lastupdated Output pointer for last state update timestamp.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_status(cfrds_server *server, int command_id, int *totalfiles, int *filesvisitedcount, int *percentage, int64_t *lastupdated);

/**
 * @brief Retrieves scan results report data.
 * @param server Initialized server connection.
 * @param command_id Command tracking ID.
 * @param result Output pointer to allocated result struct. Must be freed with cfrds_security_analyzer_result_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_result(cfrds_server *server, int command_id, cfrds_security_analyzer_result **result);

/**
 * @brief Cleans/destroys scan task results state stored on server.
 * @param server Initialized server connection.
 * @param command_id Command tracking ID.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_clean(cfrds_server *server, int command_id);

/**
 * @brief Frees an allocated cfrds_security_analyzer_result structure.
 * @param buf Structure to free.
 */
EXPORT_CFRDS void cfrds_security_analyzer_result_free(cfrds_security_analyzer_result *buf);

/**
 * @brief Automatically deallocates and nullifies a cfrds_security_analyzer_result pointer.
 * @param buf Double pointer to result. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_security_analyzer_result_cleanup(cfrds_security_analyzer_result **buf);

/**
 * @brief Retrieves total files count from a scan result report.
 * @param value Scan result report structure.
 * @return Count of files.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_totalfiles(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves completed files count from a scan result report.
 * @param value Scan result report.
 * @return Visited files count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesvisitedcount(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns count of error descriptions returned in scan report.
 * @param value Scan result structure.
 * @return Errors count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errorsdescription_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns count of successfully scanned files.
 * @param value Scan result.
 * @return Scanned count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesscanned_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves analysis result category status string for scanned file.
 * @param value Scan result.
 * @param ndx 0-based index.
 * @return Category classification string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesscanned_item_result(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves file path name of a scanned file.
 * @param value Scan result.
 * @param ndx 0-based index.
 * @return Path name string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesscanned_item_filename(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Returns count of files skipped/not scanned during execution.
 * @param value Scan result.
 * @return Skipped files count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesnotscanned_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves skip reason detail string for skipped file.
 * @param value Scan result.
 * @param ndx 0-based index.
 * @return Reason string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_reason(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves remote path name of a skipped file.
 * @param value Scan result.
 * @param ndx 0-based index.
 * @return Path name string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_filename(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves Executor Service backend config string description.
 * @param value Scan result.
 * @return Backend description.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_executorservice(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves scan progress percentage completion.
 * @param value Scan result.
 * @return Completion percentage (0 to 100).
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_percentage(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns count of overall files list.
 * @param value Scan result.
 * @return Count of files.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_files_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves path name value at a specific index.
 * @param value Scan result.
 * @param ndx 0-based index.
 * @return Path name.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_files_value(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves last updated timestamp of analysis task result state.
 * @param value Scan result.
 * @return Timestamp.
 */
EXPORT_CFRDS int64_t cfrds_security_analyzer_result_lastupdated(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns visited files count.
 * @param value Scan result.
 * @return Count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesvisited_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns count of skipped files.
 * @param value Scan result.
 * @return Count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesnotscannedcount(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns scanned files count.
 * @param value Scan result.
 * @return Count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_filesscannedcount(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves security scan tracking ID.
 * @param value Scan result.
 * @return Scan ID.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_id(const cfrds_security_analyzer_result *value);

/**
 * @brief Returns vulnerability error instances count in scan report.
 * @param value Scan result.
 * @return Vulnerabilities count.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_count(const cfrds_security_analyzer_result *value);

/**
 * @brief Retrieves error message for a vulnerability index.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Error message.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_errormessage(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves ending line coordinate of a vulnerability range.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Line coordinate integer.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_endline(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves directory path containing vulnerable code item.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Directory path.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_path(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves text snippet containing matching vulnerable code.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Code context snippet.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_vulnerablecode(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves file path name context for a vulnerability.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return File path string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_filename(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves start line coordinate of a vulnerability range.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Line coordinate integer.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_beginline(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves column coordinate offset for a vulnerability.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Column coordinate integer.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_column(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves detailed error description message.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Error details.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_error(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves start column coordinate offset of vulnerability range.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Column integer.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_begincolumn(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves type category of the vulnerability (e.g. SQL Injection).
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Type string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_type(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves ending column coordinate offset of vulnerability range.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Column integer.
 */
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_endcolumn(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves reference link or rule code string mapping.
 * @param value Scan result.
 * @param ndx 0-based vulnerability index.
 * @return Rule code.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_referencetype(const cfrds_security_analyzer_result *value, size_t ndx);

/**
 * @brief Retrieves status descriptor string of scanning job.
 * @param value Scan result.
 * @return Job status string.
 */
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_status(const cfrds_security_analyzer_result *value);

/**
 * @brief Internal setup check validation query simulating default IDE handshake parameters.
 * @param server Initialized server connection.
 * @param version API handshake client version.
 * @param num1 Output pointer to handshake check integer 1.
 * @param server_version Output pointer to server product description string. Must be freed with cfrds_str_cleanup.
 * @param client_version Output pointer to client API version details. Must be freed with cfrds_str_cleanup.
 * @param num2 Output pointer to handshake check integer 2.
 * @param num3 Output pointer to handshake check integer 3.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_ide_default(cfrds_server *server, int version, int *num1, cfrds_str *server_version, cfrds_str *client_version, int *num2, int *num3);

/**
 * @brief Queries system debugging log path directories configured under Admin API.
 * @param server Initialized server connection.
 * @param logdirectory Config target name key.
 * @param result Output pointer to log directory path string. Must be freed with cfrds_str_cleanup.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_debugging_getlogproperty(cfrds_server *server, const char *logdirectory, cfrds_str *result);

/**
 * @brief Queries custom tag path mappings defined on the ColdFusion server.
 * @param server Initialized server connection.
 * @param result Output pointer to custom tag paths structure. Must be freed with cfrds_adminapi_customtagpaths_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_getcustomtagpaths(cfrds_server *server, cfrds_adminapi_customtagpaths **result);

/**
 * @brief Frees custom tag paths collection structure.
 * @param buf Structure to free.
 */
EXPORT_CFRDS void cfrds_adminapi_customtagpaths_free(cfrds_adminapi_customtagpaths *buf);

/**
 * @brief Automatically deallocates and nullifies a cfrds_adminapi_customtagpaths pointer.
 * @param buf Double pointer to custom tag paths. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_adminapi_customtagpaths_cleanup(cfrds_adminapi_customtagpaths **buf);

/**
 * @brief Returns counts of custom tag paths.
 * @param buf Custom tag paths structure.
 * @return Count of paths.
 */
EXPORT_CFRDS int cfrds_adminapi_customtagpaths_count(const cfrds_adminapi_customtagpaths *buf);

/**
 * @brief Retrieves path string at a specific index.
 * @param buf Custom tag paths structure.
 * @param ndx 0-based index.
 * @return Path string.
 */
EXPORT_CFRDS const char *cfrds_adminapi_customtagpaths_at(const cfrds_adminapi_customtagpaths *buf, size_t ndx);

/**
 * @brief Adds or updates logical mapping configuration in Admin API.
 * @param server Initialized server connection.
 * @param name Logical name key mapping (e.g. "/myapp").
 * @param path Local physical server path target.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_setmapping(cfrds_server *server, const char *name, const char *path);

/**
 * @brief Deletes logical mapping configuration in Admin API.
 * @param server Initialized server connection.
 * @param mapping Logical mapping key to delete.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_deletemapping(cfrds_server *server, const char *mapping);

/**
 * @brief Queries all logical mapping configurations defined on the ColdFusion server.
 * @param server Initialized server connection.
 * @param result Output pointer to mappings structure. Must be freed with cfrds_adminapi_mappings_free.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_getmappings(cfrds_server *server, cfrds_adminapi_mappings **result);

/**
 * @brief Frees mappings collection structure.
 * @param buf Structure to free.
 */
EXPORT_CFRDS void cfrds_adminapi_mappings_free(cfrds_adminapi_mappings *buf);

/**
 * @brief Automatically deallocates and nullifies a cfrds_adminapi_mappings pointer.
 * @param buf Double pointer to mappings. Cleared to NULL after freeing.
 */
EXPORT_CFRDS void cfrds_adminapi_mappings_cleanup(cfrds_adminapi_mappings **buf);

/**
 * @brief Returns count of mappings defined.
 * @param buf Mappings structure.
 * @return Count of mappings.
 */
EXPORT_CFRDS int cfrds_adminapi_mappings_count(const cfrds_adminapi_mappings *buf);

/**
 * @brief Retrieves logical mapping key name at a specific index.
 * @param buf Mappings structure.
 * @param ndx 0-based index.
 * @return Logical name key string.
 */
EXPORT_CFRDS const char *cfrds_adminapi_mappings_key(const cfrds_adminapi_mappings *buf, size_t ndx);

/**
 * @brief Retrieves physical target path associated with mapping at index.
 * @param buf Mappings structure.
 * @param ndx 0-based index.
 * @return Physical path string.
 */
EXPORT_CFRDS const char *cfrds_adminapi_mappings_value(const cfrds_adminapi_mappings *buf, size_t ndx);

/**
 * @brief Handshakes charting graphing query to output binary image/chart.
 * @param server Initialized server connection.
 * @param out_buffer Output pointer to allocated buffer containing binary response. Must be freed with cfrds_buffer_free.
 * @param chart_attributes XML or parameter attributes describing chart styling.
 * @param num_series Count of charting data series.
 * @param series_data String array containing series item XMLs or data structures.
 * @return Status code.
 */
EXPORT_CFRDS cfrds_status cfrds_command_graphing(cfrds_server *server, cfrds_buffer **out_buffer, const char *chart_attributes, size_t num_series, const char **series_data);

#ifdef __cplusplus
}
#endif
