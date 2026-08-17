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

#define CFRDS_MAX_PARSER_ITEMS 1000000

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

#define CFRDS_DEFINE_CLEANUP(type, free_fn) \
    void type##_cleanup(type **buf) { \
        if (buf && *buf) { \
            free_fn(*buf); \
            *buf = NULL; \
        } \
    }

CFRDS_DEFINE_CLEANUP(cfrds_file_content, cfrds_file_content_free)
CFRDS_DEFINE_CLEANUP(cfrds_browse_dir, cfrds_browse_dir_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_dsninfo, cfrds_sql_dsninfo_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_tableinfo, cfrds_sql_tableinfo_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_columninfo, cfrds_sql_columninfo_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_primarykeys, cfrds_sql_primarykeys_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_foreignkeys, cfrds_sql_foreignkeys_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_importedkeys, cfrds_sql_importedkeys_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_exportedkeys, cfrds_sql_exportedkeys_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_resultset, cfrds_sql_resultset_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_metadata, cfrds_sql_metadata_free)
CFRDS_DEFINE_CLEANUP(cfrds_sql_supportedcommands, cfrds_sql_supportedcommands_free)
CFRDS_DEFINE_CLEANUP(cfrds_debugger_event, cfrds_debugger_event_free)

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
        size_t double_size = buffer->allocated * 2;
        size_t newsize = (((required + 512) / 1024) + 1) * 1024;
        if (double_size > newsize)
            newsize = double_size;

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

bool cfrds_buffer_append_escaped(cfrds_buffer *buffer, const char *str)
{
    if ((!buffer) || (!str))
    {
        return false;
    }

    for (const char *p = str; *p != '\0'; p++)
    {
        if (*p == ':' || *p == ';')
        {
            if (!cfrds_buffer_append_char(buffer, '\\'))
                return false;
        }
        if (!cfrds_buffer_append_char(buffer, *p))
            return false;
    }

    return true;
}

static __attribute__((unused)) bool cfrds_buffer_append_int(cfrds_buffer *buffer, int number)
{
    char str[16];

    if (buffer == NULL)
        return false;

    snprintf(str, sizeof(str), "%d", number);

    if (cfrds_buffer_append(buffer, str) == false)
        return false;

    return true;
}

bool cfrds_buffer_append_bytes(cfrds_buffer *buffer, const void *data, size_t length)
{
    if (length == 0)
    {
        return true;
    }

    if ((!buffer)||(!data))
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
    if (len == 0)
        return true;

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

    const char *data_start = *data;
    size_t rem_start = *remaining;

    if (!cfrds_buffer_parse_number(data, remaining, &tmp))
        return false;

    if (tmp < 0) {
        *data = data_start;
        *remaining = rem_start;
        return false;
    }

    size = (size_t)tmp;
    if (size > *remaining) {
        *data = data_start;
        *remaining = rem_start;
        return false;
    }

    *out = malloc(size + 1);
    if (*out == NULL) {
        *data = data_start;
        *remaining = rem_start;
        return false;
    }

    memcpy(*out, *data, size);
    (*out)[size] = 0;

    *remaining -= size;
    *data += size;

    if (out_size)
        *out_size = size;

    return true;
}

bool cfrds_buffer_parse_string(const char **data, size_t *remaining, char **out)
{
    return cfrds_buffer_parse_bytearray(data, remaining, out, NULL);
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
    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    int64_t total_elements = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &total_elements))
        return NULL;

    if (total_elements < 0 || total_elements % 5 != 0 || total_elements > (int64_t)CFRDS_MAX_PARSER_ITEMS * 5)
        return NULL;

    size_t item_count = (size_t)(total_elements / 5);
    size_t alloc_size = offsetof(cfrds_browse_dir, items) + item_count * sizeof(cfrds_browse_dir_item);
    cfrds_browse_dir *tmp = malloc(alloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, alloc_size);
    tmp->cnt = 0;

    for (size_t i = 0; i < item_count; i++)
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
            goto error;
        if (!cfrds_buffer_parse_string(&data, &size, &filename))
            goto error;
        if (!cfrds_buffer_parse_string(&data, &size, &str_permissions))
            goto error;
        if (!cfrds_buffer_parse_string(&data, &size, &str_filesize))
            goto error;
        if (!cfrds_buffer_parse_string(&data, &size, &str_timestamp))
            goto error;

        if (str_kind)
        {
            if (strcmp(str_kind, "F:") == 0 || strcmp(str_kind, "F") == 0)
                file_type = 'F';
            else if (strcmp(str_kind, "D:") == 0 || strcmp(str_kind, "D") == 0)
                file_type = 'D';
        }

        if (str_permissions)
        {
            char *endptr = NULL;
            permissions = strtol(str_permissions, &endptr, 10);
            if (endptr == str_permissions || *endptr != '\0')
                goto error;
        }

        if (str_filesize)
        {
            char *endptr = NULL;
            filesize = strtol(str_filesize, &endptr, 10);
            if (endptr == str_filesize || *endptr != '\0')
                goto error;
        }

        if (str_timestamp)
        {
            char *endptr = NULL;
            int64_t n1 = strtoll(str_timestamp, &endptr, 10);
            if (endptr == str_timestamp || *endptr != ',')
                goto error;

            const char *str_num2 = endptr + 1;
            endptr = NULL;
            int64_t n2 = strtoll(str_num2, &endptr, 10);
            if (endptr == str_num2 || *endptr != '\0')
                goto error;

            uint32_t num1 = (uint32_t)(uint64_t)n1;
            uint64_t num2 = (uint64_t)n2;

            modified = num1 + (num2 << 32);
            modified /= 10000;
            modified -= 11644473600000L;
        }

        if (((file_type != 'D') && (file_type != 'F')) || (!filename) || (permissions < 0) || (permissions > 0xff) || (filesize < 0))
            goto error;

        tmp->items[i].kind = file_type;
        tmp->items[i].name = filename;
        filename = NULL;
        tmp->items[i].permissions = (uint8_t)permissions;
        tmp->items[i].size = (size_t)filesize;
        tmp->items[i].modified = modified;
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_browse_dir_free(tmp);
    return NULL;
}

