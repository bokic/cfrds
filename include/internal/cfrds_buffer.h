#ifndef __CFRDS_BUFFER_H
#define __CFRDS_BUFFER_H

#include "../cfrds.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define cfrds_buffer void

typedef struct {
    char *host;
    uint16_t port;
    char *username;
    char *password;
    int64_t error_code;
    char *error;
} cfrds_server_int;

typedef struct {
    char *data;
    int size;
    char *modified;
    char *permission;
} cfrds_file_content_int;

typedef struct {
    char kind;
    char *name;
    uint8_t permissions;
    size_t size;
    uint64_t modified;
} cfrds_browse_dir_item_int;

typedef struct {
    size_t cnt;
    cfrds_browse_dir_item_int items[];
} cfrds_browse_dir_int;

#ifdef __cplusplus
extern "C"
{
#endif

bool cfrds_buffer_create(cfrds_buffer **buffer);

char *cfrds_buffer_data(cfrds_buffer *buffer);
size_t cfrds_buffer_data_size(cfrds_buffer *buffer);

void cfrds_buffer_append(cfrds_buffer *buffer, const char *str);
void cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length);
void cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new);
void cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch);

void cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt);
void cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str);
void cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length);

void cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size);
void cfrds_buffer_expand(cfrds_buffer *buffer, size_t size);
void cfrds_buffer_free(cfrds_buffer *buffer);

bool cfrds_buffer_parse_number(char **data, size_t *remaining, int64_t *out);
bool cfrds_buffer_parse_bytearray(char **data, size_t *remaining, char **out, int *out_size);
bool cfrds_buffer_parse_string(char **data, size_t *remaining, char **out);

cfrds_browse_dir_int *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);
cfrds_file_content_int *cfrds_buffer_to_file_content(cfrds_buffer *buffer);

bool cfrds_buffer_skip_httpheader(char **data, size_t *size);

#ifdef __cplusplus
}
#endif

#endif //  __CFRDS_BUFFER_H
