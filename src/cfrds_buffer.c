//#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#include <cfrds.h>
#include <internal/cfrds_buffer.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>


typedef struct {
    size_t allocated;
    size_t size;
    size_t offset;
    char *string;
} cfrds_buffer_int;

void cfrds_buffer_realloc_if_needed(cfrds_buffer *buffer, size_t len)
{
    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->size + len > buffer_int->allocated)
    {
        size_t new_size = buffer_int->size + len;
        new_size = (((new_size + 512) / 1024) + 1) * 1024;

        buffer_int->string = realloc(buffer_int->string, new_size);
        buffer_int->allocated = new_size;
    }
}

bool cfrds_buffer_create(cfrds_buffer **buffer)
{
    cfrds_buffer_int *ret = NULL;

    if (buffer == NULL)
         return false;

    ret = malloc(sizeof(cfrds_buffer_int));
    if (ret == NULL)
        return false;

    ret->allocated = 0;
    ret->size = 0;
    ret->offset = 0;
    ret->string = NULL;

    *buffer = ret;

    return true;
}

char *cfrds_buffer_data(cfrds_buffer *buffer)
{
    cfrds_buffer_int *ret = NULL;

    if (buffer == NULL)
         return NULL;

    ret = buffer;

    return ret->string;
}

size_t cfrds_buffer_data_size(cfrds_buffer *buffer)
{
    const cfrds_buffer_int *ret = NULL;

    if (buffer == NULL)
         return false;

    ret = buffer;

    return ret->size;
}

void cfrds_buffer_append(cfrds_buffer *buffer, const char *str)
{
    cfrds_buffer_int *buffer_int = buffer;
    size_t len = 0;

    if ((!buffer_int)||(!str))
    {
         return;
    }

    len = strlen(str);

    cfrds_buffer_realloc_if_needed(buffer_int, len);

    if (len > 0)
    {
        memcpy(&buffer_int->string[buffer_int->size], str, len);
        buffer_int->size += len;
    }
}

void cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    cfrds_buffer_int *buffer_int = buffer;

    if ((!buffer_int)||(!data)||(length == 0))
    {
        return;
    }

    cfrds_buffer_realloc_if_needed(buffer_int, length);

    memcpy(&buffer_int->string[buffer_int->size], data, length);
    buffer_int->size += length;
}

void cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new)
{
    cfrds_buffer_int *buffer_int = buffer;
    const cfrds_buffer_int *new_int = new;
    size_t len = new_int->size;

    cfrds_buffer_realloc_if_needed(buffer_int, len);

    memcpy(&buffer_int->string[buffer_int->size], new_int->string, len);
    buffer_int->size += len;
}

void cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt)
{
    char str_cnt[16];

    snprintf(str_cnt, sizeof(str_cnt), "%zu", cnt);

    cfrds_buffer_append(buffer, str_cnt);
    cfrds_buffer_append_char(buffer, ':');
}

void cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str)
{
    char str_len[32];
    size_t len = 0;

    if ((!buffer)||(!str))
    {
        return;
    }

    len = strlen(str);
    snprintf(str_len, sizeof(str_len), "%zu", len);

    cfrds_buffer_append(buffer, "STR:");
    cfrds_buffer_append(buffer, str_len);
    cfrds_buffer_append_char(buffer, ':');
    cfrds_buffer_append(buffer, str);
}

void cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    char str_len[32];

    snprintf(str_len, sizeof(str_len), "%zu", length);

    cfrds_buffer_append(buffer, "STR:");
    cfrds_buffer_append(buffer, str_len);
    cfrds_buffer_append_char(buffer, ':');
    cfrds_buffer_append_bytes(buffer, data, length);
}

void cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch)
{
    cfrds_buffer_int *buffer_int = buffer;

    cfrds_buffer_realloc_if_needed(buffer_int, 1);

    buffer_int->string[buffer_int->size] = ch;
    buffer_int->size++;
}

void cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size)
{
    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->allocated - buffer_int->size < size)
    {
        size_t new_size = buffer_int->size + size;

        buffer_int->string = realloc(buffer_int->string, new_size);
        buffer_int->allocated = new_size;
    }
}

void cfrds_buffer_expand(cfrds_buffer *buffer, size_t size)
{
    cfrds_buffer_int *buffer_int = buffer;

    buffer_int->size += size;
}

void cfrds_buffer_free(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return;

    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->string != NULL)
    {
        free(buffer_int->string);
        buffer_int->string = NULL;
    }

    free(buffer);
}

bool cfrds_buffer_parse_number(char **data, size_t *remaining, int64_t *out)
{
    char *end = NULL;

    if (data == NULL)
        return false;

    end = strchr(*data, ':');
    if ((end == NULL)||((unsigned)(end - *data) > *remaining))
        return false;

    *out = atol(*data);

    *remaining -= end - *data + 1;
    *data = end + 1;

    return true;
}

