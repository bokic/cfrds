// cppcheck-suppress-file unusedFunction
// cppcheck-suppress-file staticFunction
#include <internal/explicit_bzero.h>


#include <internal/cfrds_buffer.h>
#include <internal/wddx.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>

#define CFRDS_MAX_PARSER_ITEMS 10000

static bool cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch);


struct cfrds_buffer {
    size_t allocated;
    size_t size;
    uint8_t *data;
};


void cfrds_str_cleanup(cfrds_str *str) {
    if (*str) {
        free(*str);
        *str = NULL;
    }
}

void cfrds_buffer_cleanup(cfrds_buffer **buf) {
    if (*buf) {
        cfrds_buffer_free(*buf);
        *buf = NULL;
    }
}

void cfrds_file_content_cleanup(cfrds_file_content **buf) {
    if (*buf) {
        cfrds_file_content_free(*buf);
        *buf = NULL;
    }
}

void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf) {
    if (*buf) {
        cfrds_browse_dir_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_dsninfo_cleanup(cfrds_sql_dsninfo **buf) {
    if (*buf) {
        cfrds_sql_dsninfo_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_tableinfo_cleanup(cfrds_sql_tableinfo **buf) {
    if (*buf) {
        cfrds_sql_tableinfo_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_columninfo_cleanup(cfrds_sql_columninfo **buf) {
    if (*buf) {
        cfrds_sql_columninfo_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_primarykeys_cleanup(cfrds_sql_primarykeys **buf) {
    if (*buf) {
        cfrds_sql_primarykeys_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_foreignkeys_cleanup(cfrds_sql_foreignkeys **buf) {
    if (*buf) {
        cfrds_sql_foreignkeys_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_importedkeys_cleanup(cfrds_sql_importedkeys **buf) {
    if (*buf) {
        cfrds_sql_importedkeys_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_exportedkeys_cleanup(cfrds_sql_exportedkeys **buf) {
    if (*buf) {
        cfrds_sql_exportedkeys_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_resultset_cleanup(cfrds_sql_resultset **buf) {
    if (*buf) {
        cfrds_sql_resultset_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_metadata_cleanup(cfrds_sql_metadata **buf) {
    if (*buf) {
        cfrds_sql_metadata_free(*buf);
        *buf = NULL;
    }
}

void cfrds_sql_supportedcommands_cleanup(cfrds_sql_supportedcommands **buf) {
    if (*buf) {
        cfrds_sql_supportedcommands_free(*buf);
        *buf = NULL;
    }
}

void cfrds_debugger_event_cleanup(cfrds_debugger_event **buf) {
    if (*buf) {
        cfrds_debugger_event_free(*buf);
        *buf = NULL;
    }
}

static bool cfrds_buffer_realloc_if_needed(cfrds_buffer *buffer, size_t len)
{
    void *tmp = NULL;

    if (buffer == NULL)
        return false;

    if (SIZE_MAX - buffer->size < len)
        return false;

    size_t required = buffer->size + len;

    if (required > buffer->allocated)
    {
        if (SIZE_MAX - 512 < required)
            return false;

        size_t newsize = (((required + 512) / 1024) + 1) * 1024;
        if (newsize < required || newsize == SIZE_MAX)
            return false;

        /* +1: always keep a null sentinel byte past the data */
        tmp = realloc(buffer->data, newsize + 1);
        if (tmp == NULL)
            return false;

        size_t oldsize = buffer->allocated;
        buffer->data = tmp;
        explicit_bzero(buffer->data + oldsize, newsize + 1 - oldsize);
        buffer->allocated = newsize;
    }

    return true;
}

bool cfrds_buffer_create(cfrds_buffer **buffer)
{
    cfrds_buffer *tmp = NULL;

    if (buffer == NULL)
         return false;

    tmp = malloc(sizeof(cfrds_buffer));
    if (tmp == NULL)
        return false;

    tmp->allocated = 0;
    tmp->size = 0;
    tmp->data = NULL;

    *buffer = tmp;

    return true;
}

char *cfrds_buffer_data(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    return (char *)buffer->data;
}

size_t cfrds_buffer_data_size(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return 0;

    return buffer->size;
}

bool cfrds_buffer_append(cfrds_buffer *buffer, const char *str)
{
    size_t len = 0;

    if ((!buffer)||(!str))
    {
         return false;
    }

    len = strlen(str);

    if (cfrds_buffer_realloc_if_needed(buffer, len) == false)
    {
        return false;
    }

    if (len > 0)
    {
        memcpy(&buffer->data[buffer->size], str, len);
        buffer->size += len;
    }

    return true;
}

static __attribute__((unused)) bool cfrds_buffer_append_int(cfrds_buffer *buffer, int number)
{
    char str[16];

    if (buffer == NULL)
        return false;

    snprintf(str, sizeof(str), "%d", number);

    if (cfrds_buffer_append(buffer, str) == false) return false;

    return true;
}

bool cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    if ((!buffer)||(!data)||(length == 0))
    {
        return false;
    }

    if (cfrds_buffer_realloc_if_needed(buffer, length) == false)
    {
        return false;
    }

    memcpy(&buffer->data[buffer->size], data, length);
    buffer->size += length;

    return true;
}

bool cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new)
{
    if ((buffer == NULL)||(new == NULL))
        return false;

    size_t len = new->size;

    if (cfrds_buffer_realloc_if_needed(buffer, len) == false)
    {
        return false;
    }

    memcpy(&buffer->data[buffer->size], new->data, len);
    buffer->size += len;

    return true;
}

bool cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt)
{
    char str[32] = {0, };
    int n = 0;

    if (buffer == NULL)
        return false;

    n = snprintf(str, sizeof(str), "%zu", cnt);
    if (n > 0)
    {
        if (cfrds_buffer_append(buffer, str) == false) return false;
        if (cfrds_buffer_append_char(buffer, ':') == false) return false;
    }

    return true;
}

bool cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str)
{
    char str_len[16] = {0, };
    size_t len = 0;
    int n = 0;

    if ((!buffer)||(!str))
    {
        return false;
    }

    len = strlen(str);
    n = snprintf(str_len, sizeof(str_len), "%zu", len);
    if (n > 0)
    {
        if (cfrds_buffer_append(buffer, "STR:") == false) return false;
        if (cfrds_buffer_append(buffer, str_len) == false) return false;
        if (cfrds_buffer_append_char(buffer, ':') == false) return false;
        if (cfrds_buffer_append(buffer, str) == false) return false;
    }

    return true;
}

bool cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    char str_len[16] = {0, };
    int n = 0;

    if ((buffer == NULL)||(data == NULL))
        return false;

    n = snprintf(str_len, sizeof(str_len), "%zu", length);
    if (n > 0)
    {
        if (cfrds_buffer_append(buffer, "STR:") == false) return false;
        if (cfrds_buffer_append(buffer, str_len) == false) return false;
        if (cfrds_buffer_append_char(buffer, ':') == false) return false;
        if (cfrds_buffer_append_bytes(buffer, data, length) == false) return false;
    }

    return true;
}

static bool cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch)
{
    if (buffer == NULL)
        return false;

    if (cfrds_buffer_realloc_if_needed(buffer, 1) == false)
    {
        return false;
    }

    buffer->data[buffer->size] = (uint8_t)ch;
    buffer->size++;

    return true;
}

bool cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size)
{
    void *tmp = NULL;

    if (buffer == NULL)
        return false;

    if (SIZE_MAX - buffer->size < size)
        return false;

    size_t required = buffer->size + size;

    if (buffer->allocated < required)
    {
        if (required == SIZE_MAX)
            return false;

        size_t newsize = required;

        tmp = realloc(buffer->data, newsize + 1);
        if (tmp == NULL)
            return false;

        size_t oldsize = buffer->allocated;
        buffer->data = tmp;
        explicit_bzero(buffer->data + oldsize, newsize + 1 - oldsize);
        buffer->allocated = newsize;
    }

    return true;
}

bool cfrds_buffer_expand(cfrds_buffer *buffer, size_t size)
{
    if (buffer == NULL)
        return false;

    if (SIZE_MAX - buffer->size < size)
        return false;

    if (buffer->allocated - buffer->size < size)
    {
        if (cfrds_buffer_reserve_above_size(buffer, size) == false)
            return false;
    }

    buffer->size += size;

    return true;
}

void cfrds_buffer_free(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return;

    if (buffer->data != NULL)
    {
        free(buffer->data);
        buffer->data = NULL;
    }

    free(buffer);
}

bool cfrds_buffer_parse_number(const char **data, size_t *remaining, int64_t *out)
{
    const char *end = NULL;
    char *endptr = NULL;

    if (data == NULL)
        return false;

    end = memchr(*data, ':', *remaining);
    if (end == NULL)
        return false;

    errno = 0;
    *out = strtoll(*data, &endptr, 10);
    if (endptr != end)
        return false;
    if (errno == ERANGE)
        return false;

    *remaining -= (size_t)(end - *data + 1);
    *data = end + 1;

    return true;
}

static bool cfrds_buffer_parse_bytearray(const char **data, size_t *remaining, char **out, size_t *out_size)
{
    size_t size = 0;
    int64_t tmp = 0;

    if (out == NULL)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &tmp))
        return false;

    if (tmp < 0)
        return false;

    size = (size_t)tmp;

    if (size > *remaining)
        return false;

    *out = malloc(size + 1);
    if (*out == NULL)
        return false;

    memcpy(*out, *data, size);
    (*out)[size] = 0;

    *remaining -= size;
    *data += size;

    *out_size = size;

    return true;
}

bool cfrds_buffer_parse_string(const char **data, size_t *remaining, char **out)
{
    size_t size = 0;
    int64_t tmp = 0;

    if (out == NULL)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &tmp))
        return false;

    if (tmp < 0)
        return false;

    size = (size_t)tmp;

    if (size > *remaining)
        return false;

    *out = malloc(size + 1);
    if(*out == NULL)
        return false;

    memcpy(*out, *data, size);
    (*out)[size] = 0;

    *remaining -= size;
    *data += size;

    return true;
}

static bool cfrds_buffer_parse_string_list_item(const char **data, size_t *remaining, char **out)
{
    bool with_quotes = false;
    const char *endstr = NULL;
    cfrds_str_defer(tmp);
    size_t len = 0;

    if ((data == NULL)||(out == NULL)||(*remaining < 2))
        return false;

    if ((*data)[0] == '"')
    {
        with_quotes = true;
        (*data)++; (*remaining)--;

        endstr = memchr(*data, '"', *remaining);
        if (endstr == NULL)
            return false;
        len = (size_t)(endstr - *data);
    }
    else
    {
        endstr = memchr(*data, ',', *remaining);
        if (endstr == NULL)
            len = *remaining;
        else
            len = (size_t)(endstr - *data);
    }

    tmp = malloc(len + 1);
    if (tmp == NULL)
        return false;

    memcpy(tmp, *data, len);
    tmp[len] = '\0';
    (*data)+=len; (*remaining)-=len;
    if (with_quotes) {
        if (*remaining == 0 || (*data)[0] != '"') {
            return false;
        }

        (*data)++; (*remaining)--;
    }
    if (*remaining > 0 && (*data)[0] == ',') {
        (*data)++; (*remaining)--;
    }

    *out = tmp; tmp = NULL;

    return true;
}

cfrds_browse_dir *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer)
{
    cfrds_browse_dir *ret = NULL;

    cfrds_browse_dir_defer(tmp);
    size_t malloc_size = 0;
    int64_t total = 0;
    int64_t cnt = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return NULL;

    if ((total < 0)||(total != 0 && total % 5))
        return NULL;

    cnt = total / 5;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    malloc_size = offsetof(cfrds_browse_dir, items) + ucnt * sizeof(cfrds_browse_dir_item);

    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(str_kind);
        cfrds_str_defer(filename);
        cfrds_str_defer(str_permissions);
        cfrds_str_defer(str_filesize);
        cfrds_str_defer(str_timestamp);

        char file_type = '\0';
        ssize_t permissions = -1;
        ssize_t filesize = -1;
        uint64_t modified = UINT64_MAX;

        if (!cfrds_buffer_parse_string(&data, &size, &str_kind))
            return NULL;
        if (!cfrds_buffer_parse_string(&data, &size, &filename))
            return NULL;
        if (!cfrds_buffer_parse_string(&data, &size, &str_permissions))
            return NULL;
        if (!cfrds_buffer_parse_string(&data, &size, &str_filesize))
            return NULL;
        if (!cfrds_buffer_parse_string(&data, &size, &str_timestamp))
            return NULL;

        if (str_kind)
        {
            if (strcmp(str_kind, "F:") == 0)
              file_type = 'F';
            else
              if (strcmp(str_kind, "D:") == 0)
                file_type = 'D';
        }

        if (str_permissions)
            permissions = atol(str_permissions);

        if (str_filesize)
            filesize = atol(str_filesize);

        if (str_timestamp)
        {
            uint32_t num1 = (uint32_t)strtoul(str_timestamp, NULL, 10);

            const char *str_num2 = strchr(str_timestamp, ',');
            if (str_num2)
            {
                str_num2++;
                uint32_t num2 = (uint32_t)atol(str_num2);
                modified = num1 + ((uint64_t)num2 << 32);
                modified /= 10000;
                modified -= 11644473600000L;
            }
        }

        if(((file_type != 'D')&&(file_type != 'F'))||(!filename)||(permissions < 0)||(permissions > 0xff)||(filesize < 0))
            return NULL;

        tmp->items[c].kind = file_type;
        tmp->items[c].name = filename; filename = NULL;
        tmp->items[c].permissions = (uint8_t)permissions;
        tmp->items[c].size = (size_t)filesize;
        tmp->items[c].modified = modified;
    }

    ret = tmp; tmp = NULL;
    return ret;
}

cfrds_file_content *cfrds_buffer_to_file_content(cfrds_buffer *buffer)
{
    cfrds_file_content *ret = NULL;
    cfrds_file_content_defer(tmp);
    int64_t total = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return NULL;

    if (total != 3)
        return NULL;

    tmp = malloc(sizeof(cfrds_file_content));
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, sizeof(cfrds_file_content));

    if (!cfrds_buffer_parse_bytearray(&data, &size, &tmp->data, &tmp->size) ||
        !cfrds_buffer_parse_string(&data, &size, &tmp->modified) ||
        !cfrds_buffer_parse_string(&data, &size, &tmp->permission))
    {
        return NULL;
    }

    ret = tmp;
    tmp = NULL;

    return ret;
}

cfrds_sql_dsninfo *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer)
{
    cfrds_sql_dsninfo *ret = NULL;

    cfrds_sql_dsninfo_defer(tmp);

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

      int64_t cnt = 0;

      if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
      {
          return NULL;
      }

      if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
          return NULL;

      size_t ucnt = (size_t)cnt;
      size_t malloc_size = offsetof(cfrds_sql_dsninfo, names) + sizeof(char *) * ucnt;
      tmp = malloc(malloc_size);
      if (tmp == NULL)
          return NULL;

      explicit_bzero(tmp, malloc_size);

      tmp->cnt = ucnt;

      for(int c = 0; c < cnt; c++)
      {
          cfrds_str_defer(item);

          cfrds_buffer_parse_string(&response_data, &response_size, &item);

          if (item)
          {
              const char *pos1 = strchr(item, '"');
              if (pos1)
              {
                  const char *pos2 = strchr(pos1 + 1, '"');
                  if (pos2)
                  {
                      size_t len = (size_t)(pos2 - pos1 - 1);

                      char *str_tmp = malloc(len + 1);
                      if (str_tmp == NULL)
                          return NULL;

                      memcpy(str_tmp, pos1 + 1, len);
                      str_tmp[len] = '\0';
                      free(item);
                      item = str_tmp;
                  }
              }

              tmp->names[c] = item; item = NULL;
          }
      }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_tableinfo *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer)
{
    cfrds_sql_tableinfo *ret = NULL;

    cfrds_sql_tableinfo_defer(tmp);

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
    {
        return NULL;
    }

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t malloc_size = offsetof(cfrds_sql_tableinfo, items) + sizeof(cfrds_sql_tableinfoitem) * (size_t)cnt;

    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = 0;

    for(int c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&response_data, &response_size, &item);
        if (item)
        {
            cfrds_str_defer(field1);
            cfrds_str_defer(field2);
            cfrds_str_defer(field3);
            cfrds_str_defer(field4);

            const char *current_item = item;
            const char *end_item = NULL;

            if (current_item[0] != '"')
                return NULL;
            current_item++;
            end_item = strchr(current_item, '"');
            if (!end_item)
                return NULL;
            if (end_item >= current_item) {
                size_t size = (size_t)(end_item - current_item);

                field1 = malloc(size + 1);
                if (field1 == NULL)
                    return NULL;

                memcpy(field1, current_item, size);
                field1[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return NULL;
            current_item++;

            if (current_item[0] != '"')
                return NULL;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return NULL;

            if (end_item >= current_item) {
                size_t size = (size_t)(end_item - current_item);

                field2 = malloc(size + 1);
                if (!field2)
                    return NULL;

                memcpy(field2, current_item, size);
                field2[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return NULL;
            current_item++;

            if (current_item[0] != '"')
                return NULL;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return NULL;

            if (end_item >= current_item) {
                size_t size = (size_t)(end_item - current_item);

                field3 = malloc(size + 1);
                if (!field3)
                    return NULL;

                memcpy(field3, current_item, size);
                field3[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return NULL;
            current_item++;

            if (current_item[0] != '"')
                return NULL;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return NULL;

            if (end_item >= current_item) {
                size_t size = (size_t)(end_item - current_item);

                field4 = malloc(size + 1);
                if (!field4)
                    return NULL;

                memcpy(field4, current_item, size);
                field4[size] = '\0';
            }

            tmp->items[tmp->cnt].unknown = field1; field1 = NULL;
            tmp->items[tmp->cnt].schema  = field2; field2 = NULL;
            tmp->items[tmp->cnt].name    = field3; field3 = NULL;
            tmp->items[tmp->cnt].type    = field4; field4 = NULL;
            tmp->cnt++;
        }
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_columninfo *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer)
{
    cfrds_sql_columninfo *ret = NULL;

    cfrds_sql_columninfo_defer(tmp);

    int64_t columns = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &columns))
        return NULL;

    if (columns < 0 || columns > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucolumns = (size_t)columns;
    size_t malloc_size = offsetof(cfrds_sql_columninfo, items) + sizeof(cfrds_sql_columninfoitem) * ucolumns;
    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);
    tmp->cnt = ucolumns;

    for(int64_t column = 0; column < columns; column++)
    {
        cfrds_str_defer(row_buf);

        if (!cfrds_buffer_parse_string(&data, &size, &row_buf))
            return NULL;

        const char *column_buf = row_buf;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(field1);
        cfrds_str_defer(field2);
        cfrds_str_defer(field3);
        cfrds_str_defer(field4);
        cfrds_str_defer(field5);
        cfrds_str_defer(field6);
        cfrds_str_defer(field7);
        cfrds_str_defer(field8);
        cfrds_str_defer(field9);
        cfrds_str_defer(field10);
        cfrds_str_defer(field11);
        cfrds_str_defer(field12);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field1))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field2))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field3))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field4))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field5))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field6))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field7))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field8))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field9))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field10))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field11))
            return NULL;
        if (list_remaining > 0)
        {
            if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field12))
                return NULL;
        }
        if (list_remaining != 0)
            return NULL;

        tmp->items[column].schema    = field1; field1 = NULL;
        tmp->items[column].owner     = field2; field2 = NULL;
        tmp->items[column].table     = field3; field3 = NULL;
        tmp->items[column].name      = field4; field4 = NULL;
        tmp->items[column].type      = atoi(field5);
        tmp->items[column].typeStr   = field6; field6 = NULL;
        tmp->items[column].precision = atoi(field7);
        tmp->items[column].length    = atoi(field8);
        tmp->items[column].scale     = atoi(field9);
        tmp->items[column].radix     = atoi(field10);
        tmp->items[column].nullable  = atoi(field11);
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_primarykeys *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer)
{
    cfrds_sql_primarykeys *ret = NULL;

    cfrds_sql_primarykeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_primarykeys, items) + sizeof(cfrds_sql_primarykeysitem) * ucnt;
    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == NULL)
            return NULL;

        const char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(tableCatalog);
        cfrds_str_defer(tableOwner);
        cfrds_str_defer(tableName);
        cfrds_str_defer(colName);
        cfrds_str_defer(keySequence);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &colName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence))
            return NULL;
        if (list_remaining != 0)
            return NULL;

        tmp->items[c].tableCatalog = tableCatalog; tableCatalog = NULL;
        tmp->items[c].tableOwner   = tableOwner; tableOwner = NULL;
        tmp->items[c].tableName    = tableName; tableName = NULL;
        tmp->items[c].colName      = colName; colName = NULL;
        tmp->items[c].keySequence  = atoi(keySequence);
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_foreignkeys *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer)
{
    cfrds_sql_foreignkeys *ret = NULL;

    cfrds_sql_foreignkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_foreignkeys, items) + sizeof(cfrds_sql_foreignkeysitem) * ucnt;
    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == NULL)
            return NULL;

        const char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(pkTableCatalog);
        cfrds_str_defer(pkTableOwner);
        cfrds_str_defer(pkTableName);
        cfrds_str_defer(pkColName);
        cfrds_str_defer(fkTableCatalog);
        cfrds_str_defer(fkTableOwner);
        cfrds_str_defer(fkTableName);
        cfrds_str_defer(fkColName);
        cfrds_str_defer(keySequence);
        cfrds_str_defer(updateRule);
        cfrds_str_defer(deleteRule);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule))
            return NULL;
        if (list_remaining != 0)
            return NULL;

        tmp->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = NULL;
        tmp->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = NULL;
        tmp->items[c].pkTableName    = pkTableName; pkTableName = NULL;
        tmp->items[c].pkColName      = pkColName; pkColName = NULL;
        tmp->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = NULL;
        tmp->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = NULL;
        tmp->items[c].fkTableName    = fkTableName; fkTableName = NULL;
        tmp->items[c].fkColName      = fkColName; fkColName = NULL;
        tmp->items[c].keySequence    = atoi(keySequence);
        tmp->items[c].updateRule     = atoi(updateRule);
        tmp->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_importedkeys *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer)
{
    cfrds_sql_importedkeys *ret = NULL;

    cfrds_sql_importedkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_importedkeys, items) + sizeof(cfrds_sql_importedkeysitem) * ucnt;
    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == NULL)
            return NULL;

        const char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(pkTableCatalog);
        cfrds_str_defer(pkTableOwner);
        cfrds_str_defer(pkTableName);
        cfrds_str_defer(pkColName);
        cfrds_str_defer(fkTableCatalog);
        cfrds_str_defer(fkTableOwner);
        cfrds_str_defer(fkTableName);
        cfrds_str_defer(fkColName);
        cfrds_str_defer(keySequence);
        cfrds_str_defer(updateRule);
        cfrds_str_defer(deleteRule);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule))
            return NULL;
        if (list_remaining != 0)
            return NULL;

        tmp->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = NULL;
        tmp->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = NULL;
        tmp->items[c].pkTableName    = pkTableName; pkTableName = NULL;
        tmp->items[c].pkColName      = pkColName; pkColName = NULL;
        tmp->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = NULL;
        tmp->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = NULL;
        tmp->items[c].fkTableName    = fkTableName; fkTableName = NULL;
        tmp->items[c].fkColName      = fkColName; fkColName = NULL;
        tmp->items[c].keySequence    = atoi(keySequence);
        tmp->items[c].updateRule     = atoi(updateRule);
        tmp->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_exportedkeys *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer)
{
    cfrds_sql_exportedkeys *ret = NULL;

    cfrds_sql_exportedkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_exportedkeys, items) + sizeof(cfrds_sql_exportedkeysitem) * ucnt;
    tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == NULL)
            return NULL;

        const char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(pkTableCatalog);
        cfrds_str_defer(pkTableOwner);
        cfrds_str_defer(pkTableName);
        cfrds_str_defer(pkColName);
        cfrds_str_defer(fkTableCatalog);
        cfrds_str_defer(fkTableOwner);
        cfrds_str_defer(fkTableName);
        cfrds_str_defer(fkColName);
        cfrds_str_defer(keySequence);
        cfrds_str_defer(updateRule);
        cfrds_str_defer(deleteRule);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule))
            return NULL;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule))
            return NULL;
        if (list_remaining != 0)
            return NULL;

        tmp->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = NULL;
        tmp->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = NULL;
        tmp->items[c].pkTableName    = pkTableName; pkTableName = NULL;
        tmp->items[c].pkColName      = pkColName; pkColName = NULL;
        tmp->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = NULL;
        tmp->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = NULL;
        tmp->items[c].fkTableName    = fkTableName; fkTableName = NULL;
        tmp->items[c].fkColName      = fkColName; fkColName = NULL;
        tmp->items[c].keySequence    = atoi(keySequence);
        tmp->items[c].updateRule     = atoi(updateRule);
        tmp->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_resultset *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer)
{
    cfrds_sql_resultset *ret = NULL;

    cfrds_sql_resultset_defer(tmp);
    int64_t cnt = 0;
    int64_t cols = 0;
    int64_t rows = 0;
    size_t buf_size = 0;

    if (buffer == NULL)
        return NULL;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        return NULL;

    if (cnt < 1 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    const char *response_start_data = response_data;
    size_t response_start_size = response_size;

    rows = cnt - 1;
    {
        cfrds_str_defer(col_row);
        cfrds_buffer_parse_string(&response_data, &response_size, &col_row);
        if (col_row == NULL)
            return NULL;

        const char *row_walker = col_row;
        size_t row_size = strlen(row_walker);
        while(row_size)
        {
            cfrds_str_defer(field);
            cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
            cols++;
        }
    }

    if (cols < 1)
        return NULL;

    size_t ucnt = (size_t)cnt;

    buf_size = offsetof(cfrds_sql_resultset, values) + sizeof(char *) * ucnt * (size_t)cols;
    tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);

    tmp->columns = (size_t)cols;
    tmp->rows = (size_t)rows;

    response_data = response_start_data;
    response_size = response_start_size;

    for(int64_t r = 0; r <= rows; r++)
    {
        cfrds_str_defer(row);
        cfrds_buffer_parse_string(&response_data, &response_size, &row);
        const char *row_walker = row;
        size_t row_size = strlen(row_walker);

        for(int64_t c = 0; c < cols; c++)
        {
            char *field = NULL;

            cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);

            tmp->values[r * cols + c] = field;
        }
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_metadata *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer)
{
    cfrds_sql_metadata *ret = NULL;

    cfrds_sql_metadata_defer(tmp);
    int64_t cnt = 0;
    size_t buf_size = 0;

    if (buffer == NULL)
        return NULL;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        return NULL;

    if ((cnt < 0) || (cnt > CFRDS_MAX_PARSER_ITEMS))
        return NULL;

    size_t ucnt = (size_t)cnt;
    buf_size = offsetof(cfrds_sql_metadata, items) + sizeof(cfrds_sql_metadataitem) * ucnt;
    tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);

    tmp->cnt = ucnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        char *field = NULL;
        cfrds_str_defer(row);

        cfrds_buffer_parse_string(&response_data, &response_size, &row);
        if (row == NULL)
            return NULL;

        const char *row_walker = row;
        size_t row_size = strlen(row_walker);

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        tmp->items[c].name = field;

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        tmp->items[c].type = field;

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        tmp->items[c].jtype = field;
    }

    ret = tmp; tmp = NULL;

    return ret;
}

