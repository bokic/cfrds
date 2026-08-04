// cppcheck-suppress-file unusedFunction
// cppcheck-suppress-file staticFunction
#include <internal/explicit_bzero.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/wddx.h>
#include <internal/cfrds_int.h>
#include <cfrds.h>

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>



void cfrds_file_content_free(cfrds_file_content *value)
{
    if (value == NULL)
        return;

    free(value->data);
    free(value->modified);
    free(value->permission);
    free(value);
}

const char *cfrds_file_content_get_data(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    return value->data;
}

size_t cfrds_file_content_get_size(const cfrds_file_content *value)
{
    if (value == NULL)
        return 0;

    return value->size;
}

const char *cfrds_file_content_get_modified(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    return value->modified;
}

const char *cfrds_file_content_get_permission(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    return value->permission;
}

void cfrds_browse_dir_free(cfrds_browse_dir *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c < value->cnt; c++)
    {
        free(value->items[c].name);
    }

    free(value);
}

size_t cfrds_browse_dir_count(const cfrds_browse_dir *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

#define CFRDS_CHECK_BOUNDS(value, ndx, default_ret) \
    do { \
        if ((value) == NULL || (ndx) >= (value)->cnt) \
            return (default_ret); \
    } while (0)

#include <internal/cfrds_accessors.h>

DEFINE_CHAR_ACCESSOR(cfrds_browse_dir_item_get_kind, cfrds_browse_dir, kind, 0)
DEFINE_STRING_ACCESSOR(cfrds_browse_dir_item_get_name, cfrds_browse_dir, name)
DEFINE_UINT8_ACCESSOR(cfrds_browse_dir_item_get_permissions, cfrds_browse_dir, permissions, 0)
DEFINE_SIZE_ACCESSOR(cfrds_browse_dir_item_get_size, cfrds_browse_dir, size, 0)
DEFINE_UINT64_ACCESSOR(cfrds_browse_dir_item_get_modified, cfrds_browse_dir, modified, 0)

void cfrds_sql_dsninfo_free(cfrds_sql_dsninfo *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c < value->cnt; c++)
    {
        free(value->names[c]);
    }

    free(value);
}

size_t cfrds_sql_dsninfo_count(const cfrds_sql_dsninfo *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

const char *cfrds_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    if (ndx >= value->cnt)
        return NULL;

    return value->names[ndx];
}

void cfrds_sql_tableinfo_free(cfrds_sql_tableinfo *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c < value->cnt; c++)
    {
        if(value->items[c].unknown) free(value->items[c].unknown);
        if(value->items[c].schema) free(value->items[c].schema);
        if(value->items[c].name) free(value->items[c].name);
        if(value->items[c].type) free(value->items[c].type);
    }

    free(value);
}

size_t cfrds_sql_tableinfo_count(const cfrds_sql_tableinfo *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_tableinfo_get_column_unknown, cfrds_sql_tableinfo, unknown)
DEFINE_STRING_ACCESSOR(cfrds_sql_tableinfo_get_column_schema, cfrds_sql_tableinfo, schema)
DEFINE_STRING_ACCESSOR(cfrds_sql_tableinfo_get_column_name, cfrds_sql_tableinfo, name)
DEFINE_STRING_ACCESSOR(cfrds_sql_tableinfo_get_column_type, cfrds_sql_tableinfo, type)

void cfrds_sql_columninfo_free(cfrds_sql_columninfo *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c < value->cnt; c++)
    {
        if(value->items[c].schema) free(value->items[c].schema);
        if(value->items[c].owner) free(value->items[c].owner);
        if(value->items[c].table) free(value->items[c].table);
        if(value->items[c].name) free(value->items[c].name);
        if(value->items[c].typeStr) free(value->items[c].typeStr);
    }

    free(value);
}

size_t cfrds_sql_columninfo_count(const cfrds_sql_columninfo *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_columninfo_get_schema, cfrds_sql_columninfo, schema)
DEFINE_STRING_ACCESSOR(cfrds_sql_columninfo_get_owner, cfrds_sql_columninfo, owner)
DEFINE_STRING_ACCESSOR(cfrds_sql_columninfo_get_table, cfrds_sql_columninfo, table)
DEFINE_STRING_ACCESSOR(cfrds_sql_columninfo_get_name, cfrds_sql_columninfo, name)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_type, cfrds_sql_columninfo, type, -1)
DEFINE_STRING_ACCESSOR(cfrds_sql_columninfo_get_typeStr, cfrds_sql_columninfo, typeStr)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_precision, cfrds_sql_columninfo, precision, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_length, cfrds_sql_columninfo, length, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_scale, cfrds_sql_columninfo, scale, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_radix, cfrds_sql_columninfo, radix, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_columninfo_get_nullable, cfrds_sql_columninfo, nullable, -1)

