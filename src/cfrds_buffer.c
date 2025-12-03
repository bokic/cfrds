#include <string.h>
#include <cfrds.h>
#include <cfrds.h>
#include <wddx.h>
#include <internal/cfrds_buffer.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>


#if defined(__APPLE__) || defined(_WIN32)
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif

typedef struct {
    size_t allocated;
    size_t size;
    size_t offset;
    uint8_t *data;
} cfrds_buffer_int;


void cfrds_str_cleanup(char **str) {
    if (*str) {
        free(*str);
        *str = nullptr;
    }
}

void cfrds_fd_cleanup(int *fd) {
    if (fd) {
#ifdef _WIN32
        closesocket((SOCKET)*fd);
#else
        close(*fd);
#endif

        *fd = 0;
    }
}

void cfrds_buffer_cleanup(cfrds_buffer **buf) {
    if (*buf) {
        cfrds_buffer_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_file_content_cleanup(cfrds_file_content **buf) {
    if (*buf) {
        cfrds_buffer_file_content_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf) {
    if (*buf) {
        cfrds_buffer_browse_dir_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_dsninfo_cleanup(cfrds_sql_dsninfo **buf) {
    if (*buf) {
        cfrds_buffer_sql_dsninfo_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_tableinfo_cleanup(cfrds_sql_tableinfo **buf) {
    if (*buf) {
        cfrds_buffer_sql_tableinfo_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_columninfo_cleanup(cfrds_sql_columninfo **buf) {
    if (*buf) {
        cfrds_buffer_sql_columninfo_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_primarykeys_cleanup(cfrds_sql_primarykeys **buf) {
    if (*buf) {
        cfrds_buffer_sql_primarykeys_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_foreignkeys_cleanup(cfrds_sql_foreignkeys **buf) {
    if (*buf) {
        cfrds_buffer_sql_foreignkeys_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_importedkeys_cleanup(cfrds_sql_importedkeys **buf) {
    if (*buf) {
        cfrds_buffer_sql_importedkeys_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_exportedkeys_cleanup(cfrds_sql_exportedkeys **buf) {
    if (*buf) {
        cfrds_buffer_sql_exportedkeys_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_resultset_cleanup(cfrds_sql_resultset **buf) {
    if (*buf) {
        cfrds_buffer_sql_resultset_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_metadata_cleanup(cfrds_sql_metadata **buf) {
    if (*buf) {
        cfrds_buffer_sql_metadata_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_sql_supportedcommands_cleanup(cfrds_sql_supportedcommands **buf) {
    if (*buf) {
        cfrds_buffer_sql_supportedcommands_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_debugger_event_cleanup(cfrds_debugger_event **buf) {
    if (*buf) {
        cfrds_buffer_debugger_event_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_buffer_realloc_if_needed(cfrds_buffer *buffer, size_t len)
{
    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->size + len > buffer_int->allocated)
    {
        size_t newsize = buffer_int->size + len;
        newsize = (((newsize + 512) / 1024) + 1) * 1024;

        buffer_int->data = realloc(buffer_int->data, newsize);
        int oldsize = buffer_int->allocated;
        explicit_bzero(buffer_int->data + oldsize, newsize - oldsize);
        buffer_int->allocated = newsize;
    }
}

bool cfrds_buffer_create(cfrds_buffer **buffer)
{
    cfrds_buffer_int *tmp = nullptr;

    if (buffer == nullptr)
         return false;

    tmp = malloc(sizeof(cfrds_buffer_int));
    if (tmp == nullptr)
        return false;

    tmp->allocated = 0;
    tmp->size = 0;
    tmp->offset = 0;
    tmp->data = nullptr;

    *buffer = tmp;

    return true;
}

char *cfrds_buffer_data(cfrds_buffer *buffer)
{
    cfrds_buffer_int *ret = nullptr;

    if (buffer == nullptr)
        return nullptr;

    ret = buffer;

    cfrds_buffer_realloc_if_needed(ret, ret->size + 1);
    ret->data[ret->size] = 0;

    return (char *)ret->data;
}

size_t cfrds_buffer_data_size(cfrds_buffer *buffer)
{
    const cfrds_buffer_int *ret = nullptr;

    if (buffer == nullptr)
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
        memcpy(&buffer_int->data[buffer_int->size], str, len);
        buffer_int->size += len;
    }
}

void cfrds_buffer_append_int(cfrds_buffer *buffer, int number)
{
    char str[16];

    snprintf(str, sizeof(str), "%d", number);

    cfrds_buffer_append(buffer, str);
}

void cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    cfrds_buffer_int *buffer_int = buffer;

    if ((!buffer_int)||(!data)||(length == 0))
    {
        return;
    }

    cfrds_buffer_realloc_if_needed(buffer_int, length);

    memcpy(&buffer_int->data[buffer_int->size], data, length);
    buffer_int->size += length;
}

void cfrds_buffer_append_buffer(cfrds_buffer *buffer, cfrds_buffer *new)
{
    cfrds_buffer_int *buffer_int = buffer;
    const cfrds_buffer_int *new_int = new;
    size_t len = new_int->size;

    cfrds_buffer_realloc_if_needed(buffer_int, len);

    memcpy(&buffer_int->data[buffer_int->size], new_int->data, len);
    buffer_int->size += len;
}

void cfrds_buffer_append_rds_count(cfrds_buffer *buffer, size_t cnt)
{
    char str[16] = {0, };
    int n = 0;

    n = snprintf(str, sizeof(str), "%zu", cnt);
    if (n > 0)
    {
        cfrds_buffer_append(buffer, str);
        cfrds_buffer_append_char(buffer, ':');
    }
}

void cfrds_buffer_append_rds_string(cfrds_buffer *buffer, const char *str)
{
    char str_len[16] = {0, };
    size_t len = 0;
    int n = 0;

    if ((!buffer)||(!str))
    {
        return;
    }

    len = strlen(str);
    n = snprintf(str_len, sizeof(str_len), "%zu", len);
    if (n > 0)
    {
        cfrds_buffer_append(buffer, "STR:");
        cfrds_buffer_append(buffer, str_len);
        cfrds_buffer_append_char(buffer, ':');
        cfrds_buffer_append(buffer, str);
    }
}

void cfrds_buffer_append_rds_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    char str_len[16] = {0, };
    int n = 0;

    n = snprintf(str_len, sizeof(str_len), "%zu", length);
    if (n > 0)
    {
        cfrds_buffer_append(buffer, "STR:");
        cfrds_buffer_append(buffer, str_len);
        cfrds_buffer_append_char(buffer, ':');
        cfrds_buffer_append_bytes(buffer, data, length);
    }
}

void cfrds_buffer_append_char(cfrds_buffer *buffer, const char ch)
{
    cfrds_buffer_int *buffer_int = buffer;

    cfrds_buffer_realloc_if_needed(buffer_int, 1);

    buffer_int->data[buffer_int->size] = ch;
    buffer_int->size++;
}

void cfrds_buffer_reserve_above_size(cfrds_buffer *buffer, size_t size)
{
    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->allocated - buffer_int->size < size)
    {
        size_t newsize = buffer_int->size + size;

        buffer_int->data = realloc(buffer_int->data, newsize);
        int oldsize = buffer_int->allocated;
        explicit_bzero(buffer_int->data + oldsize, newsize- oldsize);
        buffer_int->allocated = newsize;
    }
}

void cfrds_buffer_expand(cfrds_buffer *buffer, size_t size)
{
    cfrds_buffer_int *buffer_int = buffer;

    buffer_int->size += size;
}

void cfrds_buffer_free(cfrds_buffer *buffer)
{
    if (buffer == nullptr)
        return;

    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->data != nullptr)
    {
        free(buffer_int->data);
        buffer_int->data = nullptr;
    }

    free(buffer);
}

bool cfrds_buffer_parse_number(const char **data, size_t *remaining, int64_t *out)
{
    char *end = nullptr;

    if (data == nullptr)
        return false;

    end = strchr(*data, ':');
    if ((end == nullptr)||((unsigned)(end - *data) > *remaining))
        return false;

    *out = atol(*data);

    *remaining -= end - *data + 1;
    *data = end + 1;

    return true;
}

bool cfrds_buffer_parse_bytearray(const char **data, size_t *remaining, char **out, int *out_size)
{
    int64_t size = 0;

    if (out == nullptr)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &size))
        return false;

    if (size < 0)
        return false;

    *out = malloc(size + 1);
    if (*out == nullptr)
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
    int64_t size = 0;

    if (out == nullptr)
        return false;

    if (!cfrds_buffer_parse_number(data, remaining, &size))
        return false;

    if (size <= 0)
        return false;

    *out = malloc(size + 1);
    if(*out == nullptr)
        return false;

    memcpy(*out, *data, size);
    (*out)[size] = 0;

    *remaining -= size;
    *data += size;

    return true;
}

bool cfrds_buffer_parse_string_list_item(const char **data, size_t *remaining, char **out)
{
    bool with_quotes = false;
    const char *endstr = nullptr;
    cfrds_str_defer(tmp);
    int len = 0;

    if ((data == nullptr)||(out == nullptr)||(*remaining < 2))
        return false;

    if (*data[0] == '"')
    {
        with_quotes = true;
        (*data)++; (*remaining)--;

        endstr = strchr(*data, '"');
        if (endstr == nullptr)
            return false;
        len = endstr - *data;
    }
    else
    {
        endstr = strchr(*data, ',');
        if (endstr == nullptr)
            len = strlen(*data);
        else
            len = endstr - *data;
    }


    tmp = malloc(len + 1);
    if (tmp == nullptr)
        return false;

    memcpy(tmp, *data, len);
    tmp[len] = '\0';
    (*data)+=len; (*remaining)-=len;

    if (with_quotes) {
        if (*data[0] != '"')
            return false;

        (*data)++; (*remaining)--;
    }

    if (*data[0] == ',') {
        (*data)++; (*remaining)--;
    }

    *out = tmp; tmp = nullptr;

    return true;
}

cfrds_browse_dir_int *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer)
{
    cfrds_browse_dir_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;
    cfrds_browse_dir_defer(tmp);
    size_t malloc_size = 0;
    int64_t total = 0;
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return nullptr;

    if ((total <= 0)||(total % 5))
        return nullptr;

    cnt = total / 5;

    malloc_size = offsetof(cfrds_browse_dir_int, items) + (cnt * (offsetof(cfrds_browse_dir_int, items[1]) - (offsetof(cfrds_browse_dir_int, items[0]))));

    tmp = malloc(malloc_size);
    if (tmp == nullptr)
        return nullptr;

    explicit_bzero(tmp, malloc_size);

    ((cfrds_browse_dir_int *)tmp)->cnt = cnt;

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
        uint64_t modified = -1;

        cfrds_buffer_parse_string(&data, &size, &str_kind);
        cfrds_buffer_parse_string(&data, &size, &filename);
        cfrds_buffer_parse_string(&data, &size, &str_permissions);
        cfrds_buffer_parse_string(&data, &size, &str_filesize);
        cfrds_buffer_parse_string(&data, &size, &str_timestamp);

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
        }

        if(((file_type != 'D')&&(file_type != 'F'))||(!filename)||(permissions < 0)||(permissions > 0xff)||(filesize < 0))
            return nullptr;

        ((cfrds_browse_dir_int *)tmp)->items[c].kind = file_type;
        ((cfrds_browse_dir_int *)tmp)->items[c].name = filename; filename = nullptr;
        ((cfrds_browse_dir_int *)tmp)->items[c].permissions = permissions;
        ((cfrds_browse_dir_int *)tmp)->items[c].size = filesize;
        ((cfrds_browse_dir_int *)tmp)->items[c].modified = modified;
    }

    ret = tmp; tmp = nullptr;
    return ret;
}

cfrds_file_content_int *cfrds_buffer_to_file_content(cfrds_buffer *buffer)
{
    cfrds_file_content_int *ret = nullptr;
    cfrds_buffer_int *buffer_int = buffer;
    int64_t total = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return nullptr;

    if (total != 3)
        return nullptr;

    ret = malloc(sizeof(cfrds_file_content_int));
    if (ret == nullptr)
        return nullptr;

    cfrds_buffer_parse_bytearray(&data, &size, &ret->data, &ret->size);
    cfrds_buffer_parse_string(&data, &size, &ret->modified);
    cfrds_buffer_parse_string(&data, &size, &ret->permission);

    return ret;
}

cfrds_sql_dsninfo_int *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer)
{
    cfrds_sql_dsninfo_int *ret = nullptr;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

      int64_t cnt = 0;

      if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
      {
          return nullptr;
      }

      if (cnt > 10000)
          return nullptr;

      ret = malloc(offsetof(cfrds_sql_dsninfo_int, names) + sizeof(char *) * cnt);
      if (ret == nullptr)
          return nullptr;

      ret->cnt = cnt;

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
                      size_t len = pos2 - pos1 - 1;

                      char *tmp = malloc(len + 1);
                      if (tmp == nullptr)
                          return nullptr;

                      memcpy(tmp, pos1 + 1, len);
                      tmp[len] = '\0';
                      free(item);
                      item = tmp;
                  }
              }

              ret->names[c] = item; item = nullptr;
          }
      }

    return ret;
}

cfrds_sql_tableinfo_int *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer)
{
    cfrds_sql_tableinfo_int *ret = nullptr;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
    {
        return nullptr;
    }

    if (cnt > 10000)
        return nullptr;

    size_t malloc_size = offsetof(cfrds_sql_tableinfo_int, items) + sizeof(cfrds_sql_tableinfoitem_int) * cnt;

    ret = malloc(malloc_size);
    if (ret == nullptr)
        return nullptr;

    explicit_bzero(ret, malloc_size);

    ret->cnt = 0;

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
            const char *end_item = nullptr;

            if (current_item[0] != '"')
                return nullptr;
            current_item++;
            end_item = strchr(current_item, '"');
            if (!end_item)
                return nullptr;
            if (end_item > current_item) {
                int size = end_item - current_item;

                field1 = malloc(size + 1);
                if (field1 == nullptr)
                    return nullptr;

                memcpy(field1, current_item, size);
                field1[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return nullptr;
            current_item++;

            if (current_item[0] != '"')
                return nullptr;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return nullptr;

            if (end_item > current_item) {
                int size = end_item - current_item;

                field2 = malloc(size + 1);
                if (!field2)
                    return nullptr;

                memcpy(field2, current_item, size);
                field2[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return nullptr;
            current_item++;

            if (current_item[0] != '"')
                return nullptr;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return nullptr;

            if (end_item > current_item) {
                int size = end_item - current_item;

                field3 = malloc(size + 1);
                if (!field3)
                    return nullptr;

                memcpy(field3, current_item, size);
                field3[size] = '\0';
            }
            current_item = end_item + 1;

            if (current_item[0] != ',')
                return nullptr;
            current_item++;

            if (current_item[0] != '"')
                return nullptr;
            current_item++;

            end_item = strchr(current_item, '"');
            if (!end_item)
                return nullptr;

            if (end_item > current_item) {
                int size = end_item - current_item;

                field4 = malloc(size + 1);
                if (!field4)
                    return nullptr;

                memcpy(field4, current_item, size);
                field4[size] = '\0';
            }

            ret->items[ret->cnt].unknown = field1; field1 = nullptr;
            ret->items[ret->cnt].schema  = field2; field2 = nullptr;
            ret->items[ret->cnt].name    = field3; field3 = nullptr;
            ret->items[ret->cnt].type    = field4; field4 = nullptr;
            ret->cnt++;
        }
    }

    return ret;
}

cfrds_sql_columninfo_int *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer)
{
    cfrds_sql_columninfo_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    int64_t columns = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &columns))
        return nullptr;

    if (columns <= 0)
        return nullptr;

    ret = malloc(offsetof(cfrds_sql_columninfo_int, items) + sizeof(cfrds_sql_columninfoitem_int) * columns);
    if (ret == nullptr)
        return nullptr;

    explicit_bzero(ret, offsetof(cfrds_sql_columninfo_int, items) + sizeof(cfrds_sql_columninfoitem_int) * columns);
    ret->cnt = columns;

    for(int64_t column = 0; column < columns; column++)
    {
        cfrds_str_defer(row_buf);

        if (!cfrds_buffer_parse_string(&data, &size, &row_buf))
            return nullptr;

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

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field1))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field2))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field3))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field4))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field5))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field6))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field7))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field8))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field9))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field10))
            return nullptr;
        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field11))
            return nullptr;
        if (list_remaining != 0)
            return nullptr;

        ret->items[column].schema    = field1; field1 = nullptr;
        ret->items[column].owner     = field2; field2 = nullptr;
        ret->items[column].table     = field3; field3 = nullptr;
        ret->items[column].name      = field4; field4 = nullptr;
        ret->items[column].type      = atoi(field5);
        ret->items[column].typeStr   = field6; field6 = nullptr;
        ret->items[column].percision = atoi(field7);
        ret->items[column].length    = atoi(field8);
        ret->items[column].scale     = atoi(field9);
        ret->items[column].radix     = atoi(field10);
        ret->items[column].nullable  = atoi(field11);
    }

    return ret;
}

