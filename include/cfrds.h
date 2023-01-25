#ifndef __CFRDS_H
#define __CFRDS_H

#include <cfrds_buffer.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#ifndef EXPORT_CFRDS
 #if defined(_MSC_VER)
  #define EXPORT_CFRDS __declspec(dllexport)
 #else
  #define EXPORT_CFRDS __attribute__((visibility("default")))
 #endif
#endif

#define cfrds_server void
#define cfrds_result void

enum cfrds_status {
    CFRDS_STATUS_OK,
    CFRDS_STATUS_PARAM_IS_NULL,
    CFRDS_STATUS_SERVER_IS_NULL,
    CFRDS_STATUS_COMMAND_FAILED,
    CFRDS_STATUS_RESPONSE_ERROR,
};

#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_CFRDS bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password);
EXPORT_CFRDS void cfrds_server_free(cfrds_server *server);
EXPORT_CFRDS void cfrds_server_set_error(cfrds_server *server, const char *error);
EXPORT_CFRDS const char *cfrds_server_get_error(cfrds_server *server);
EXPORT_CFRDS const char *cfrds_server_get_host(cfrds_server *server);
EXPORT_CFRDS uint16_t cfrds_server_get_port(cfrds_server *server);

EXPORT_CFRDS enum cfrds_status cfrds_browse_dir(cfrds_server *server, void *path, cfrds_buffer_browse_dir **out);

EXPORT_CFRDS enum cfrds_status cfrds_read_file(cfrds_server *server, void *pathname, cfrds_buffer_file_content **out);
EXPORT_CFRDS enum cfrds_status cfrds_write_file(cfrds_server *server, void *pathname, const void *data, size_t length);
EXPORT_CFRDS enum cfrds_status cfrds_rename(cfrds_server *server, char *current_name, char *new_name);
EXPORT_CFRDS enum cfrds_status cfrds_remove_file(cfrds_server *server, char *name);
EXPORT_CFRDS enum cfrds_status cfrds_remove_dir(cfrds_server *server, char *name);
EXPORT_CFRDS enum cfrds_status cfrds_exists(cfrds_server *server, char *pathname);
EXPORT_CFRDS enum cfrds_status cfrds_create_dir(cfrds_server *server, char *name);

#ifdef __cplusplus
}
#endif

#endif //  __CFRDS_H