void cfrds_sql_primarykeys_free(cfrds_sql_primarykeys *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c < value->cnt; c++)
    {
        if(value->items[c].tableCatalog) free(value->items[c].tableCatalog);
        if(value->items[c].tableOwner) free(value->items[c].tableOwner);
        if(value->items[c].tableName) free(value->items[c].tableName);
        if(value->items[c].colName) free(value->items[c].colName);
    }

    free(value);
}

size_t cfrds_sql_primarykeys_count(const cfrds_sql_primarykeys *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_primarykeys_get_catalog, cfrds_sql_primarykeys, tableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_primarykeys_get_owner, cfrds_sql_primarykeys, tableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_primarykeys_get_table, cfrds_sql_primarykeys, tableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_primarykeys_get_column, cfrds_sql_primarykeys, colName)
DEFINE_INT_ACCESSOR(cfrds_sql_primarykeys_get_key_sequence, cfrds_sql_primarykeys, keySequence, -1)

static void cfrds_sql_keyitems_free(cfrds_sql_foreignkeysitem *items, size_t cnt)
{
    if (items == NULL)
        return;

    for (size_t c = 0; c < cnt; c++)
    {
        free(items[c].pkTableCatalog);
        free(items[c].pkTableOwner);
        free(items[c].pkTableName);
        free(items[c].pkColName);
        free(items[c].fkTableCatalog);
        free(items[c].fkTableOwner);
        free(items[c].fkTableName);
        free(items[c].fkColName);
    }
}

void cfrds_sql_foreignkeys_free(cfrds_sql_foreignkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_keyitems_free(value->items, value->cnt);
    free(value);
}

size_t cfrds_sql_foreignkeys_count(const cfrds_sql_foreignkeys *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_pkcatalog, cfrds_sql_foreignkeys, pkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_pkowner, cfrds_sql_foreignkeys, pkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_pktable, cfrds_sql_foreignkeys, pkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_pkcolumn, cfrds_sql_foreignkeys, pkColName)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_fkcatalog, cfrds_sql_foreignkeys, fkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_fkowner, cfrds_sql_foreignkeys, fkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_fktable, cfrds_sql_foreignkeys, fkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_foreignkeys_get_fkcolumn, cfrds_sql_foreignkeys, fkColName)
DEFINE_INT_ACCESSOR(cfrds_sql_foreignkeys_get_key_sequence, cfrds_sql_foreignkeys, keySequence, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_foreignkeys_get_updaterule, cfrds_sql_foreignkeys, updateRule, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_foreignkeys_get_deleterule, cfrds_sql_foreignkeys, deleteRule, -1)

void cfrds_sql_importedkeys_free(cfrds_sql_importedkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_keyitems_free((cfrds_sql_foreignkeysitem *)value->items, value->cnt);
    free(value);
}

size_t cfrds_sql_importedkeys_count(const cfrds_sql_importedkeys *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_pkcatalog, cfrds_sql_importedkeys, pkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_pkowner, cfrds_sql_importedkeys, pkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_pktable, cfrds_sql_importedkeys, pkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_pkcolumn, cfrds_sql_importedkeys, pkColName)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_fkcatalog, cfrds_sql_importedkeys, fkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_fkowner, cfrds_sql_importedkeys, fkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_fktable, cfrds_sql_importedkeys, fkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_importedkeys_get_fkcolumn, cfrds_sql_importedkeys, fkColName)
DEFINE_INT_ACCESSOR(cfrds_sql_importedkeys_get_key_sequence, cfrds_sql_importedkeys, keySequence, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_importedkeys_get_updaterule, cfrds_sql_importedkeys, updateRule, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_importedkeys_get_deleterule, cfrds_sql_importedkeys, deleteRule, -1)

void cfrds_sql_exportedkeys_free(cfrds_sql_exportedkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_keyitems_free((cfrds_sql_foreignkeysitem *)value->items, value->cnt);
    free(value);
}