cfrds_file_content *cfrds_buffer_to_file_content(cfrds_buffer *buffer)
{
    cfrds_file_content *ret = NULL;
    cfrds_file_content_defer(tmp);

    if (buffer == NULL)
        return NULL;

    const char *data = (const char *)buffer->data;
    size_t size = buffer->size;

    int64_t total = 0;
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
        !cfrds_buffer_parse_string(&data, &size, &tmp->permission) ||
        size != 0)
    {
        return NULL;
    }

    ret = tmp;
    tmp = NULL;

    return ret;
}

cfrds_sql_dsninfo *cfrds_buffer_to_sql_dsninfo(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_dsninfo, names) + sizeof(char *) * ucnt;
    cfrds_sql_dsninfo *tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parse_string(&data, &size, &item))
            goto error;

        if (item == NULL)
            goto error;

        char *dsn_name = NULL;
        const char *item_buf = item;
        size_t item_len = strlen(item_buf);
        if (!cfrds_buffer_parse_string_list_item(&item_buf, &item_len, &dsn_name))
            goto error;

        tmp->names[i] = dsn_name;
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_dsninfo_free(tmp);
    return NULL;
}

cfrds_sql_tableinfo *cfrds_buffer_to_sql_tableinfo(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_tableinfo, items) + sizeof(cfrds_sql_tableinfoitem) * ucnt;
    cfrds_sql_tableinfo *tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parse_string(&data, &size, &item))
            goto error;

        if (item == NULL)
            goto error;

        cfrds_str_defer(field1);
        cfrds_str_defer(field2);
        cfrds_str_defer(field3);
        cfrds_str_defer(field4);

        const char *column_buf = item;
        size_t list_remaining = strlen(column_buf);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field1) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field2) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field3) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field4) ||
            list_remaining != 0)
        {
            goto error;
        }

        tmp->items[i].unknown = field1; field1 = NULL;
        tmp->items[i].schema  = field2; field2 = NULL;
        tmp->items[i].name    = field3; field3 = NULL;
        tmp->items[i].type    = field4; field4 = NULL;
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_tableinfo_free(tmp);
    return NULL;
}

cfrds_sql_columninfo *cfrds_buffer_to_sql_columninfo(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t columns = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &columns))
        return NULL;

    if (columns < 0 || columns > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucolumns = (size_t)columns;
    size_t malloc_size = offsetof(cfrds_sql_columninfo, items) + sizeof(cfrds_sql_columninfoitem) * ucolumns;
    cfrds_sql_columninfo *tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    for (size_t i = 0; i < ucolumns; i++)
    {
        cfrds_str_defer(row_buf);
        if (!cfrds_buffer_parse_string(&data, &size, &row_buf))
            goto error;

        if (row_buf == NULL)
            goto error;

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

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field1) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field2) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field3) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field4) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field5) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field6) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field7) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field8) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field9) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field10) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field11))
        {
            goto error;
        }

        if (list_remaining > 0)
        {
            if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &field12))
                goto error;
        }
        if (list_remaining != 0)
            goto error;

        tmp->items[i].schema    = field1; field1 = NULL;
        tmp->items[i].owner     = field2; field2 = NULL;
        tmp->items[i].table     = field3; field3 = NULL;
        tmp->items[i].name      = field4; field4 = NULL;
        tmp->items[i].type      = atoi(field5);
        tmp->items[i].typeStr   = field6; field6 = NULL;
        tmp->items[i].precision = atoi(field7);
        tmp->items[i].length    = atoi(field8);
        tmp->items[i].scale     = atoi(field9);
        tmp->items[i].radix     = atoi(field10);
        tmp->items[i].nullable  = atoi(field11);
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_columninfo_free(tmp);
    return NULL;
}