cfrds_sql_supportedcommands *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer)
{
    cfrds_sql_supportedcommands *ret = NULL;

    cfrds_sql_supportedcommands_defer(tmp);

    int64_t rows = 0;
    int64_t row_size = 0;
    size_t cnt = 0;
    size_t buf_size = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return NULL;

    if (rows != 1)
        return NULL;

    if (!cfrds_buffer_parse_number(&data, &size, &row_size))
        return NULL;

    if (size != (size_t)row_size + 1)
        return NULL;

    {
        const char *start_data = data;
        size_t start_size = size - 1;
        while(start_size)
        {
            cfrds_str_defer(field);
            cfrds_buffer_parse_string_list_item(&start_data, &start_size, &field);
            cnt++;
        }
    }

    buf_size = offsetof(cfrds_sql_supportedcommands, commands) + sizeof(char *) * cnt;
    tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);

    tmp->cnt = cnt;

    for(size_t c = 0; c < cnt; c++)
    {
        char *field = NULL;

        cfrds_buffer_parse_string_list_item(&data, &size, &field);
        tmp->commands[c] = field;
    }

    ret = tmp; tmp = NULL;

    return ret;
}


char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer)
{
    char *ret = NULL;

    int64_t rows = 0;
    cfrds_str_defer(row);

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return NULL;

    cfrds_buffer_parse_string(&data, &size, &row);
    if (row == NULL)
        return NULL;

    data = row;
    size = strlen(data);

    cfrds_buffer_parse_string_list_item(&data, &size, &ret);

    return ret;
}