size_t cfrds_sql_exportedkeys_count(const cfrds_sql_exportedkeys *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_pkcatalog, cfrds_sql_exportedkeys, pkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_pkowner, cfrds_sql_exportedkeys, pkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_pktable, cfrds_sql_exportedkeys, pkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_pkcolumn, cfrds_sql_exportedkeys, pkColName)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_fkcatalog, cfrds_sql_exportedkeys, fkTableCatalog)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_fkowner, cfrds_sql_exportedkeys, fkTableOwner)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_fktable, cfrds_sql_exportedkeys, fkTableName)
DEFINE_STRING_ACCESSOR(cfrds_sql_exportedkeys_get_fkcolumn, cfrds_sql_exportedkeys, fkColName)
DEFINE_INT_ACCESSOR(cfrds_sql_exportedkeys_get_key_sequence, cfrds_sql_exportedkeys, keySequence, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_exportedkeys_get_updaterule, cfrds_sql_exportedkeys, updateRule, -1)
DEFINE_INT_ACCESSOR(cfrds_sql_exportedkeys_get_deleterule, cfrds_sql_exportedkeys, deleteRule, -1)

void cfrds_sql_resultset_free(cfrds_sql_resultset *value)
{
    if (value == NULL)
        return;

    for (size_t i = 0; i < (value->rows + 1) * value->columns; i++)
    {
        free(value->values[i]);
    }

    free(value);
}

size_t cfrds_sql_resultset_rows(const cfrds_sql_resultset *value)
{
    if (value == NULL)
        return 0;

    return value->rows;
}

size_t cfrds_sql_resultset_columns(const cfrds_sql_resultset *value)
{
    if (value == NULL)
        return 0;

    return value->columns;
}

const char *cfrds_sql_resultset_column_name(const cfrds_sql_resultset *value, size_t column)
{
    if (value == NULL)
        return NULL;

    if (column >= value->columns)
        return NULL;

    return value->values[column];
}

const char *cfrds_sql_resultset_value(const cfrds_sql_resultset *value, size_t row, size_t column)
{
    if (value == NULL)
        return NULL;

    if (row >= value->rows)
        return NULL;

    if (column >= value->columns)
        return NULL;

    return value->values[(row + 1) * value->columns + column];
}

void cfrds_sql_metadata_free(cfrds_sql_metadata *value)
{
    if (value == NULL)
        return;

    for (size_t c = 0; c < value->cnt; c++)
    {
        free(value->items[c].name);
        free(value->items[c].type);
        free(value->items[c].jtype);
    }

    free(value);
}

size_t cfrds_sql_metadata_count(const cfrds_sql_metadata *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

const char *cfrds_sql_metadata_get_name(const cfrds_sql_metadata *value, size_t column)
{
    if (value == NULL)
        return NULL;

    if (column >= value->cnt)
        return NULL;

    return value->items[column].name;
}

const char *cfrds_sql_metadata_get_type(const cfrds_sql_metadata *value, size_t column)
{
    if (value == NULL)
        return NULL;

    if (column >= value->cnt)
        return NULL;

    return value->items[column].type;
}

const char *cfrds_sql_metadata_get_jtype(const cfrds_sql_metadata *value, size_t column)
{
    if (value == NULL)
        return NULL;

    if (column >= value->cnt)
        return NULL;

    return value->items[column].jtype;
}

void cfrds_sql_supportedcommands_free(cfrds_sql_supportedcommands *value)
{
    if (value == NULL)
        return;

    for (size_t c = 0; c < value->cnt; c++)
    {
        free(value->commands[c]);
    }

    free(value);
}

size_t cfrds_sql_supportedcommands_count(const cfrds_sql_supportedcommands *value)
{
    if (value == NULL)
        return 0;

    return value->cnt;
}

const char *cfrds_sql_supportedcommands_get(const cfrds_sql_supportedcommands *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    if (ndx >= value->cnt)
        return NULL;

    return value->commands[ndx];
}

void cfrds_security_analyzer_result_free(cfrds_security_analyzer_result *buf)
{
    if (buf)
    {
        json_object_put((struct json_object *)buf);
    }
}

void cfrds_security_analyzer_result_cleanup(cfrds_security_analyzer_result **buf)
{
    if (buf)
    {
        cfrds_security_analyzer_result_free(*buf);
        *buf = NULL;
    }
}

/*
 * Static helpers for accessing fields from the cached json_object.
 */
static struct json_object *sa_result_json(const cfrds_security_analyzer_result *value)
{
    return (struct json_object *)value;
}

static int sa_get_int(const cfrds_security_analyzer_result *value, const char *key)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return -1;

    struct json_object *field = NULL;
    json_object_object_get_ex(json_obj, key, &field);
    if (field == NULL)
        return -1;

    if (json_object_get_type(field) != json_type_int)
        return -1;

    return json_object_get_int(field);
}