cfrds_sql_primarykeys *cfrds_buffer_to_sql_primarykeys(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(cfrds_sql_primarykeys, items) + sizeof(cfrds_sql_primarykeysitem) * ucnt;
    cfrds_sql_primarykeys *tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parse_string(&data, &size, &item))
            goto error;

        if (item == NULL)
            goto error;

        const char *column_buf = item;
        size_t list_remaining = strlen(column_buf);

        cfrds_str_defer(tableCatalog);
        cfrds_str_defer(tableOwner);
        cfrds_str_defer(tableName);
        cfrds_str_defer(colName);
        cfrds_str_defer(keySequence);

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableCatalog) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableOwner) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &tableName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &colName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence) ||
            list_remaining != 0)
        {
            goto error;
        }

        tmp->items[i].tableCatalog = tableCatalog; tableCatalog = NULL;
        tmp->items[i].tableOwner   = tableOwner; tableOwner = NULL;
        tmp->items[i].tableName    = tableName; tableName = NULL;
        tmp->items[i].colName      = colName; colName = NULL;
        tmp->items[i].keySequence  = atoi(keySequence);
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_primarykeys_free(tmp);
    return NULL;
}


static struct cfrds_sql_keyinfo *cfrds_buffer_to_sql_keyinfo(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t malloc_size = offsetof(struct cfrds_sql_keyinfo, items) + sizeof(cfrds_sql_keyinfoitem) * ucnt;
    struct cfrds_sql_keyinfo *tmp = malloc(malloc_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, malloc_size);

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parse_string(&data, &size, &item))
            goto error;

        if (item == NULL)
            goto error;

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

        if (!cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableCatalog) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableOwner) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkTableName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &pkColName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableCatalog) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableOwner) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkTableName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &fkColName) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &keySequence) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &updateRule) ||
            !cfrds_buffer_parse_string_list_item(&column_buf, &list_remaining, &deleteRule) ||
            list_remaining != 0)
        {
            goto error;
        }

        cfrds_sql_foreignkeysitem *target = &tmp->items[i];
        target->pkTableCatalog = pkTableCatalog; pkTableCatalog = NULL;
        target->pkTableOwner   = pkTableOwner; pkTableOwner = NULL;
        target->pkTableName    = pkTableName; pkTableName = NULL;
        target->pkColName      = pkColName; pkColName = NULL;
        target->fkTableCatalog = fkTableCatalog; fkTableCatalog = NULL;
        target->fkTableOwner   = fkTableOwner; fkTableOwner = NULL;
        target->fkTableName    = fkTableName; fkTableName = NULL;
        target->fkColName      = fkColName; fkColName = NULL;
        target->keySequence    = atoi(keySequence);
        target->updateRule     = atoi(updateRule);
        target->deleteRule     = atoi(deleteRule);
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_foreignkeys_free((cfrds_sql_foreignkeys *)tmp);
    return NULL;
}

cfrds_sql_foreignkeys *cfrds_buffer_to_sql_foreignkeys(cfrds_buffer *buffer)
{
    return (cfrds_sql_foreignkeys *)cfrds_buffer_to_sql_keyinfo(buffer);
}

cfrds_sql_importedkeys *cfrds_buffer_to_sql_importedkeys(cfrds_buffer *buffer)
{
    return (cfrds_sql_importedkeys *)cfrds_buffer_to_sql_keyinfo(buffer);
}

cfrds_sql_exportedkeys *cfrds_buffer_to_sql_exportedkeys(cfrds_buffer *buffer)
{
    return (cfrds_sql_exportedkeys *)cfrds_buffer_to_sql_keyinfo(buffer);
}