char *cfrds_buffer_to_debugger_start(cfrds_buffer *buffer)
{
    char *ret = NULL;

    int64_t rows = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 2)
        return NULL;

    cfrds_buffer_parse_string(&data, &size, &ret);

    return ret;
}

bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer)
{
    cfrds_str_defer(xml);

    int64_t rows = 0;

    if (buffer == NULL)
        return false;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return false;

    cfrds_buffer_parse_string(&data, &size, &xml);

    if (xml)
    {
        WDDX_defer(result);

        result = wddx_from_xml(xml);

        const char *status = wddx_get_string(result, "0,STATUS");
        if (status == NULL)
            return false;

        if (strcmp(status, "RDS_OK") == 0)
            return true;
    }

    return false;
}


int cfrds_buffer_to_debugger_info(cfrds_buffer *buffer)
{
    cfrds_str_defer(ret);

    int64_t rows = 0;

    if (buffer == NULL)
        return -1;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return -1;

    cfrds_buffer_parse_string(&data, &size, &ret);

    if (ret)
    {
        WDDX_defer(result);

        result = wddx_from_xml(ret);

        const char *status = wddx_get_string(result, "0,STATUS");
        if ((status == NULL)||(strcmp(status, "RDS_OK") != 0))
            return -1;

        return (int)wddx_get_number(result, "0,DEBUG_SERVER_PORT", NULL);
    }

    return -1;
}

cfrds_debugger_event *cfrds_buffer_to_debugger_event(cfrds_buffer *buffer)
{
    cfrds_str_defer(xml);

    int64_t rows = 0;

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return NULL;

    cfrds_buffer_parse_string(&data, &size, &xml);
    if (xml)
        return (cfrds_debugger_event *)wddx_from_xml(xml);

    return NULL;
}