static int sa_get_array_length(const cfrds_security_analyzer_result *value, const char *key)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return -1;

    struct json_object *field = NULL;
    json_object_object_get_ex(json_obj, key, &field);
    if (field == NULL)
        return -1;

    if (json_object_get_type(field) != json_type_array)
        return -1;

    return (int)json_object_array_length(field);
}

static cfrds_str sa_get_string(const cfrds_security_analyzer_result *value, const char *key)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return NULL;

    struct json_object *field = NULL;
    json_object_object_get_ex(json_obj, key, &field);
    if (field == NULL)
        return NULL;

    if (json_object_get_type(field) != json_type_string)
        return NULL;

    return strdup(json_object_get_string(field));
}

static struct json_object *sa_get_array_item(const cfrds_security_analyzer_result *value, const char *array_key, size_t ndx)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return NULL;

    struct json_object *array = NULL;
    json_object_object_get_ex(json_obj, array_key, &array);
    if (array == NULL)
        return NULL;

    if (json_object_get_type(array) != json_type_array)
        return NULL;

    size_t len = json_object_array_length(array);
    if (ndx >= len)
        return NULL;

    struct json_object *item = json_object_array_get_idx(array, ndx);
    if (item == NULL)
        return NULL;

    if (json_object_get_type(item) != json_type_object)
        return NULL;

    return item;
}

static cfrds_str sa_get_array_item_string(const cfrds_security_analyzer_result *value, const char *array_key, size_t ndx, const char *field_key)
{
    struct json_object *item = sa_get_array_item(value, array_key, ndx);
    if (item == NULL)
        return NULL;

    struct json_object *field = NULL;
    json_object_object_get_ex(item, field_key, &field);
    if (field == NULL)
        return NULL;

    const char *str = json_object_get_string(field);
    if (str == NULL)
        return NULL;

    return strdup(str);
}

static int sa_get_array_item_int(const cfrds_security_analyzer_result *value, const char *array_key, size_t ndx, const char *field_key)
{
    struct json_object *item = sa_get_array_item(value, array_key, ndx);
    if (item == NULL)
        return -1;

    struct json_object *field = NULL;
    json_object_object_get_ex(item, field_key, &field);
    if (field == NULL)
        return -1;

    if (json_object_get_type(field) != json_type_int)
        return -1;

    return json_object_get_int(field);
}

int cfrds_security_analyzer_result_totalfiles(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "totalfiles");
}

int cfrds_security_analyzer_result_filesvisitedcount(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "filesvisitedcount");
}

int cfrds_security_analyzer_result_errorsdescription_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "errorsdescription");
}

int cfrds_security_analyzer_result_filesscanned_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "filesscanned");
}

cfrds_str cfrds_security_analyzer_result_filesscanned_item_result(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "filesscanned", ndx, "result");
}

cfrds_str cfrds_security_analyzer_result_filesscanned_item_filename(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "filesscanned", ndx, "filename");
}

int cfrds_security_analyzer_result_filesnotscanned_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "filesnotscanned");
}

cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_reason(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "filesnotscanned", ndx, "reason");
}

cfrds_str cfrds_security_analyzer_result_filesnotscanned_item_filename(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "filesnotscanned", ndx, "filename");
}

cfrds_str cfrds_security_analyzer_result_executorservice(const cfrds_security_analyzer_result *value)
{
    return sa_get_string(value, "executorservice");
}

int cfrds_security_analyzer_result_percentage(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "percentage");
}

int cfrds_security_analyzer_result_files_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "files");
}

cfrds_str cfrds_security_analyzer_result_files_value(const cfrds_security_analyzer_result *value, size_t ndx)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return NULL;

    struct json_object *files = NULL;
    json_object_object_get_ex(json_obj, "files", &files);
    if (files == NULL)
        return NULL;

    if (json_object_get_type(files) != json_type_array)
        return NULL;

    size_t len = json_object_array_length(files);
    if (ndx >= len)
        return NULL;

    json_object *item = json_object_array_get_idx(files, ndx);
    if (item == NULL)
        return NULL;

    if (json_object_get_type(item) != json_type_string)
        return NULL;

    return strdup(json_object_get_string(item));
}

