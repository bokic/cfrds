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

#define cfrds_server void
#define cfrds_file_content void
#define cfrds_browse_dir void
#define cfrds_sql_dsninfo void
#define cfrds_sql_tableinfo void

enum cfrds_status {
    CFRDS_STATUS_OK,
    CFRDS_STATUS_MEMORY_ERROR,
    CFRDS_STATUS_PARAM_IS_NULL,
    CFRDS_STATUS_SERVER_IS_NULL,
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

#ifdef __cplusplus
extern "C"
{
#endif

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
EXPORT_CFRDS const char *cfrds_buffer_sql_tableinfo_field(const cfrds_sql_tableinfo *value, int row, int field);

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
EXPORT_CFRDS enum cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql);

#ifdef __cplusplus
}
#endif
