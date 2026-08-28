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
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

void cfrds_buffer_cleanup(cfrds_buffer **buf) {
    if (buf && *buf) {
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

/* ------------------------------------------------------------------------- */
/* Parser cursor: bundles the (data, remaining) pair so every cfrds_buffer_to_* */
/* decoder shares one bounds-checked parsing surface. Adding a field to a     */
/* decoder is now just another parser call; forgetting the bounds check is no */
/* longer possible because the primitives always validate against remaining.  */
/* ------------------------------------------------------------------------- */

typedef struct {
    const char *data;
    size_t remaining;
} cfrds_buffer_parser;

static void cfrds_buffer_parser_begin(cfrds_buffer_parser *parser, cfrds_buffer *buffer)
{
    parser->data = (const char *)buffer->data;
    parser->remaining = buffer->size;
}

static void cfrds_buffer_parser_begin_string(cfrds_buffer_parser *parser, const char *str)
{
    parser->data = str;
    parser->remaining = strlen(str);
}

static bool cfrds_buffer_parser_number(cfrds_buffer_parser *parser, int64_t *out)
{
    return cfrds_buffer_parse_number(&parser->data, &parser->remaining, out);
}

static bool cfrds_buffer_parser_string(cfrds_buffer_parser *parser, char **out)
{
    return cfrds_buffer_parse_string(&parser->data, &parser->remaining, out);
}

static bool cfrds_buffer_parser_bytearray(cfrds_buffer_parser *parser, char **out, size_t *out_size)
{
    return cfrds_buffer_parse_bytearray(&parser->data, &parser->remaining, out, out_size);
}

static bool cfrds_buffer_parser_string_list_item(cfrds_buffer_parser *parser, char **out)
{
    return cfrds_buffer_parse_string_list_item(&parser->data, &parser->remaining, out);
}

static bool cfrds_buffer_parser_strings(cfrds_buffer_parser *parser, char **fields[], size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (!cfrds_buffer_parser_string(parser, fields[i]))
            return false;
    }

    return true;
}

static bool cfrds_buffer_parser_string_list_items(cfrds_buffer_parser *parser, char **fields[], size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        if (!cfrds_buffer_parser_string_list_item(parser, fields[i]))
            return false;
    }

    return true;
}

static bool cfrds_buffer_parser_at_end(const cfrds_buffer_parser *parser)
{
    return parser->remaining == 0;
}

/* Parse the leading count of a response and enforce the global item cap. */
static bool cfrds_buffer_parser_count(cfrds_buffer_parser *parser, size_t *out)
{
    int64_t cnt = 0;

    if (!cfrds_buffer_parser_number(parser, &cnt))
        return false;

    if (cnt < 0 || cnt > (int64_t)CFRDS_MAX_PARSER_ITEMS)
        return false;

    *out = (size_t)cnt;
    return true;
}

/* Allocate a struct with a trailing flexible array, overflow-checked and bzero'd. */
static void *cfrds_buffer_parser_alloc(size_t header_offset, size_t element_size, size_t count, size_t *allocated)
{
    size_t total = 0;
    void *result = NULL;

    if (element_size != 0 && count > (SIZE_MAX - header_offset) / element_size)
        return NULL;

    total = header_offset + element_size * count;
    result = malloc(total);
    if (result == NULL)
        return NULL;

    explicit_bzero(result, total);

    if (allocated)
        *allocated = total;

    return result;
}