int64_t cfrds_security_analyzer_result_lastupdated(const cfrds_security_analyzer_result *value)
{
    struct json_object *json_obj = sa_result_json(value);
    if (json_obj == NULL)
        return -1;

    struct json_object *lastupdated = NULL;
    json_object_object_get_ex(json_obj, "lastupdated", &lastupdated);
    if (lastupdated == NULL)
        return -1;

    if (json_object_get_type(lastupdated) != json_type_int)
        return -1;

    return json_object_get_int64(lastupdated);
}

int cfrds_security_analyzer_result_filesvisited_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "filesvisited");
}

int cfrds_security_analyzer_result_filesnotscannedcount(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "filesnotscannedcount");
}

int cfrds_security_analyzer_result_filesscannedcount(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "filesscannedcount");
}

int cfrds_security_analyzer_result_id(const cfrds_security_analyzer_result *value)
{
    return sa_get_int(value, "id");
}

int cfrds_security_analyzer_result_errors_count(const cfrds_security_analyzer_result *value)
{
    return sa_get_array_length(value, "errors");
}

cfrds_str cfrds_security_analyzer_result_errors_item_errormessage(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "errormessage");
}

int cfrds_security_analyzer_result_errors_item_endline(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_int(value, "errors", ndx, "endline");
}

cfrds_str cfrds_security_analyzer_result_errors_item_path(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "path");
}

cfrds_str cfrds_security_analyzer_result_errors_item_vulnerablecode(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "vulnerablecode");
}

cfrds_str cfrds_security_analyzer_result_errors_item_filename(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "filename");
}

int cfrds_security_analyzer_result_errors_item_beginline(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_int(value, "errors", ndx, "beginline");
}

int cfrds_security_analyzer_result_errors_item_column(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_int(value, "errors", ndx, "column");
}

cfrds_str cfrds_security_analyzer_result_errors_item_error(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "Error");
}

int cfrds_security_analyzer_result_errors_item_begincolumn(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_int(value, "errors", ndx, "begincolumn");
}

cfrds_str cfrds_security_analyzer_result_errors_item_type(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "type");
}

int cfrds_security_analyzer_result_errors_item_endcolumn(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_int(value, "errors", ndx, "endcolumn");
}

cfrds_str cfrds_security_analyzer_result_errors_item_referencetype(const cfrds_security_analyzer_result *value, size_t ndx)
{
    return sa_get_array_item_string(value, "errors", ndx, "referencetype");
}

cfrds_str cfrds_security_analyzer_result_status(const cfrds_security_analyzer_result *value)
{
    cfrds_str ret = sa_get_string(value, "status");
    if (ret == NULL)
        return strdup("");

    return ret;
}

cfrds_status cfrds_command_ide_default(cfrds_server *server, int version, int *num1, cfrds_str *server_version, cfrds_str *client_version, int *num2, int *num3)
{
    cfrds_status ret;

    char param[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    /* Protocol quirk: IDE_DEFAULT expects version argument formatted with a trailing comma (e.g. "N,") */
    snprintf(param, sizeof(param), "%d,", version);

    ret = cfrds_send_command(server, &response, "IDE_DEFAULT", (const char *[]){ "", param, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(_num1);
        cfrds_str_defer(_server_version);
        cfrds_str_defer(_client_version);
        cfrds_str_defer(_num2);
        cfrds_str_defer(_num3);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 5)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num1))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_server_version))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_client_version))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num2))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num3))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *num1 = atoi(_num1);

        *server_version = _server_version; _server_version = NULL;
        *client_version = _client_version; _client_version = NULL;

        *num2 = atoi(_num2);
        *num3 = atoi(_num3);
    }

    return ret;
}

cfrds_status cfrds_command_adminapi_debugging_getlogproperty(cfrds_server *server, const char *logdirectory, cfrds_str *result)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.debugging", "getlogproperty", logdirectory, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strlen(xml) > 0)
        {
            WDDX_defer(wddx);
            wddx = wddx_from_xml(xml);

            const WDDX_NODE *data = wddx_data(wddx);
            if (wddx_node_type(data) != WDDX_STRING)
            {
                cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "wddx_node_type(data) != WDDX_STRING");
                return CFRDS_STATUS_RESPONSE_ERROR;
            }

            *result = strdup(wddx_node_string(data));
        }
    }

    return ret;
}

cfrds_status cfrds_command_adminapi_extensions_getcustomtagpaths(cfrds_server *server, cfrds_adminapi_customtagpaths **result)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "getcustomtagpaths", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *result = wddx_from_xml(xml);
        if (!*result)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

