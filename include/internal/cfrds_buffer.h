#ifndef __CFRDS_BUFFER_H
#define __CFRDS_BUFFER_H

#include <cfrds.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef ULONG_PTR SIZE_T;
typedef LONG_PTR SSIZE_T;
typedef SIZE_T size_t;
typedef SSIZE_T ssize_t;
#endif


#ifndef EXPORT_CFRDS
 #if defined(_MSC_VER)
  #define EXPORT_CFRDS __declspec(dllexport)
 #else
  #define EXPORT_CFRDS __attribute__((visibility("default")))
 #endif
#endif

#define cfrds_buffer void


#ifdef __cplusplus
extern "C"
{
#endif

EXPORT_CFRDS bool cfrds_buffer_create(cfrds_buffer **buffer);

EXPORT_CFRDS char *cfrds_buffer_data(cfrds_buffer *buffer);
EXPORT_CFRDS size_t cfrds_buffer_data_size(cfrds_buffer *buffer);

EXPORT_CFRDS void cfrds_buffer_append(cfrds_buffer *buffer, const char *str);
EXPORT_CFRDS void cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length);
EXPORT_CFRDS void cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new);
EXPORT_CFRDS void cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch);

EXPORT_CFRDS void cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt);
EXPORT_CFRDS void cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str);
EXPORT_CFRDS void cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length);

EXPORT_CFRDS void cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size);
EXPORT_CFRDS void cfrds_buffer_expand(cfrds_buffer *buffer, size_t size);
EXPORT_CFRDS void cfrds_buffer_free(cfrds_buffer *buffer);

EXPORT_CFRDS cfrds_browse_dir_t *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer);
EXPORT_CFRDS cfrds_file_content_t *cfrds_buffer_to_file_content(cfrds_buffer *buffer);

EXPORT_CFRDS bool cfrds_buffer_skip_httpheader(char **data, size_t *size);
EXPORT_CFRDS void cfrds_buffer_file_content_free(cfrds_buffer_file_content *value);
EXPORT_CFRDS void cfrds_buffer_browse_dir_free(cfrds_buffer_browse_dir *value);

#ifdef __cplusplus
}
#endif

#endif //  __CFRDS_BUFFER_H