cfrds_sql_primarykeys_int *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer)
{
    cfrds_sql_primarykeys_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_primarykeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return nullptr;

    tmp = malloc(offsetof(cfrds_sql_primarykeys_int, items) + sizeof(cfrds_sql_primarykeysitem_int) * cnt);
    if (tmp == nullptr)
        return nullptr;

    ((cfrds_sql_primarykeys_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == nullptr)
            return nullptr;

        const char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(tableCatalog);
        cfrds_str_defer(tableOwner);
        cfrds_str_defer(tableName);
        cfrds_str_defer(colName);
        cfrds_str_defer(keySequence);

        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &colName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence);

        ((cfrds_sql_primarykeys_int *)tmp)->items[c].tableCatalog = tableCatalog; tableCatalog = nullptr;
        ((cfrds_sql_primarykeys_int *)tmp)->items[c].tableOwner   = tableOwner; tableOwner = nullptr;
        ((cfrds_sql_primarykeys_int *)tmp)->items[c].tableName    = tableName; tableName = nullptr;
        ((cfrds_sql_primarykeys_int *)tmp)->items[c].colName      = colName; colName = nullptr;
        ((cfrds_sql_primarykeys_int *)tmp)->items[c].keySequence  = atoi(keySequence);
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_foreignkeys_int *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer)
{
    cfrds_sql_foreignkeys_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_foreignkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return nullptr;

    tmp = malloc(offsetof(cfrds_sql_foreignkeys_int, items) + sizeof(cfrds_sql_foreignkeysitem_int) * cnt);
    if (tmp == nullptr)
        return nullptr;

    ((cfrds_sql_foreignkeys_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == nullptr)
            return nullptr;

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

        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule);

        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].pkTableName    = pkTableName; pkTableName = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].pkColName      = pkColName; pkColName = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].fkTableName    = fkTableName; fkTableName = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].fkColName      = fkColName; fkColName = nullptr;
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].keySequence    = atoi(keySequence);
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].updateRule     = atoi(updateRule);
        ((cfrds_sql_foreignkeys_int *)tmp)->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_importedkeys_int *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer)
{
    cfrds_sql_importedkeys_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_importedkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return nullptr;

    if (cnt < 1)
        return nullptr;

    tmp = malloc(offsetof(cfrds_sql_importedkeys_int, items) + sizeof(cfrds_sql_importedkeysitem_int) * cnt);
    if (tmp == nullptr)
        return nullptr;

    ((cfrds_sql_importedkeys_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == nullptr)
            return nullptr;

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

        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule);

        ((cfrds_sql_importedkeys_int *)tmp)->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].pkTableName    = pkTableName; pkTableName = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].pkColName      = pkColName; pkColName = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].fkTableName    = fkTableName; fkTableName = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].fkColName      = fkColName; fkColName = nullptr;
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].keySequence    = atoi(keySequence);
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].updateRule     = atoi(updateRule);
        ((cfrds_sql_importedkeys_int *)tmp)->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_exportedkeys_int *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer)
{
    cfrds_sql_exportedkeys_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_exportedkeys_defer(tmp);
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return nullptr;

    tmp = malloc(offsetof(cfrds_sql_exportedkeys_int, items) + sizeof(cfrds_sql_exportedkeysitem_int) * cnt);
    if (tmp == nullptr)
        return nullptr;

    ((cfrds_sql_exportedkeys_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == nullptr)
            return nullptr;

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

        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule);

        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].pkTableCatalog = pkTableCatalog; pkTableCatalog = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].pkTableOwner   = pkTableOwner; pkTableOwner = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].pkTableName    = pkTableName; pkTableName = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].pkColName      = pkColName; pkColName = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].fkTableCatalog = fkTableCatalog; fkTableCatalog = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].fkTableOwner   = fkTableOwner; fkTableOwner = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].fkTableName    = fkTableName; fkTableName = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].fkColName      = fkColName; fkColName = nullptr;
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].keySequence    = atoi(keySequence);
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].updateRule     = atoi(updateRule);
        ((cfrds_sql_exportedkeys_int *)tmp)->items[c].deleteRule     = atoi(deleteRule);
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_resultset_int *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer)
{
    cfrds_sql_resultset_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_resultset_defer(tmp);
    int64_t cnt = 0;
    int64_t cols = 0;
    int64_t rows = 0;
    int buf_size = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        return nullptr;

    if (cnt < 1)
        return nullptr;

    const char *response_start_data = response_data;
    size_t response_start_size = response_size;

    rows = cnt - 1;
    {
        cfrds_str_defer(col_row);
        cfrds_buffer_parse_string(&response_data, &response_size, &col_row);
        if (col_row == nullptr)
            return nullptr;

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
        return nullptr;

    cnt = cols * (rows + 1);

    buf_size = offsetof(cfrds_sql_resultset_int, values) + sizeof(char *) * cnt;
    tmp = malloc(buf_size);
    if (tmp == nullptr)
        return nullptr;

    explicit_bzero(tmp, buf_size);

    ((cfrds_sql_resultset_int *)tmp)->columns = cols;
    ((cfrds_sql_resultset_int *)tmp)->rows = rows;

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
            char *field = nullptr;

            cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);

            ((cfrds_sql_resultset_int *)tmp)->values[r * cols + c] = field;
        }
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_metadata_int *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer)
{
    cfrds_sql_metadata_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_resultset_defer(tmp);
    int64_t cnt = 0;
    int buf_size = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        return nullptr;

    buf_size = offsetof(cfrds_sql_metadata_int, items) + sizeof(cfrds_sql_metadataitem_int) * cnt;
    tmp = malloc(buf_size);
    if (tmp == nullptr)
        return nullptr;

    explicit_bzero(tmp, buf_size);

    ((cfrds_sql_metadata_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        char *field = nullptr;
        cfrds_str_defer(row);

        cfrds_buffer_parse_string(&response_data, &response_size, &row);
        if (row == nullptr)
            return nullptr;

        const char *row_walker = row;
        size_t row_size = strlen(row_walker);

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        ((cfrds_sql_metadata_int *)tmp)->items[c].name = field;

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        ((cfrds_sql_metadata_int *)tmp)->items[c].type = field;

        cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field);
        ((cfrds_sql_metadata_int *)tmp)->items[c].jtype = field;
    }

    ret = tmp; tmp = nullptr;

    return ret;
}

