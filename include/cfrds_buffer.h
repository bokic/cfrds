#ifndef __CFRDS_BUFFER_H
#define __CFRDS_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define cfrds_buffer void

typedef struct {
    char *data;
    char *modified;
    char *permission;
} cfrds_buffer_file_content;

typedef struct {
    char kind;
    char *name;
    uint8_t permissions;
    size_t size;
    uint64_t modified;
} cfrds_buffer_browse_dir_item;

typedef struct {
    size_t cnt;
    cfrds_buffer_browse_dir_item items[];
} cfrds_buffer_browse_dir;


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

cfrds_buffer_browse_dir *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);
cfrds_buffer_file_content *cfrds_buffer_to_file_content(cfrds_buffer *buffer);


void cfrds_buffer_file_content_free(cfrds_buffer_file_content *value);
void cfrds_buffer_browse_dir_free(cfrds_buffer_browse_dir *value);

#endif //  __CFRDS_BUFFER_H