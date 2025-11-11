#include <string.h>
#include <cfrds.h>
#include <cfrds.h>
#include <internal/cfrds_buffer.h>

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>


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
        close(*fd);
        *fd = 0;
    }
}

void cfrds_buffer_cleanup(cfrds_buffer **buf) {
    if (*buf) {
        cfrds_buffer_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_browse_dir_cleanup(cfrds_browse_dir **buf) {
    if (*buf) {
        cfrds_buffer_browse_dir_free(*buf);
        *buf = nullptr;
    }
}

void cfrds_buffer_realloc_if_needed(cfrds_buffer *buffer, size_t len)
{
    cfrds_buffer_int *buffer_int = buffer;

    if (buffer_int->size + len > buffer_int->allocated)
    {
        size_t new_size = buffer_int->size + len;
        new_size = (((new_size + 512) / 1024) + 1) * 1024;

        buffer_int->data = realloc(buffer_int->data, new_size);
        buffer_int->allocated = new_size;
    }
}

bool cfrds_buffer_create(cfrds_buffer **buffer)
{
    cfrds_buffer_int *ret = nullptr;

    if (buffer == nullptr)
         return false;

    ret = malloc(sizeof(cfrds_buffer_int));
    if (ret == nullptr)
        return false;

    ret->allocated = 0;
    ret->size = 0;
    ret->offset = 0;
    ret->data = nullptr;

    *buffer = ret;

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
        size_t new_size = buffer_int->size + size;

        buffer_int->data = realloc(buffer_int->data, new_size);
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

bool cfrds_buffer_parse_number(char **data, size_t *remaining, int64_t *out)
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

bool cfrds_buffer_parse_bytearray(char **data, size_t *remaining, char **out, int *out_size)
{
    int64_t size = 0;

    if (out == nullptr)
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

    if (out == nullptr)
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

bool cfrds_buffer_parse_string_list_item(char **data, size_t *remaining, char **out)
{
    cfrds_str_defer(tmp);

    if ((data == nullptr)||(out == nullptr)||(*remaining < 2))
        return false;

    if (*data[0] != '"')
        return false;

    (*data)++; (*remaining)--;

    const char *endstr = strchr(*data, '"');
    if (endstr == nullptr)
        return false;

    int len = endstr - *data;
    tmp = malloc(len + 1);
    if (tmp == nullptr)
        return false;
    memcpy(tmp, *data, len);
    tmp[len] = '\0';
    (*data)+=len; (*remaining)-=len;

    if (*data[0] != '"') {
        return false;
    }

    (*data)++; (*remaining)--;

    if (*data[0] == ',') {
        (*data)++; (*remaining)--;
    }

    *out = tmp;
    tmp = nullptr;

    return true;
}

bool cfrds_buffer_skip_httpheader(char **data, size_t *remaining)
{
    char *body = nullptr;

    body = strstr(*data, "\r\n\r\n");
    if (body == nullptr)
        return false;

    *remaining -= body - *data;
    *data = body + 4;
    *remaining -= 4;

    return true;
}

cfrds_browse_dir_int *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer)
{
    cfrds_browse_dir_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;
    cfrds_browse_dir_defer(tmp);
    int64_t total = 0;
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    char *data = (char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &total))
        return nullptr;

    if ((total <= 0)||(total % 5))
        return nullptr;

    cnt = total / 5;

    size_t ret_size = offsetof(cfrds_browse_dir_int, items) + (cnt * (offsetof(cfrds_browse_dir_int, items[1]) - (offsetof(cfrds_browse_dir_int, items[0]))));
    tmp = malloc(ret_size);
    if (tmp == nullptr)
        return nullptr;
    memset(tmp, 0, ret_size);

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

    char *data = (char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return nullptr;

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

    char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (cfrds_buffer_skip_httpheader(&response_data, &response_size))
    {
        int64_t cnt = 0;

        if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        {
            return nullptr;
        }

        if (cnt > 10000)
            return nullptr;

        ret = malloc(offsetof(cfrds_sql_dsninfo_int, names) + sizeof(char *) * cnt);
        if (ret == nullptr) return nullptr;

        ret->cnt = cnt;

        for(int c = 0; c < cnt; c++)
        {
            char *item = nullptr;

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
                        if (tmp == nullptr) return nullptr;
                        memcpy(tmp, pos1 + 1, len);
                        tmp[len] = '\0';
                        free(item);
                        item = tmp;
                    }
                }

                ret->names[c] = item;
            }
        }
    }

    return ret;
}