cfrds_sql_supportedcommands_int *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer)
{
    cfrds_sql_supportedcommands_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    cfrds_sql_supportedcommands_defer(tmp);

    int64_t rows = 0;
    int64_t row_size = 0;
    int64_t cnt = 0;
    int buf_size = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return nullptr;

    if (rows != 1)
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &row_size))
        return nullptr;

    if (size != (size_t)row_size + 1)
        return nullptr;

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

    buf_size = offsetof(cfrds_sql_supportedcommands_int, commands) + sizeof(char *) * cnt;
    tmp = malloc(buf_size);
    if (tmp == nullptr)
        return nullptr;

    explicit_bzero(tmp, buf_size);

    ((cfrds_sql_supportedcommands_int *)tmp)->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        char *field = nullptr;

        cfrds_buffer_parse_string_list_item(&data, &size, &field);
        ((cfrds_sql_supportedcommands_int *)tmp)->commands[c] = field;
    }

    ret = tmp; tmp = nullptr;

    return ret;
}


char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer)
{
    char *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;

    int64_t rows = 0;
    cfrds_str_defer(row);

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return nullptr;

    cfrds_buffer_parse_string(&data, &size, &row);
    if (row == nullptr)
        return nullptr;

    data = (const char *)row;
    size = strlen(data);

    cfrds_buffer_parse_string_list_item(&data, &size, &ret);

    return ret;
}

