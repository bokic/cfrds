#ifndef __CFRDS_H
#define __CFRDS_H

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
    CFRDS_STATUS_DIR_ALREADY_EXISTS,
};

typedef struct {
    char *data;
    char *modified;
    char *permission;
} cfrds_file_content_t;

typedef struct {
    char kind;
    char *name;
    uint8_t permissions;
    size_t size;
    uint64_t modified;
} cfrds_browse_dir_item_t;

typedef struct {
    size_t cnt;
    cfrds_browse_dir_item_t items[];
} cfrds_browse_dir_t;

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

EXPORT_CFRDS enum cfrds_status cfrds_browse_dir(cfrds_server *server, void *path, cfrds_browse_dir_t **out);

EXPORT_CFRDS enum cfrds_status cfrds_read_file(cfrds_server *server, void *pathname, cfrds_file_content_t **out);
EXPORT_CFRDS enum cfrds_status cfrds_write_file(cfrds_server *server, void *pathname, const void *data, size_t length);
EXPORT_CFRDS enum cfrds_status cfrds_rename(cfrds_server *server, char *current_name, char *new_name);
EXPORT_CFRDS enum cfrds_status cfrds_remove_file(cfrds_server *server, char *name);
EXPORT_CFRDS enum cfrds_status cfrds_remove_dir(cfrds_server *server, char *name);
EXPORT_CFRDS enum cfrds_status cfrds_exists(cfrds_server *server, char *pathname, bool *out);
EXPORT_CFRDS enum cfrds_status cfrds_create_dir(cfrds_server *server, char *name);
EXPORT_CFRDS enum cfrds_status cfrds_get_root_dir(cfrds_server *server);

#ifdef __cplusplus
}
#endif

#endif //  __CFRDS_H
