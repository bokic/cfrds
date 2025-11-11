#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef ULONG_PTR SIZE_T;
typedef LONG_PTR SSIZE_T;
typedef SIZE_T size_t;
typedef SSIZE_T ssize_t;
#endif


#ifdef libcfrds_EXPORTS
 #if defined(_MSC_VER)
  #define EXPORT_CFRDS __declspec(dllexport)
 #else
  #define EXPORT_CFRDS __attribute__((visibility("default")))
 #endif
#else
 #define EXPORT_CFRDS
#endif

typedef void cfrds_server;
typedef void cfrds_buffer;
typedef void cfrds_file_content;
typedef void cfrds_browse_dir;
typedef void cfrds_sql_dsninfo;
typedef void cfrds_sql_tableinfo;
typedef void cfrds_sql_columninfo;
typedef void cfrds_sql_primarykeys;
typedef void cfrds_sql_supportedcommands;

enum cfrds_status {
    CFRDS_STATUS_OK,
    CFRDS_STATUS_MEMORY_ERROR,
    CFRDS_STATUS_PARAM_IS_nullptr,
    CFRDS_STATUS_SERVER_IS_nullptr,
    CFRDS_STATUS_INDEX_OUT_OF_BOUNDS,
    CFRDS_STATUS_COMMAND_FAILED,
    CFRDS_STATUS_RESPONSE_ERROR,
    CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND,
    CFRDS_STATUS_DIR_ALREADY_EXISTS,
    CFRDS_STATUS_SOCKET_CREATION_FAILED,
    CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED,
    CFRDS_STATUS_WRITING_TO_SOCKET_FAILED,
    CFRDS_STATUS_PARTIALLY_WRITE_TO_SOCKET,
    CFRDS_STATUS_READING_FROM_SOCKET_FAILED,
};


#define cfrds_server_defer(var) cfrds_browse_dir* var __attribute__((cleanup(cfrds_server_cleanup))) = nullptr
#define cfrds_fd_defer(var) int var __attribute__((cleanup(cfrds_fd_cleanup))) = -1
#define cfrds_str_defer(var) char* var __attribute__((cleanup(cfrds_str_cleanup))) = nullptr
#define cfrds_buffer_defer(var) cfrds_buffer* var __attribute__((cleanup(cfrds_buffer_cleanup))) = nullptr
#define cfrds_browse_dir_defer(var) cfrds_browse_dir* var __attribute__((cleanup(cfrds_browse_dir_cleanup))) = nullptr

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_CFRDS void cfrds_str_cleanup(char **str);
EXPORT_CFRDS void cfrds_fd_cleanup(int *fd);
EXPORT_CFRDS void cfrds_buffer_cleanup(cfrds_buffer **buf);
EXPORT_CFRDS void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf);
EXPORT_CFRDS void cfrds_server_cleanup(cfrds_server *server);

EXPORT_CFRDS bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password);
EXPORT_CFRDS void cfrds_server_free(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_error(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_host(cfrds_server *server);
EXPORT_CFRDS uint16_t cfrds_server_get_port(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_username(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_password(cfrds_server *server);

EXPORT_CFRDS void cfrds_buffer_file_content_free(cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_buffer_file_content_get_data(const cfrds_file_content *value);
EXPORT_CFRDS int cfrds_buffer_file_content_get_size(const cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_buffer_file_content_get_modified(const cfrds_file_content *value);
EXPORT_CFRDS const char *cfrds_buffer_file_content_get_permission(const cfrds_file_content *value);

EXPORT_CFRDS void cfrds_buffer_browse_dir_free(cfrds_browse_dir *value);
EXPORT_CFRDS size_t cfrds_buffer_browse_dir_count(const cfrds_browse_dir *value);
EXPORT_CFRDS char cfrds_buffer_browse_dir_item_get_kind(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS const char *cfrds_buffer_browse_dir_item_get_name(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS uint8_t cfrds_buffer_browse_dir_item_get_permissions(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS size_t cfrds_buffer_browse_dir_item_get_size(const cfrds_browse_dir *value, size_t ndx);
EXPORT_CFRDS uint64_t cfrds_buffer_browse_dir_item_get_modified(const cfrds_browse_dir *value, size_t ndx);

EXPORT_CFRDS void cfrds_buffer_sql_dsninfo_free(cfrds_sql_dsninfo *value);
EXPORT_CFRDS size_t cfrds_buffer_sql_dsninfo_count(const cfrds_sql_dsninfo *value);
EXPORT_CFRDS const char *cfrds_buffer_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx);

EXPORT_CFRDS void cfrds_buffer_sql_tableinfo_free(cfrds_sql_tableinfo *value);
EXPORT_CFRDS size_t cfrds_buffer_sql_tableinfo_count(const cfrds_sql_tableinfo *value);
EXPORT_CFRDS const char *cfrds_buffer_sql_tableinfo_get_unknown(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_tableinfo_get_schema(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_tableinfo_get_name(const cfrds_sql_tableinfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_tableinfo_get_type(const cfrds_sql_tableinfo *value, size_t column);

EXPORT_CFRDS void cfrds_buffer_sql_columninfo_free(cfrds_sql_columninfo *value);
EXPORT_CFRDS size_t cfrds_buffer_sql_columninfo_count(const cfrds_sql_columninfo *value);
EXPORT_CFRDS const char *cfrds_buffer_sql_columninfo_get_schema(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_columninfo_get_owner(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_columninfo_get_table(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_columninfo_get_name(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_type(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS const char *cfrds_buffer_sql_columninfo_get_typeStr(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_percision(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_length(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_scale(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_radix(const cfrds_sql_columninfo *value, size_t column);
EXPORT_CFRDS int cfrds_buffer_sql_columninfo_get_nullable(const cfrds_sql_columninfo *value, size_t column);

EXPORT_CFRDS void cfrds_buffer_sql_primarykeys_free(cfrds_sql_primarykeys *value);

EXPORT_CFRDS enum cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_read(cfrds_server *server, const char *pathname, cfrds_file_content **out);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path);
EXPORT_CFRDS enum cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, char **out);

EXPORT_CFRDS enum cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo);
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo);
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo);

// TODO: Unimplemented!
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys);
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name);
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name);
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name);

EXPORT_CFRDS enum cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql);

// TODO: Unimplemented!
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql);

EXPORT_CFRDS enum cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands);

// TODO: Unimplemented!
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, const char *sql);

#ifdef __cplusplus
}
#endif