char *cfrds_buffer_to_debugger_start(cfrds_buffer *buffer)
{
    char *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;
    int64_t rows = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 2)
        return nullptr;

    cfrds_buffer_parse_string(&data, &size, &ret);

    return ret;
}

bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer)
{
    cfrds_str_defer(xml);

    cfrds_buffer_int *buffer_int = buffer;
    int64_t rows = 0;

    if (buffer_int == nullptr)
        return false;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return false;

    cfrds_buffer_parse_string(&data, &size, &xml);

    if (xml)
    {
        WDDX_defer(result);

        result = wddx_from_xml(xml);

        const char *status = wddx_get_string(result, "0,STATUS");
        if (status == nullptr)
            return false;

        if (strcmp(status, "RDS_OK") == 0)
            return true;
    }

    return false;
}


int cfrds_buffer_to_debugger_info(cfrds_buffer *buffer)
{
    cfrds_str_defer(ret);

    cfrds_buffer_int *buffer_int = buffer;
    int64_t rows = 0;

    if (buffer_int == nullptr)
        return -1;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return -1;

    cfrds_buffer_parse_string(&data, &size, &ret);

    if (ret)
    {
        WDDX_defer(result);

        result = wddx_from_xml(ret);

        const char *status = wddx_get_string(result, "0,STATUS");
        if ((status == nullptr)||(strcmp(status, "RDS_OK") != 0))
            return -1;

        return wddx_get_number(result, "0,DEBUG_SERVER_PORT", nullptr);
    }

    return -1;
}

cfrds_debugger_event *cfrds_buffer_to_debugger_event(cfrds_buffer *buffer)
{
    cfrds_str_defer(xml);

    cfrds_buffer_int *buffer_int = buffer;
    int64_t rows = 0;

    if (buffer_int == nullptr)
        return nullptr;

    const char *data = (const char *)buffer_int->data;
    size_t size = buffer_int->size;

    cfrds_buffer_parse_number(&data, &size, &rows);
    if (rows != 1)
        return nullptr;

    cfrds_buffer_parse_string(&data, &size, &xml);
    if (xml)
        return wddx_from_xml(xml);

    return nullptr;
}