void cfrds_adminapi_customtagpaths_free(cfrds_adminapi_customtagpaths *buf)
{
    if (buf)
    {
        wddx_cleanup(buf);
    }
}

void cfrds_adminapi_customtagpaths_cleanup(cfrds_adminapi_customtagpaths **buf)
{
    if (buf)
    {
        cfrds_adminapi_customtagpaths_free(*buf);
        *buf = NULL;
    }
}

int cfrds_adminapi_customtagpaths_count(const cfrds_adminapi_customtagpaths *buf)
{
    if (buf == NULL)
        return 0;

    if (wddx_node_type(buf) != WDDX_ARRAY)
        return 0;

    return wddx_node_array_size(buf);
}

const char *cfrds_adminapi_customtagpaths_at(const cfrds_adminapi_customtagpaths *buf, size_t ndx)
{
    if (buf == NULL)
        return NULL;

    const WDDX_NODE *item = wddx_node_array_at(buf, ndx);

    if (wddx_node_type(item) != WDDX_STRING)
        return NULL;

    return wddx_node_string(item);
}

cfrds_status cfrds_command_adminapi_extensions_setmapping(cfrds_server *server, const char *name, const char *path)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);
    cfrds_buffer_defer(arg);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (!cfrds_buffer_create(&arg))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append(arg, "name:"))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_escaped(arg, name))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append(arg, ";path:"))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_escaped(arg, path))
        return CFRDS_STATUS_MEMORY_ERROR;

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "setmappings", cfrds_buffer_data(arg), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strlen(xml) > 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, xml);
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_adminapi_extensions_deletemapping(cfrds_server *server, const char *mapping)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    /* NOTE: "deleltemappings" (with the extra 'l') is a required typo hardcoded in the Adobe ColdFusion RDS backend. */
    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "deleltemappings", mapping, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strlen(xml) > 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, xml);
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_adminapi_extensions_getmappings(cfrds_server *server, cfrds_adminapi_mappings **result)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "getmappings", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *result = wddx_from_xml(xml);
    }

    return ret;
}

void cfrds_adminapi_mappings_free(cfrds_adminapi_mappings *buf)
{
    if (buf)
    {
        wddx_cleanup(buf);
    }
}

void cfrds_adminapi_mappings_cleanup(cfrds_adminapi_mappings **buf)
{
    if (buf)
    {
        cfrds_adminapi_mappings_free(*buf);
        *buf = NULL;
    }
}

int cfrds_adminapi_mappings_count(const cfrds_adminapi_mappings *buf)
{
    if (buf == NULL)
        return 0;

    if (wddx_node_type(buf) != WDDX_STRUCT)
        return 0;

    return wddx_node_struct_size(buf);
}

const char *cfrds_adminapi_mappings_key(const cfrds_adminapi_mappings *buf, size_t ndx)
{
    const char *ret = NULL;

    if (buf == NULL)
        return NULL;

    if (wddx_node_type(buf) != WDDX_STRUCT)
        return NULL;

    wddx_node_struct_at(buf, ndx, &ret);

    return ret;
}

const char *cfrds_adminapi_mappings_value(const cfrds_adminapi_mappings *buf, size_t ndx)
{
    if (buf == NULL)
        return NULL;

    if (wddx_node_type(buf) != WDDX_STRUCT)
        return NULL;

    const WDDX_NODE *val = wddx_node_struct_at(buf, ndx, NULL);

    if (wddx_node_type(val) != WDDX_STRING)
        return NULL;

    return wddx_node_string(val);
}

cfrds_status cfrds_command_graphing(cfrds_server *server, cfrds_buffer **out_buffer, const char *chart_attributes, size_t num_series, const char **series_data)
{
    cfrds_status ret;

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((chart_attributes == NULL) || (out_buffer == NULL) || (num_series > 0 && series_data == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    char num_series_str[32];
    snprintf(num_series_str, sizeof(num_series_str), "%zu", num_series);

    size_t total_params = 3 + num_series;
    const char **list = malloc((total_params + 1) * sizeof(const char *));
    if (list == NULL)
    {
        server->error_code = -1;
        return CFRDS_STATUS_RESPONSE_ERROR;
    }

    list[0] = "GRAPH";
    list[1] = chart_attributes;
    list[2] = num_series_str;
    for (size_t i = 0; i < num_series; i++)
    {
        list[3 + i] = series_data[i];
    }
    list[total_params] = NULL;

    ret = cfrds_send_command(server, out_buffer, "GRAPHING", list);

    free(list);
    return ret;
}