cfrds_browse_dir *cfrds_buffer_to_browse_dir(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    int64_t total_elements = 0;
    if (!cfrds_buffer_parser_number(&parser, &total_elements))
        return NULL;

    if (total_elements < 0 || total_elements % 5 != 0 || total_elements > (int64_t)CFRDS_MAX_PARSER_ITEMS * 5)
        return NULL;

    size_t item_count = (size_t)(total_elements / 5);
    cfrds_browse_dir *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_browse_dir, items), sizeof(cfrds_browse_dir_item), item_count, NULL);
    if (tmp == NULL)
        return NULL;

    tmp->cnt = 0;

    for (size_t i = 0; i < item_count; i++)
    {
        cfrds_str_defer(str_kind);
        cfrds_str_defer(filename);
        cfrds_str_defer(str_permissions);
        cfrds_str_defer(str_filesize);
        cfrds_str_defer(str_timestamp);

        char **fields[] = { &str_kind, &filename, &str_permissions, &str_filesize, &str_timestamp };
        if (!cfrds_buffer_parser_strings(&parser, fields, 5))
            goto error;

        char file_type = '\0';
        ssize_t permissions = -1;
        ssize_t filesize = -1;
        uint64_t modified = UINT64_MAX;

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

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    int64_t total = 0;
    if (!cfrds_buffer_parser_number(&parser, &total))
        return NULL;

    if (total != 3)
        return NULL;

    tmp = malloc(sizeof(cfrds_file_content));
    if (tmp == NULL)
        return NULL;

    explicit_bzero(tmp, sizeof(cfrds_file_content));

    if (!cfrds_buffer_parser_bytearray(&parser, &tmp->data, &tmp->size) ||
        !cfrds_buffer_parser_string(&parser, &tmp->modified) ||
        !cfrds_buffer_parser_string(&parser, &tmp->permission) ||
        !cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucnt = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucnt))
        return NULL;

    cfrds_sql_dsninfo *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_dsninfo, names), sizeof(char *), ucnt, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parser_string(&parser, &item))
            goto error;

        if (item == NULL)
            goto error;

        cfrds_buffer_parser list_parser;
        cfrds_buffer_parser_begin_string(&list_parser, item);

        if (!cfrds_buffer_parser_string_list_item(&list_parser, &tmp->names[i]))
            goto error;

        tmp->cnt++;
    }

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucnt = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucnt))
        return NULL;

    cfrds_sql_tableinfo *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_tableinfo, items), sizeof(cfrds_sql_tableinfoitem), ucnt, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parser_string(&parser, &item))
            goto error;

        if (item == NULL)
            goto error;

        cfrds_str_defer(field1);
        cfrds_str_defer(field2);
        cfrds_str_defer(field3);
        cfrds_str_defer(field4);

        cfrds_buffer_parser list_parser;
        cfrds_buffer_parser_begin_string(&list_parser, item);

        if (!cfrds_buffer_parser_string_list_items(&list_parser, (char **[]){ &field1, &field2, &field3, &field4 }, 4) ||
            !cfrds_buffer_parser_at_end(&list_parser))
        {
            goto error;
        }

        tmp->items[i].unknown = field1; field1 = NULL;
        tmp->items[i].schema  = field2; field2 = NULL;
        tmp->items[i].name    = field3; field3 = NULL;
        tmp->items[i].type    = field4; field4 = NULL;
        tmp->cnt++;
    }

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucolumns = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucolumns))
        return NULL;

    cfrds_sql_columninfo *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_columninfo, items), sizeof(cfrds_sql_columninfoitem), ucolumns, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucolumns; i++)
    {
        cfrds_str_defer(row_buf);
        if (!cfrds_buffer_parser_string(&parser, &row_buf))
            goto error;

        if (row_buf == NULL)
            goto error;

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

        cfrds_buffer_parser list_parser;
        cfrds_buffer_parser_begin_string(&list_parser, row_buf);

        if (!cfrds_buffer_parser_string_list_items(&list_parser, (char **[]){ &field1, &field2, &field3, &field4, &field5, &field6, &field7, &field8, &field9, &field10, &field11 }, 11))
            goto error;

        if (!cfrds_buffer_parser_at_end(&list_parser))
        {
            if (!cfrds_buffer_parser_string_list_item(&list_parser, &field12))
                goto error;
        }
        if (!cfrds_buffer_parser_at_end(&list_parser))
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

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucnt = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucnt))
        return NULL;

    cfrds_sql_primarykeys *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_primarykeys, items), sizeof(cfrds_sql_primarykeysitem), ucnt, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parser_string(&parser, &item))
            goto error;

        if (item == NULL)
            goto error;

        cfrds_str_defer(tableCatalog);
        cfrds_str_defer(tableOwner);
        cfrds_str_defer(tableName);
        cfrds_str_defer(colName);
        cfrds_str_defer(keySequence);

        cfrds_buffer_parser list_parser;
        cfrds_buffer_parser_begin_string(&list_parser, item);

        if (!cfrds_buffer_parser_string_list_items(&list_parser, (char **[]){ &tableCatalog, &tableOwner, &tableName, &colName, &keySequence }, 5) ||
            !cfrds_buffer_parser_at_end(&list_parser))
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

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucnt = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucnt))
        return NULL;

    struct cfrds_sql_keyinfo *tmp = cfrds_buffer_parser_alloc(offsetof(struct cfrds_sql_keyinfo, items), sizeof(cfrds_sql_keyinfoitem), ucnt, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(item);
        if (!cfrds_buffer_parser_string(&parser, &item))
            goto error;

        if (item == NULL)
            goto error;

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

        cfrds_buffer_parser list_parser;
        cfrds_buffer_parser_begin_string(&list_parser, item);

        if (!cfrds_buffer_parser_string_list_items(&list_parser, (char **[]){ &pkTableCatalog, &pkTableOwner, &pkTableName, &pkColName, &fkTableCatalog, &fkTableOwner, &fkTableName, &fkColName, &keySequence, &updateRule, &deleteRule }, 11) ||
            !cfrds_buffer_parser_at_end(&list_parser))
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

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    int64_t cnt = 0;
    if (!cfrds_buffer_parser_number(&parser, &cnt))
        return NULL;

    if (cnt < 1 || cnt > CFRDS_MAX_PARSER_ITEMS)
        return NULL;

    size_t rows = (size_t)(cnt - 1);
    size_t total_row_count = (size_t)cnt;

    cfrds_str_defer(col_row);
    if (!cfrds_buffer_parser_string(&parser, &col_row))
        return NULL;

    if (col_row == NULL)
        return NULL;

    size_t cols = 0;
    cfrds_buffer_parser list_parser;
    cfrds_buffer_parser_begin_string(&list_parser, col_row);
    while (!cfrds_buffer_parser_at_end(&list_parser))
    {
        cfrds_str_defer(field);
        if (!cfrds_buffer_parser_string_list_item(&list_parser, &field))
            return NULL;
        cols++;
    }

    if (cols < 1)
        return NULL;

    cfrds_sql_resultset *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_resultset, values), sizeof(char *), total_row_count * cols, NULL);
    if (tmp == NULL)
        return NULL;

    tmp->columns = cols;
    tmp->rows = rows;

    cfrds_buffer_parser_begin_string(&list_parser, col_row);
    for (size_t c = 0; c < cols; c++)
    {
        if (!cfrds_buffer_parser_string_list_item(&list_parser, &tmp->values[c]))
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }
    }

    for (size_t r = 1; r <= rows; r++)
    {
        cfrds_str_defer(row);
        if (!cfrds_buffer_parser_string(&parser, &row))
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }

        if (row == NULL)
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }

        cfrds_buffer_parser row_parser;
        cfrds_buffer_parser_begin_string(&row_parser, row);
        for (size_t c = 0; c < cols; c++)
        {
            if (!cfrds_buffer_parser_string_list_item(&row_parser, &tmp->values[r * cols + c]))
            {
                cfrds_sql_resultset_free(tmp);
                return NULL;
            }
        }
        if (!cfrds_buffer_parser_at_end(&row_parser))
        {
            cfrds_sql_resultset_free(tmp);
            return NULL;
        }
    }

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    size_t ucnt = 0;
    if (!cfrds_buffer_parser_count(&parser, &ucnt))
        return NULL;

    cfrds_sql_metadata *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_metadata, items), sizeof(cfrds_sql_metadataitem), ucnt, NULL);
    if (tmp == NULL)
        return NULL;

    for (size_t i = 0; i < ucnt; i++)
    {
        cfrds_str_defer(row);
        if (!cfrds_buffer_parser_string(&parser, &row))
            goto error;

        if (row == NULL)
            goto error;

        cfrds_str_defer(field1);
        cfrds_str_defer(field2);
        cfrds_str_defer(field3);

        cfrds_buffer_parser row_parser;
        cfrds_buffer_parser_begin_string(&row_parser, row);

        if (!cfrds_buffer_parser_string_list_items(&row_parser, (char **[]){ &field1, &field2, &field3 }, 3) ||
            !cfrds_buffer_parser_at_end(&row_parser))
        {
            goto error;
        }

        tmp->items[i].name = field1; field1 = NULL;
        tmp->items[i].type = field2; field2 = NULL;
        tmp->items[i].jtype = field3; field3 = NULL;
        tmp->cnt++;
    }

    if (!cfrds_buffer_parser_at_end(&parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    int64_t rows = 0;
    if (!cfrds_buffer_parser_number(&parser, &rows))
        return NULL;

    if (rows != 1)
        return NULL;

    cfrds_str_defer(commands_str);
    if (!cfrds_buffer_parser_string(&parser, &commands_str))
        return NULL;

    if (commands_str == NULL || !cfrds_buffer_parser_at_end(&parser))
        return NULL;

    size_t cnt = 0;
    cfrds_buffer_parser list_parser;
    cfrds_buffer_parser_begin_string(&list_parser, commands_str);
    while (!cfrds_buffer_parser_at_end(&list_parser))
    {
        cfrds_str_defer(field);
        if (!cfrds_buffer_parser_string_list_item(&list_parser, &field))
            return NULL;
        cnt++;
    }

    cfrds_sql_supportedcommands *tmp = cfrds_buffer_parser_alloc(offsetof(cfrds_sql_supportedcommands, commands), sizeof(char *), cnt, NULL);
    if (tmp == NULL)
        return NULL;

    tmp->cnt = cnt;

    cfrds_buffer_parser_begin_string(&list_parser, commands_str);
    for (size_t c = 0; c < cnt; c++)
    {
        if (!cfrds_buffer_parser_string_list_item(&list_parser, &tmp->commands[c]))
        {
            cfrds_sql_supportedcommands_free(tmp);
            return NULL;
        }
    }

    return tmp;
}

char *cfrds_buffer_to_sql_dbdescription(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return NULL;

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    int64_t rows = 0;
    if (!cfrds_buffer_parser_number(&parser, &rows))
        return NULL;

    if (rows != 1)
        return NULL;

    cfrds_str_defer(row);
    if (!cfrds_buffer_parser_string(&parser, &row))
        return NULL;

    if (row == NULL || !cfrds_buffer_parser_at_end(&parser))
        return NULL;

    char *ret = NULL;
    cfrds_buffer_parser row_parser;
    cfrds_buffer_parser_begin_string(&row_parser, row);

    if (!cfrds_buffer_parser_string_list_item(&row_parser, &ret) || !cfrds_buffer_parser_at_end(&row_parser))
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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    cfrds_buffer_parser_number(&parser, &rows);
    if (rows != 2)
        return NULL;

    cfrds_buffer_parser_string(&parser, &ret);

    return ret;
}

bool cfrds_buffer_to_debugger_stop(cfrds_buffer *buffer)
{
    cfrds_str_defer(xml);

    int64_t rows = 0;

    if (buffer == NULL)
        return false;

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    cfrds_buffer_parser_number(&parser, &rows);
    if (rows != 1)
        return false;

    cfrds_buffer_parser_string(&parser, &xml);

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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    cfrds_buffer_parser_number(&parser, &rows);
    if (rows != 1)
        return -1;

    cfrds_buffer_parser_string(&parser, &ret);

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

    cfrds_buffer_parser parser;
    cfrds_buffer_parser_begin(&parser, buffer);

    cfrds_buffer_parser_number(&parser, &rows);
    if (rows != 1)
        return NULL;

    cfrds_buffer_parser_string(&parser, &xml);
    if (xml)
        return (cfrds_debugger_event *)wddx_from_xml(xml);

    return NULL;
}