cfrds_sql_resultset *cfrds_buffer_to_sql_sqlstmnt(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *response_data = cfrds_buffer_data(buffer);
    size_t response_size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&response_data, &response_size, &cnt))
        return NULL;

    if (cnt < 1 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t rows = (size_t)(cnt - 1);

    cfrds_str_defer(col_row);
    if (!cfrds_buffer_parse_string(&response_data, &response_size, &col_row))
        return NULL;

    if (col_row == NULL)
        return NULL;

    size_t cols = 0;
    const char *col_walker = col_row;
    size_t col_size = strlen(col_walker);
    while (col_size > 0)
    {
        cfrds_str_defer(field);
        if (!cfrds_buffer_parse_string_list_item(&col_walker, &col_size, &field))
            return NULL;
        cols++;
    }

    if (cols < 1)
        return NULL;

    size_t total_row_count = (size_t)cnt;
    size_t buf_size = offsetof(cfrds_sql_resultset, values) + sizeof(char *) * total_row_count * cols;
    cfrds_sql_resultset *tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);
    tmp->columns = cols;
    tmp->rows = rows;

    col_walker = col_row;
    col_size = strlen(col_walker);
    for (size_t c = 0; c < cols; c++)
    {
        char *field = NULL;
        if (!cfrds_buffer_parse_string_list_item(&col_walker, &col_size, &field))
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }
        tmp->values[0 * cols + c] = field;
    }

    for (size_t r = 1; r <= rows; r++)
    {
        cfrds_str_defer(row);
        if (!cfrds_buffer_parse_string(&response_data, &response_size, &row))
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }

        if (row == NULL)
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }

        const char *row_walker = row;
        size_t row_size = strlen(row_walker);
        for (size_t c = 0; c < cols; c++)
        {
            char *field = NULL;
            if (!cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field))
            {
                cfrds_sql_resultset_free(tmp);
                return NULL;
            }
            tmp->values[r * cols + c] = field;
        }
        if (row_size != 0)
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }
    }

    if (response_size != 0)
    {
        cfrds_sql_resultset_free(tmp);
        return NULL;
    }

    return tmp;
}

cfrds_sql_metadata *cfrds_buffer_to_sql_metadata(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &cnt))
        return NULL;

    if (cnt < 0 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t ucnt = (size_t)cnt;
    size_t buf_size = offsetof(cfrds_sql_metadata, items) + sizeof(cfrds_sql_metadataitem) * ucnt;
    cfrds_sql_metadata *tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(row);
        if (!cfrds_buffer_parse_string(&data, &size, &row))
            goto error;

        if (row == NULL)
            goto error;

        const char *row_walker = row;
        size_t row_size = strlen(row_walker);

        char *field1 = NULL;
        char *field2 = NULL;
        char *field3 = NULL;

        if (!cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field1) ||
            !cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field2) ||
            !cfrds_buffer_parse_string_list_item(&row_walker, &row_size, &field3) ||
            row_size != 0)
        {
            free(field1);
            free(field2);
            free(field3);
            goto error;
        }

        tmp->items[i].name = field1;
        tmp->items[i].type = field2;
        tmp->items[i].jtype = field3;
        tmp->cnt++;
    }

    if (size != 0)
        goto error;

    return tmp;

error:
    cfrds_sql_metadata_free(tmp);
    return NULL;
}

cfrds_sql_supportedcommands *cfrds_buffer_to_sql_supportedcommands(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t rows = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return NULL;

    if (rows != 1)
        return NULL;

    cfrds_str_defer(commands_str);
    if (!cfrds_buffer_parse_string(&data, &size, &commands_str))
        return NULL;

    if (commands_str == NULL || size != 0)
        return NULL;

    size_t cnt = 0;
    const char *start_data = commands_str;
    size_t start_size = strlen(commands_str);
    while (start_size > 0)
    {
        cfrds_str_defer(field);
        if (!cfrds_buffer_parse_string_list_item(&start_data, &start_size, &field))
            return NULL;
        cnt++;
    }

    size_t buf_size = offsetof(cfrds_sql_supportedcommands, commands) + sizeof(char *) * cnt;
    cfrds_sql_supportedcommands *tmp = malloc(buf_size);
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, buf_size);
    tmp->cnt = cnt;

    start_data = commands_str;
    start_size = strlen(commands_str);
    for (size_t c = 0; c < cnt; c++)
    {
        char *field = NULL;
        if (!cfrds_buffer_parse_string_list_item(&start_data, &start_size, &field))
        {
            cfrds_sql_supportedcommands_free(tmp);
            return NULL;
        }
        tmp->commands[c] = field;
    }

    return tmp;
}

char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);

    int64_t rows = 0;
    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return NULL;

    if (rows != 1)
        return NULL;

    cfrds_str_defer(row);
    if (!cfrds_buffer_parse_string(&data, &size, &row))
        return NULL;

    if (row == NULL || size != 0)
        return NULL;

    char *ret = NULL;
    const char *row_ptr = row;
    size_t row_len = strlen(row);

    if (!cfrds_buffer_parse_string_list_item(&row_ptr, &row_len, &ret) || row_len != 0)
    {
        free(ret);
        return NULL;
    }

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