bool cfrds_buffer_parse_bytearray(char **data, size_t *remaining, char **out, int *out_size)
{
    int64_t size = 0;

    if (out == NULL)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &size))
        return false;

    if (size < 0)
        return false;

    *out = malloc(size + 1);
    memcpy(*out, *data, size);
    (*out)[size] = 0;

    *remaining -= size;
    *data += size;

    *out_size = size;

    return true;
}

bool cfrds_buffer_parse_string(char **data, size_t *remaining, char **out)
{
    int64_t str_size = 0;

    if (out == NULL)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &str_size))
        return false;

    if (str_size <= 0)
        return false;

    *out = malloc(str_size + 1);
    memcpy(*out, *data, str_size);
    (*out)[str_size] = 0;

    *remaining -= str_size;
    *data += str_size;

    return true;
}

bool cfrds_buffer_skip_httpheader(char **data, size_t *remaining)
{
    char *body = NULL;

    body = strstr(*data, "\r\n\r\n");
    if (body == NULL)
        return false;

    *remaining -= body - *data;
    *data = body + 4;
    *remaining -= 4;

    return true;
}

cfrds_browse_dir_int *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer)
{
    cfrds_buffer_int *buffer_int = buffer;
    int64_t total = 0;
    int64_t cnt = 0;

    if (buffer_int == NULL)
        return NULL;

    char *data = buffer_int->string;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return NULL;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return NULL;

    if ((total <= 0)||(total % 5))
        return NULL;

    cnt = total / 5;

    size_t ret_size = offsetof(cfrds_browse_dir_int, items) + (cnt * (offsetof(cfrds_browse_dir_int, items[1]) - (offsetof(cfrds_browse_dir_int, items[0]))));
    cfrds_browse_dir_int *ret = malloc(ret_size);
    memset(ret, 0, ret_size);

    ret->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        char *str_kind = NULL;
        char *filename = NULL;
        char *str_permissions = NULL;
        char *str_filesize = NULL;
        char *str_timestamp = NULL;

        char file_type = '\0';
        ssize_t permissions = -1;
        ssize_t filesize = -1;
        uint64_t modified = -1;

        cfrds_buffer_parse_string(&data, &size, &str_kind);
        cfrds_buffer_parse_string(&data, &size, &filename);
        cfrds_buffer_parse_string(&data, &size, &str_permissions);
        cfrds_buffer_parse_string(&data, &size, &str_filesize);
        cfrds_buffer_parse_string(&data, &size, &str_timestamp);

        if (str_kind)
        {
            if (strcmp(str_kind, "F:") == 0) file_type = 'F';
            else if (strcmp(str_kind, "D:") == 0) file_type = 'D';

            free(str_kind);
            str_kind = NULL;
        }

        if (str_permissions)
        {
            permissions = atol(str_permissions);

            free(str_permissions);
            str_permissions = NULL;
        }

        if (str_filesize)
        {
            filesize = atol(str_filesize);

            free(str_filesize);
            str_filesize = NULL;
        }

        if (str_timestamp)
        {
            uint32_t num1 = atol(str_timestamp);

            const char *str_num2 = strchr(str_timestamp, ',');
            if (str_num2)
            {
                str_num2++;
                uint32_t num2 = atol(str_num2);
                modified = num1 + ((uint64_t)num2 << 32);
                modified /= 10000;
                modified -= 11644473600000L;
            }

            free(str_timestamp);
            str_filesize = NULL;
        }

        if(((file_type != 'D')&&(file_type != 'F'))||(!filename)||(permissions < 0)||(permissions > 0xff)||(filesize < 0))
        {
            free(filename);

            cfrds_buffer_browse_dir_free(ret);

            return NULL;
        }

        ret->items[c].kind = file_type;
        ret->items[c].name = filename;
        ret->items[c].permissions = permissions;
        ret->items[c].size = filesize;
        ret->items[c].modified = modified;
    }

    return ret;
}

cfrds_file_content_int *cfrds_buffer_to_file_content(cfrds_buffer *buffer)
{
    cfrds_file_content_int *ret = NULL;
    cfrds_buffer_int *buffer_int = buffer;
    int64_t total = 0;

    if (buffer_int == NULL)
        return NULL;

    char *data = buffer_int->string;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return NULL;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return NULL;

    if (total != 3)
        return NULL;

    ret = malloc(sizeof(cfrds_file_content_int));

    cfrds_buffer_parse_bytearray(&data, &size, &ret->data, &ret->size);
    cfrds_buffer_parse_string(&data, &size, &ret->modified);
    cfrds_buffer_parse_string(&data, &size, &ret->permission);

    return ret;
}