cfrds_sql_tableinfo_int *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer)
{
    cfrds_sql_tableinfo_int *ret = nullptr;

    char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    if (cfrds_buffer_skip_httpheader(&response_data, &response_size))
    {
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
        memset(ret, 0, malloc_size);

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

                if (current_item[0] != '"') return nullptr;
                current_item++;
                end_item = strchr(current_item, '"');
                if (!end_item) return nullptr;
                if (end_item > current_item) {
                    int size = end_item - current_item;
                    field1 = malloc(size + 1);
                    if (!field1) return nullptr;
                    memcpy(field1, current_item, size);
                    field1[size] = '\0';
                }
                current_item = end_item + 1;

                if (current_item[0] != ',') return nullptr;
                current_item++;

                if (current_item[0] != '"') return nullptr;
                current_item++;
                end_item = strchr(current_item, '"');
                if (!end_item) return nullptr;
                if (end_item > current_item) {
                    int size = end_item - current_item;
                    field2 = malloc(size + 1);
                    if (!field2) return nullptr;
                    memcpy(field2, current_item, size);
                    field2[size] = '\0';
                }
                current_item = end_item + 1;

                if (current_item[0] != ',') return nullptr;
                current_item++;

                if (current_item[0] != '"') return nullptr;
                current_item++;
                end_item = strchr(current_item, '"');
                if (!end_item) return nullptr;
                if (end_item > current_item) {
                    int size = end_item - current_item;
                    field3 = malloc(size + 1);
                    if (!field3) return nullptr;
                    memcpy(field3, current_item, size);
                    field3[size] = '\0';
                }
                current_item = end_item + 1;

                if (current_item[0] != ',') return nullptr;
                current_item++;

                if (current_item[0] != '"') return nullptr;
                current_item++;
                end_item = strchr(current_item, '"');
                if (!end_item) return nullptr;
                if (end_item > current_item) {
                    int size = end_item - current_item;
                    field4 = malloc(size + 1);
                    if (!field4) return nullptr;
                    memcpy(field4, current_item, size);
                    field4[size] = '\0';
                }
                current_item = end_item + 1;

                ret->items[ret->cnt].unknown = field1; field1 = nullptr;
                ret->items[ret->cnt].schema  = field2; field2 = nullptr;
                ret->items[ret->cnt].name    = field3; field3 = nullptr;
                ret->items[ret->cnt].type    = field4; field4 = nullptr;
                ret->cnt++;
            }
        }
    }

    return ret;
}

cfrds_sql_columninfo_int *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer)
{
    cfrds_sql_columninfo_int *ret = nullptr;

    cfrds_buffer_int *buffer_int = buffer;
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

    cfrds_str_defer(row_buf);
    int64_t columns = 0;
    //int64_t row_size = 0;
    //int64_t cnt = 1;

    if (buffer_int == nullptr)
        return nullptr;

    char *data = (char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &columns))
        return nullptr;

    if (columns <= 0) return nullptr;

    ret = malloc(offsetof(cfrds_sql_columninfo_int, items) + sizeof(cfrds_sql_columninfoitem_int) * columns);
    if (ret == nullptr) return nullptr;

    memset(ret, 0, offsetof(cfrds_sql_columninfo_int, items) + sizeof(cfrds_sql_columninfoitem_int) * columns);
    ret->cnt = columns;

    for(int64_t column = 0; column < columns; column++)
    {
        if (!cfrds_buffer_parse_string(&data, &size, &row_buf))
            return nullptr;

        char *column_buf = row_buf;

        size_t list_remaining = strlen(column_buf);

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
    cfrds_sql_primarykeys_int *ret = nullptr; // TODO: Defer this!
    cfrds_buffer_int *buffer_int = buffer;
    int64_t cnt = 0;

    if (buffer_int == nullptr)
        return nullptr;

    char *data = (char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return nullptr;

    ret = malloc(offsetof(cfrds_sql_primarykeys_int, items) + sizeof(cfrds_sql_primarykeysitem_int) * cnt);
    if (ret == nullptr)
        return nullptr;

    ret->cnt = cnt;

    for(int64_t c = 0; c < cnt; c++)
    {
        cfrds_str_defer(item);
        cfrds_str_defer(field1);
        cfrds_str_defer(field2);
        cfrds_str_defer(field3);
        cfrds_str_defer(field4);
        cfrds_str_defer(field5);

        cfrds_buffer_parse_string(&data, &size, &item);
        if (item == nullptr)
        {
          free(ret); // TODO: Use defer here!
          return nullptr;
        }

        char *column_buf = item;

        size_t list_remaining = strlen(column_buf);

        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field1);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field2);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field3);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field4);
        cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field5);

        ret->items[c].tableCatalog = field1; field1 = nullptr;
        ret->items[c].tableOwner   = field2; field2 = nullptr;
        ret->items[c].tableName    = field3; field3 = nullptr;
        ret->items[c].colName      = field4; field4 = nullptr;
        ret->items[c].keySequence  = atoi(field5);
    }

    return ret;
}

cfrds_sql_supportedcommands_int *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer)
{
    cfrds_sql_supportedcommands_int *ret = nullptr;
    cfrds_buffer_int *buffer_int = buffer;
    int64_t rows = 0;
    int64_t row_size = 0;
    //int64_t cnt = 1;

    if (buffer_int == nullptr)
        return nullptr;

    char *data = (char *)buffer_int->data;
    size_t size = buffer_int->size;

    if (!cfrds_buffer_skip_httpheader(&data, &size))
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return nullptr;

    if (!cfrds_buffer_parse_number(&data, &size, &row_size))
        return nullptr;

    if (size != (size_t)row_size + 1)
        return nullptr;


    //ret = malloc(offsetof(cfrds_sql_supportedcommands_int, items) + sizeof(char *) * cnt);
    //ret->cnt = cnt;

    printf("data: %s\n", data);

    // TODO: Implement me!
    return ret;
}
