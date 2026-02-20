#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


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
typedef struct cfrds_debugger_event cfrds_debugger_event;
typedef struct cfrds_security_analyzer_result cfrds_security_analyzer_result;
typedef struct cfrds_adminapi_customtagpaths cfrds_adminapi_customtagpaths;
typedef struct cfrds_adminapi_mappings cfrds_adminapi_mappings;

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

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_CFRDS void cfrds_buffer_cleanup(cfrds_buffer **buf);
EXPORT_CFRDS void cfrds_file_content_cleanup(cfrds_file_content **buf);
EXPORT_CFRDS void cfrds_str_cleanup(cfrds_str *str);

EXPORT_CFRDS bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password);
EXPORT_CFRDS void cfrds_server_free(cfrds_server *server);
EXPORT_CFRDS void cfrds_server_cleanup(cfrds_server **server);
EXPORT_CFRDS void cfrds_server_clear_error(cfrds_server *server);
EXPORT_CFRDS void cfrds_server_shutdown_socket(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_error(const cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_host(const cfrds_server *server);
EXPORT_CFRDS uint16_t cfrds_server_get_port(const cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_username(const cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_password(const cfrds_server *server);

EXPORT_CFRDS cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out);
EXPORT_CFRDS void cfrds_browse_dir_free(cfrds_browse_dir *value);
EXPORT_CFRDS void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf);
EXPORT_CFRDS size_t cfrds_browse_dir_count(const cfrds_browse_dir *value);
EXPORT_CFRDS char cfrds_browse_dir_item_get_kind(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_browse_dir_item_get_name(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS uint8_t cfrds_browse_dir_item_get_permissions(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS size_t cfrds_browse_dir_item_get_size(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS uint64_t cfrds_browse_dir_item_get_modified(const cfrds_browse_dir *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_file_read(cfrds_server *server, const char *pathname, cfrds_file_content **out);
EXPORT_CFRDS void cfrds_file_content_free(cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_file_content_get_data(const cfrds_file_content *value);
EXPORT_CFRDS int cfrds_file_content_get_size(const cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_file_content_get_modified(const cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_file_content_get_permission(const cfrds_file_content *value);

EXPORT_CFRDS cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length);
EXPORT_CFRDS cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname);
EXPORT_CFRDS cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname);
EXPORT_CFRDS cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path);
EXPORT_CFRDS cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out);
EXPORT_CFRDS cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path);
EXPORT_CFRDS cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, cfrds_str *out);

EXPORT_CFRDS cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo);
EXPORT_CFRDS void cfrds_sql_dsninfo_free(cfrds_sql_dsninfo *value);
EXPORT_CFRDS void cfrds_sql_dsninfo_cleanup(cfrds_sql_dsninfo **buf);
EXPORT_CFRDS size_t cfrds_sql_dsninfo_count(const cfrds_sql_dsninfo *value);
EXPORT_CFRDS const char *cfrds_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo);
EXPORT_CFRDS void cfrds_sql_tableinfo_free(cfrds_sql_tableinfo *value);
EXPORT_CFRDS void cfrds_sql_tableinfo_cleanup(cfrds_sql_tableinfo **buf);
EXPORT_CFRDS size_t cfrds_sql_tableinfo_count(const cfrds_sql_tableinfo *value);
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_unknown(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_schema(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_name(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_tableinfo_get_column_type(const cfrds_sql_tableinfo *value, size_t column);

EXPORT_CFRDS cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo);
EXPORT_CFRDS void cfrds_sql_columninfo_free(cfrds_sql_columninfo *value);
EXPORT_CFRDS void cfrds_sql_columninfo_cleanup(cfrds_sql_columninfo **buf);
EXPORT_CFRDS size_t cfrds_sql_columninfo_count(const cfrds_sql_columninfo *value);
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_schema(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_owner(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_table(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_name(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_type(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_columninfo_get_typeStr(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_precision(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_length(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_scale(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_radix(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_sql_columninfo_get_nullable(const cfrds_sql_columninfo *value, size_t column);

EXPORT_CFRDS cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys);
EXPORT_CFRDS void cfrds_sql_primarykeys_free(cfrds_sql_primarykeys *value);
EXPORT_CFRDS void cfrds_sql_primarykeys_cleanup(cfrds_sql_primarykeys **buf);
EXPORT_CFRDS size_t cfrds_sql_primarykeys_count(const cfrds_sql_primarykeys *value);
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_catalog(const cfrds_sql_primarykeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_owner(const cfrds_sql_primarykeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_table(const cfrds_sql_primarykeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_primarykeys_get_column(const cfrds_sql_primarykeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_primarykeys_get_key_sequence(const cfrds_sql_primarykeys *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_foreignkeys **foreignkeys);
EXPORT_CFRDS void cfrds_sql_foreignkeys_free(cfrds_sql_foreignkeys *value);
EXPORT_CFRDS void cfrds_sql_foreignkeys_cleanup(cfrds_sql_foreignkeys **buf);
EXPORT_CFRDS size_t cfrds_sql_foreignkeys_count(const cfrds_sql_foreignkeys *value);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkowner(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pktable(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_pkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkowner(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fktable(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_foreignkeys_get_fkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_key_sequence(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_updaterule(const cfrds_sql_foreignkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_foreignkeys_get_deleterule(const cfrds_sql_foreignkeys *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_importedkeys **importedkeys);
EXPORT_CFRDS void cfrds_sql_importedkeys_free(cfrds_sql_importedkeys *value);
EXPORT_CFRDS void cfrds_sql_importedkeys_cleanup(cfrds_sql_importedkeys **buf);
EXPORT_CFRDS size_t cfrds_sql_importedkeys_count(const cfrds_sql_importedkeys *value);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkcatalog(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkowner(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pktable(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_pkcolumn(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkcatalog(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkowner(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fktable(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_importedkeys_get_fkcolumn(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_importedkeys_get_key_sequence(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_importedkeys_get_updaterule(const cfrds_sql_importedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_importedkeys_get_deleterule(const cfrds_sql_importedkeys *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_exportedkeys **exportedkeys);
EXPORT_CFRDS void cfrds_sql_exportedkeys_free(cfrds_sql_exportedkeys *value);
EXPORT_CFRDS void cfrds_sql_exportedkeys_cleanup(cfrds_sql_exportedkeys **buf);
EXPORT_CFRDS size_t cfrds_sql_exportedkeys_count(const cfrds_sql_exportedkeys *value);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkowner(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pktable(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_pkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkowner(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fktable(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_exportedkeys_get_fkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_key_sequence(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_updaterule(const cfrds_sql_exportedkeys *value, size_t ndx);
EXPORT_CFRDS int cfrds_sql_exportedkeys_get_deleterule(const cfrds_sql_exportedkeys *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_resultset **resultset);
EXPORT_CFRDS void cfrds_sql_resultset_free(cfrds_sql_resultset *value);
EXPORT_CFRDS void cfrds_sql_resultset_cleanup(cfrds_sql_resultset **buf);
EXPORT_CFRDS size_t cfrds_sql_resultset_columns(const cfrds_sql_resultset *value);
EXPORT_CFRDS size_t cfrds_sql_resultset_rows(const cfrds_sql_resultset *value);
EXPORT_CFRDS const char *cfrds_sql_resultset_column_name(const cfrds_sql_resultset *value, size_t column);
EXPORT_CFRDS const char *cfrds_sql_resultset_value(const cfrds_sql_resultset *value, size_t row, size_t column);

EXPORT_CFRDS cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_metadata **metadata);
EXPORT_CFRDS void cfrds_sql_metadata_free(cfrds_sql_metadata *value);
EXPORT_CFRDS void cfrds_sql_metadata_cleanup(cfrds_sql_metadata **buf);
EXPORT_CFRDS size_t cfrds_sql_metadata_count(const cfrds_sql_metadata *value);
EXPORT_CFRDS const char *cfrds_sql_metadata_get_name(const cfrds_sql_metadata *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_metadata_get_type(const cfrds_sql_metadata *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_sql_metadata_get_jtype(const cfrds_sql_metadata *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands);
EXPORT_CFRDS void cfrds_sql_supportedcommands_free(cfrds_sql_supportedcommands *value);
EXPORT_CFRDS void cfrds_sql_supportedcommands_cleanup(cfrds_sql_supportedcommands **buf);
EXPORT_CFRDS size_t cfrds_sql_supportedcommands_count(const cfrds_sql_supportedcommands *value);
EXPORT_CFRDS const char *cfrds_sql_supportedcommands_get(const cfrds_sql_supportedcommands *value, size_t ndx);

EXPORT_CFRDS cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, cfrds_str *description);

EXPORT_CFRDS cfrds_status cfrds_command_debugger_start(cfrds_server *server, cfrds_str *session_id);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_stop(cfrds_server *server, const char *session_id);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_server_info(cfrds_server *server, const char *session_id, uint16_t *port);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_breakpoint_on_exception(cfrds_server *server, const char *session_id, bool value);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_breakpoint(cfrds_server *server, const char *session_id, const char *filepath, int line, bool enable);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_clear_all_breakpoints(cfrds_server *server, const char *session_id);

EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_debug_events(cfrds_server *server, const char *session_id, cfrds_debugger_event **event);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_all_fetch_flags_enabled(cfrds_server *server, const char *session_id, bool threads, bool watch, bool scopes, bool cf_trace, bool java_trace, cfrds_debugger_event **event);
EXPORT_CFRDS void cfrds_debugger_event_free(cfrds_debugger_event *event);
EXPORT_CFRDS void cfrds_debugger_event_cleanup(cfrds_debugger_event **buf);
EXPORT_CFRDS cfrds_debugger_type cfrds_debugger_event_get_type(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_get_source(const cfrds_debugger_event *event);
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_get_line(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_get_thread_name(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_breakpoint_set_get_pathname(const cfrds_debugger_event *event);
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_set_get_req_line(const cfrds_debugger_event *event);
EXPORT_CFRDS int cfrds_debugger_event_breakpoint_set_get_act_line(const cfrds_debugger_event *event);

EXPORT_CFRDS int cfrds_debugger_event_get_scopes_count(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_get_scopes_item(const cfrds_debugger_event *event, int ndx);
EXPORT_CFRDS int cfrds_debugger_event_get_threads_count(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_get_threads_item(const cfrds_debugger_event *event, int ndx);
EXPORT_CFRDS int cfrds_debugger_event_get_watch_count(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_get_watch_item(const cfrds_debugger_event *event, int ndx);
EXPORT_CFRDS int cfrds_debugger_event_get_cf_trace_count(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_get_cf_trace_item(const cfrds_debugger_event *event, int ndx);
EXPORT_CFRDS int cfrds_debugger_event_get_java_trace_count(const cfrds_debugger_event *event);
EXPORT_CFRDS const char *cfrds_debugger_event_get_java_trace_item(const cfrds_debugger_event *event, int ndx);

EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_in(cfrds_server *server, const char *session_id, const char *thread_name);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_over(cfrds_server *server, const char *session_id, const char *thread_name);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_step_out(cfrds_server *server, const char *session_id, const char *thread_name);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_continue(cfrds_server *server, const char *session_id, const char *thread_name);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_watch_expression(cfrds_server *server, const char *session_id, const char *thread_name, const char *expression);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_set_variable(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable, const char *value);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_watch_variables(cfrds_server *server, const char *session_id, const char *variables);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_get_output(cfrds_server *server, const char *session_id, const char *thread_name);
EXPORT_CFRDS cfrds_status cfrds_command_debugger_set_scope_filter(cfrds_server *server, const char *session_id, const char *filter);

EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_scan(cfrds_server *server, const char *pathnames, bool recursively, int cores, int *command_id);
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_cancel(cfrds_server *server, int command_id);
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_status(cfrds_server *server, int command_id, int *totalfiles, int *filesvisitedcount, int *percentage, int64_t *lastupdated);
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_result(cfrds_server *server, int command_id, cfrds_security_analyzer_result **result);
EXPORT_CFRDS cfrds_status cfrds_command_security_analyzer_clean(cfrds_server *server, int command_id);

EXPORT_CFRDS void cfrds_security_analyzer_result_free(cfrds_security_analyzer_result *buf);
EXPORT_CFRDS void cfrds_security_analyzer_result_cleanup(cfrds_security_analyzer_result **buf);
EXPORT_CFRDS int cfrds_security_analyzer_result_totalfiles(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesvisitedcount(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_errorsdescription_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesscanned_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesscanned_item_result(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesscanned_item_filename(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesnotscanned_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_reason(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_filename(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_executorservice(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_percentage(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_files_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_files_value(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int64_t cfrds_security_analyzer_result_lastupdated(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesvisited_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesnotscannedcount(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_filesscannedcount(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_id(cfrds_security_analyzer_result *value);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_count(cfrds_security_analyzer_result *value);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_errormessage(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_endline(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_path(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_vulnerablecode(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_filename(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_beginline(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_column(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_error(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_begincolumn(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_type(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS int cfrds_security_analyzer_result_errors_item_endcolumn(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_errors_item_referencetype(cfrds_security_analyzer_result *value, int ndx);
EXPORT_CFRDS cfrds_str cfrds_security_analyzer_result_status(cfrds_security_analyzer_result *value);

EXPORT_CFRDS cfrds_status cfrds_command_ide_default(cfrds_server *server, int version, int *num1, cfrds_str *server_version, cfrds_str *client_version, int *num2, int *num3);

EXPORT_CFRDS cfrds_status cfrds_command_adminapi_debugging_getlogproperty(cfrds_server *server, const char *logdirectory, cfrds_str *result);
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_getcustomtagpaths(cfrds_server *server, cfrds_adminapi_customtagpaths **result);
EXPORT_CFRDS void cfrds_adminapi_customtagpaths_free(cfrds_adminapi_customtagpaths *buf);
EXPORT_CFRDS void cfrds_adminapi_customtagpaths_cleanup(cfrds_adminapi_customtagpaths **buf);
EXPORT_CFRDS int cfrds_adminapi_customtagpaths_count(cfrds_adminapi_customtagpaths *buf);
EXPORT_CFRDS const char *cfrds_adminapi_customtagpaths_at(cfrds_adminapi_customtagpaths *buf, int ndx);
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_setmapping(cfrds_server *server, const char *name, const char *path);
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_deletemapping(cfrds_server *server, const char *mapping);
EXPORT_CFRDS cfrds_status cfrds_command_adminapi_extensions_getmappings(cfrds_server *server, cfrds_adminapi_mappings **result);
EXPORT_CFRDS void cfrds_adminapi_mappings_free(cfrds_adminapi_mappings *buf);
EXPORT_CFRDS void cfrds_adminapi_mappings_cleanup(cfrds_adminapi_mappings **buf);
EXPORT_CFRDS int cfrds_adminapi_mappings_count(cfrds_adminapi_mappings *buf);
EXPORT_CFRDS const char *cfrds_adminapi_mappings_key(cfrds_adminapi_mappings *buf, int ndx);
EXPORT_CFRDS const char *cfrds_adminapi_mappings_value(cfrds_adminapi_mappings *buf, int ndx);

#ifdef __cplusplus
}
#endif
