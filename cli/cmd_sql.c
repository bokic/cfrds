#include "cmd_common.h"

int handle_cmd_sql(cfrds_server *server, const char *command, const char *path, int argc, char *argv[])
{
    (void)argc;
    cfrds_status res;

    if (strcmp(command, "dsninfo") == 0) {
        cfrds_sql_dsninfo_defer(dsninfo);
        res = cfrds_command_sql_dsninfo(server, &dsninfo);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dsninfo FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            struct json_object *arr = json_object_new_array();
            size_t cnt = cfrds_sql_dsninfo_count(dsninfo);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *item = cfrds_sql_dsninfo_item_get_name(dsninfo, c);
                json_object_array_add(arr, json_object_new_string(item ? item : ""));
            }
            json_object_object_add(obj, "dsns", arr);
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            size_t cnt = cfrds_sql_dsninfo_count(dsninfo);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *item = cfrds_sql_dsninfo_item_get_name(dsninfo, c);
                puts(item);
            }
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "tableinfo") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_tableinfo_defer(tableinfo);
            const char *dsn = path + 1;

            res = cfrds_command_sql_tableinfo(server, dsn, &tableinfo);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "tableinfo FAILED with error");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *arr = json_object_new_array();
                size_t cnt = cfrds_sql_tableinfo_count(tableinfo);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name = cfrds_sql_tableinfo_get_column_name(tableinfo, c);
                    const char *type = cfrds_sql_tableinfo_get_column_type(tableinfo, c);
                    struct json_object *t_obj = json_object_new_object();
                    json_object_object_add(t_obj, "name", json_object_new_string(name ? name : ""));
                    json_object_object_add(t_obj, "type", json_object_new_string(type ? type : ""));
                    json_object_array_add(arr, t_obj);
                }
                json_object_object_add(obj, "tables", arr);
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                size_t cnt = cfrds_sql_tableinfo_count(tableinfo);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name = cfrds_sql_tableinfo_get_column_name(tableinfo, c);
                    const char *type = cfrds_sql_tableinfo_get_column_type(tableinfo, c);
                    printf("%s, %s\n", name, type);
                }
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "columninfo") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            const char *schema_separator = path + 1;
            const char *table_separator = strchr(schema_separator, '/');
            if(table_separator) {
                cfrds_sql_columninfo_defer(columninfo);
                size_t tmp_size = 0;
                cfrds_str_defer(schema);

                tmp_size = (size_t)(table_separator - schema_separator);
                schema = malloc(tmp_size + 1);
                if (schema == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                memcpy(schema, schema_separator, tmp_size);
                schema[tmp_size] = '\0';

                const char *table = table_separator + 1;

                res = cfrds_command_sql_columninfo(server, schema, table, &columninfo);
                if (res != CFRDS_STATUS_OK)
                {
                    HANDLE_SERVER_ERROR(res, "columninfo FAILED with error");
                }

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    struct json_object *arr = json_object_new_array();
                    size_t cnt = cfrds_sql_columninfo_count(columninfo);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *name = cfrds_sql_columninfo_get_name(columninfo, c);
                        const char *type = cfrds_sql_columninfo_get_typeStr(columninfo, c);
                        struct json_object *col_obj = json_object_new_object();
                        json_object_object_add(col_obj, "name", json_object_new_string(name ? name : ""));
                        json_object_object_add(col_obj, "type", json_object_new_string(type ? type : ""));
                        json_object_array_add(arr, col_obj);
                    }
                    json_object_object_add(obj, "columns", arr);
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    size_t cnt = cfrds_sql_columninfo_count(columninfo);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *name = cfrds_sql_columninfo_get_name(columninfo, c);
                        const char *type = cfrds_sql_columninfo_get_typeStr(columninfo, c);
                        printf("%s, %s\n", name, type);
                    }
                }
            } else {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No table name");
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "primarykeys") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_primarykeys_defer(primarykeys);
                cfrds_str_defer(tablename);
                size_t tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = (size_t)(table - schema);
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                res = cfrds_command_sql_primarykeys(server, schema, tablename, &primarykeys);
                if (res != CFRDS_STATUS_OK)
                {
                    HANDLE_SERVER_ERROR(res, "primarykeys FAILED with error");
                }

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    struct json_object *arr = json_object_new_array();
                    size_t cnt = cfrds_sql_primarykeys_count(primarykeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *col_name = cfrds_sql_primarykeys_get_catalog(primarykeys, c);
                        const char *col_owner = cfrds_sql_primarykeys_get_owner(primarykeys, c);
                        const char *col_table = cfrds_sql_primarykeys_get_table(primarykeys, c);
                        const char *col_column = cfrds_sql_primarykeys_get_column(primarykeys, c);
                        int col_key_sequence = cfrds_sql_primarykeys_get_key_sequence(primarykeys, c);

                        struct json_object *key_obj = json_object_new_object();
                        json_object_object_add(key_obj, "name", json_object_new_string(col_name ? col_name : ""));
                        json_object_object_add(key_obj, "owner", json_object_new_string(col_owner ? col_owner : ""));
                        json_object_object_add(key_obj, "table", json_object_new_string(col_table ? col_table : ""));
                        json_object_object_add(key_obj, "column", json_object_new_string(col_column ? col_column : ""));
                        json_object_object_add(key_obj, "key_sequence", json_object_new_int(col_key_sequence));
                        json_object_array_add(arr, key_obj);
                    }
                    json_object_object_add(obj, "keys", arr);
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    size_t cnt = cfrds_sql_primarykeys_count(primarykeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *col_name = cfrds_sql_primarykeys_get_catalog(primarykeys, c);
                        const char *col_owner = cfrds_sql_primarykeys_get_owner(primarykeys, c);
                        const char *col_table = cfrds_sql_primarykeys_get_table(primarykeys, c);
                        const char *col_column = cfrds_sql_primarykeys_get_column(primarykeys, c);
                        int col_key_sequence = cfrds_sql_primarykeys_get_key_sequence(primarykeys, c);

                        printf("name: '%s', owner: '%s', table: '%s', column: '%s', key_sequence: %d\n", col_name, col_owner, col_table, col_column, col_key_sequence);
                    }
                }
            } else {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No table name");
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "foreignkeys") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_foreignkeys_defer(foreignkeys);
                cfrds_str_defer(tablename);
                size_t tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = (size_t)(table - schema);
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                res = cfrds_command_sql_foreignkeys(server, schema, tablename, &foreignkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    HANDLE_SERVER_ERROR(res, "foreignkeys FAILED with error");
                }

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    struct json_object *arr = json_object_new_array();
                    size_t cnt = cfrds_sql_foreignkeys_count(foreignkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_foreignkeys_get_pkcatalog(foreignkeys, c);
                        const char *pk_owner = cfrds_sql_foreignkeys_get_pkowner(foreignkeys, c);
                        const char *pk_table = cfrds_sql_foreignkeys_get_pktable(foreignkeys, c);
                        const char *pk_column = cfrds_sql_foreignkeys_get_pkcolumn(foreignkeys, c);
                        const char *fk_catalog = cfrds_sql_foreignkeys_get_fkcatalog(foreignkeys, c);
                        const char *fk_owner = cfrds_sql_foreignkeys_get_fkowner(foreignkeys, c);
                        const char *fk_table = cfrds_sql_foreignkeys_get_fktable(foreignkeys, c);
                        const char *fk_column = cfrds_sql_foreignkeys_get_fkcolumn(foreignkeys, c);
                        int key_sequence = cfrds_sql_foreignkeys_get_key_sequence(foreignkeys, c);
                        int updaterule = cfrds_sql_foreignkeys_get_updaterule(foreignkeys, c);
                        int deleterule = cfrds_sql_foreignkeys_get_deleterule(foreignkeys, c);

                        struct json_object *key_obj = json_object_new_object();
                        json_object_object_add(key_obj, "pk_catalog", json_object_new_string(pk_catalog ? pk_catalog : ""));
                        json_object_object_add(key_obj, "pk_owner", json_object_new_string(pk_owner ? pk_owner : ""));
                        json_object_object_add(key_obj, "pk_table", json_object_new_string(pk_table ? pk_table : ""));
                        json_object_object_add(key_obj, "pk_column", json_object_new_string(pk_column ? pk_column : ""));
                        json_object_object_add(key_obj, "fk_catalog", json_object_new_string(fk_catalog ? fk_catalog : ""));
                        json_object_object_add(key_obj, "fk_owner", json_object_new_string(fk_owner ? fk_owner : ""));
                        json_object_object_add(key_obj, "fk_table", json_object_new_string(fk_table ? fk_table : ""));
                        json_object_object_add(key_obj, "fk_column", json_object_new_string(fk_column ? fk_column : ""));
                        json_object_object_add(key_obj, "key_sequence", json_object_new_int(key_sequence));
                        json_object_object_add(key_obj, "updaterule", json_object_new_int(updaterule));
                        json_object_object_add(key_obj, "deleterule", json_object_new_int(deleterule));
                        json_object_array_add(arr, key_obj);
                    }
                    json_object_object_add(obj, "keys", arr);
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    size_t cnt = cfrds_sql_foreignkeys_count(foreignkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_foreignkeys_get_pkcatalog(foreignkeys, c);
                        const char *pk_owner = cfrds_sql_foreignkeys_get_pkowner(foreignkeys, c);
                        const char *pk_table = cfrds_sql_foreignkeys_get_pktable(foreignkeys, c);
                        const char *pk_column = cfrds_sql_foreignkeys_get_pkcolumn(foreignkeys, c);
                        const char *fk_catalog = cfrds_sql_foreignkeys_get_fkcatalog(foreignkeys, c);
                        const char *fk_owner = cfrds_sql_foreignkeys_get_fkowner(foreignkeys, c);
                        const char *fk_table = cfrds_sql_foreignkeys_get_fktable(foreignkeys, c);
                        const char *fk_column = cfrds_sql_foreignkeys_get_fkcolumn(foreignkeys, c);
                        int key_sequence = cfrds_sql_foreignkeys_get_key_sequence(foreignkeys, c);
                        int updaterule = cfrds_sql_foreignkeys_get_updaterule(foreignkeys, c);
                        int deleterule = cfrds_sql_foreignkeys_get_deleterule(foreignkeys, c);

                        printf("pk_catalog: '%s', pk_owner: '%s', pk_table: '%s', pk_column: '%s', fk_catalog: '%s', fk_owner: '%s', fk_table: '%s', fk_column: '%s', key_sequence: %d, updaterule: %d, deleterule: %d\n",
                            pk_catalog,
                            pk_owner,
                            pk_table,
                            pk_column,
                            fk_catalog,
                            fk_owner,
                            fk_table,
                            fk_column,
                            key_sequence,
                            updaterule,
                            deleterule);
                    }
                }
            } else {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No table name");
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "importedkeys") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_importedkeys_defer(importedkeys);
                cfrds_str_defer(tablename);
                size_t tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = (size_t)(table - schema);
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                res = cfrds_command_sql_importedkeys(server, schema, tablename, &importedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    HANDLE_SERVER_ERROR(res, "importedkeys FAILED with error");
                }

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    struct json_object *arr = json_object_new_array();
                    size_t cnt = cfrds_sql_importedkeys_count(importedkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_importedkeys_get_pkcatalog(importedkeys, c);
                        const char *pk_owner = cfrds_sql_importedkeys_get_pkowner(importedkeys, c);
                        const char *pk_table = cfrds_sql_importedkeys_get_pktable(importedkeys, c);
                        const char *pk_column = cfrds_sql_importedkeys_get_pkcolumn(importedkeys, c);
                        const char *fk_catalog = cfrds_sql_importedkeys_get_fkcatalog(importedkeys, c);
                        const char *fk_owner = cfrds_sql_importedkeys_get_fkowner(importedkeys, c);
                        const char *fk_table = cfrds_sql_importedkeys_get_fktable(importedkeys, c);
                        const char *fk_column = cfrds_sql_importedkeys_get_fkcolumn(importedkeys, c);
                        int key_sequence = cfrds_sql_importedkeys_get_key_sequence(importedkeys, c);
                        int updaterule = cfrds_sql_importedkeys_get_updaterule(importedkeys, c);
                        int deleterule = cfrds_sql_importedkeys_get_deleterule(importedkeys, c);

                        struct json_object *key_obj = json_object_new_object();
                        json_object_object_add(key_obj, "pk_catalog", json_object_new_string(pk_catalog ? pk_catalog : ""));
                        json_object_object_add(key_obj, "pk_owner", json_object_new_string(pk_owner ? pk_owner : ""));
                        json_object_object_add(key_obj, "pk_table", json_object_new_string(pk_table ? pk_table : ""));
                        json_object_object_add(key_obj, "pk_column", json_object_new_string(pk_column ? pk_column : ""));
                        json_object_object_add(key_obj, "fk_catalog", json_object_new_string(fk_catalog ? fk_catalog : ""));
                        json_object_object_add(key_obj, "fk_owner", json_object_new_string(fk_owner ? fk_owner : ""));
                        json_object_object_add(key_obj, "fk_table", json_object_new_string(fk_table ? fk_table : ""));
                        json_object_object_add(key_obj, "fk_column", json_object_new_string(fk_column ? fk_column : ""));
                        json_object_object_add(key_obj, "key_sequence", json_object_new_int(key_sequence));
                        json_object_object_add(key_obj, "updaterule", json_object_new_int(updaterule));
                        json_object_object_add(key_obj, "deleterule", json_object_new_int(deleterule));
                        json_object_array_add(arr, key_obj);
                    }
                    json_object_object_add(obj, "keys", arr);
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    size_t cnt = cfrds_sql_importedkeys_count(importedkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_importedkeys_get_pkcatalog(importedkeys, c);
                        const char *pk_owner = cfrds_sql_importedkeys_get_pkowner(importedkeys, c);
                        const char *pk_table = cfrds_sql_importedkeys_get_pktable(importedkeys, c);
                        const char *pk_column = cfrds_sql_importedkeys_get_pkcolumn(importedkeys, c);
                        const char *fk_catalog = cfrds_sql_importedkeys_get_fkcatalog(importedkeys, c);
                        const char *fk_owner = cfrds_sql_importedkeys_get_fkowner(importedkeys, c);
                        const char *fk_table = cfrds_sql_importedkeys_get_fktable(importedkeys, c);
                        const char *fk_column = cfrds_sql_importedkeys_get_fkcolumn(importedkeys, c);
                        int key_sequence = cfrds_sql_importedkeys_get_key_sequence(importedkeys, c);
                        int updaterule = cfrds_sql_importedkeys_get_updaterule(importedkeys, c);
                        int deleterule = cfrds_sql_importedkeys_get_deleterule(importedkeys, c);

                        printf("pk_catalog: '%s', pk_owner: '%s', pk_table: '%s', pk_column: '%s', fk_catalog: '%s', fk_owner: '%s', fk_table: '%s', fk_column: '%s', key_sequence: %d, updaterule: %d, deleterule: %d\n",
                            pk_catalog,
                            pk_owner,
                            pk_table,
                            pk_column,
                            fk_catalog,
                            fk_owner,
                            fk_table,
                            fk_column,
                            key_sequence,
                            updaterule,
                            deleterule);
                    }
                }
            } else {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No table name");
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "exportedkeys") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_exportedkeys_defer(exportedkeys);
                cfrds_str_defer(tablename);
                size_t tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = (size_t)(table - schema);
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                {
                    HANDLE_ERROR(CFRDS_STATUS_MEMORY_ERROR, "malloc FAILED!");
                }

                res = cfrds_command_sql_exportedkeys(server, schema, tablename, &exportedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    HANDLE_SERVER_ERROR(res, "exportedkeys FAILED with error");
                }

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    struct json_object *arr = json_object_new_array();
                    size_t cnt = cfrds_sql_exportedkeys_count(exportedkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_exportedkeys_get_pkcatalog(exportedkeys, c);
                        const char *pk_owner = cfrds_sql_exportedkeys_get_pkowner(exportedkeys, c);
                        const char *pk_table = cfrds_sql_exportedkeys_get_pktable(exportedkeys, c);
                        const char *pk_column = cfrds_sql_exportedkeys_get_pkcolumn(exportedkeys, c);
                        const char *fk_catalog = cfrds_sql_exportedkeys_get_fkcatalog(exportedkeys, c);
                        const char *fk_owner = cfrds_sql_exportedkeys_get_fkowner(exportedkeys, c);
                        const char *fk_table = cfrds_sql_exportedkeys_get_fktable(exportedkeys, c);
                        const char *fk_column = cfrds_sql_exportedkeys_get_fkcolumn(exportedkeys, c);
                        int key_sequence = cfrds_sql_exportedkeys_get_key_sequence(exportedkeys, c);
                        int updaterule = cfrds_sql_exportedkeys_get_updaterule(exportedkeys, c);
                        int deleterule = cfrds_sql_exportedkeys_get_deleterule(exportedkeys, c);

                        struct json_object *key_obj = json_object_new_object();
                        json_object_object_add(key_obj, "pk_catalog", json_object_new_string(pk_catalog ? pk_catalog : ""));
                        json_object_object_add(key_obj, "pk_owner", json_object_new_string(pk_owner ? pk_owner : ""));
                        json_object_object_add(key_obj, "pk_table", json_object_new_string(pk_table ? pk_table : ""));
                        json_object_object_add(key_obj, "pk_column", json_object_new_string(pk_column ? pk_column : ""));
                        json_object_object_add(key_obj, "fk_catalog", json_object_new_string(fk_catalog ? fk_catalog : ""));
                        json_object_object_add(key_obj, "fk_owner", json_object_new_string(fk_owner ? fk_owner : ""));
                        json_object_object_add(key_obj, "fk_table", json_object_new_string(fk_table ? fk_table : ""));
                        json_object_object_add(key_obj, "fk_column", json_object_new_string(fk_column ? fk_column : ""));
                        json_object_object_add(key_obj, "key_sequence", json_object_new_int(key_sequence));
                        json_object_object_add(key_obj, "updaterule", json_object_new_int(updaterule));
                        json_object_object_add(key_obj, "deleterule", json_object_new_int(deleterule));
                        json_object_array_add(arr, key_obj);
                    }
                    json_object_object_add(obj, "keys", arr);
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    size_t cnt = cfrds_sql_exportedkeys_count(exportedkeys);
                    for(size_t c = 0; c < cnt; c++)
                    {
                        const char *pk_catalog = cfrds_sql_exportedkeys_get_pkcatalog(exportedkeys, c);
                        const char *pk_owner = cfrds_sql_exportedkeys_get_pkowner(exportedkeys, c);
                        const char *pk_table = cfrds_sql_exportedkeys_get_pktable(exportedkeys, c);
                        const char *pk_column = cfrds_sql_exportedkeys_get_pkcolumn(exportedkeys, c);
                        const char *fk_catalog = cfrds_sql_exportedkeys_get_fkcatalog(exportedkeys, c);
                        const char *fk_owner = cfrds_sql_exportedkeys_get_fkowner(exportedkeys, c);
                        const char *fk_table = cfrds_sql_exportedkeys_get_fktable(exportedkeys, c);
                        const char *fk_column = cfrds_sql_exportedkeys_get_fkcolumn(exportedkeys, c);
                        int key_sequence = cfrds_sql_exportedkeys_get_key_sequence(exportedkeys, c);
                        int updaterule = cfrds_sql_exportedkeys_get_updaterule(exportedkeys, c);
                        int deleterule = cfrds_sql_exportedkeys_get_deleterule(exportedkeys, c);

                        printf("pk_catalog: '%s', pk_owner: '%s', pk_table: '%s', pk_column: '%s', fk_catalog: '%s', fk_owner: '%s', fk_table: '%s', fk_column: '%s', key_sequence: %d, updaterule: %d, deleterule: %d\n",
                            pk_catalog,
                            pk_owner,
                            pk_table,
                            pk_column,
                            fk_catalog,
                            fk_owner,
                            fk_table,
                            fk_column,
                            key_sequence,
                            updaterule,
                            deleterule);
                    }
                }
            } else {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No table name");
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "No schema name");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "sql") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_resultset_defer(resultset);
            const char *schema = path + 1;

            const char *sql = argv[3];

            res = cfrds_command_sql_sqlstmnt(server, schema, sql, &resultset);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "sql FAILED with error");
            }

            size_t cols = cfrds_sql_resultset_columns(resultset);
            if (cols == 0)
            {
                HANDLE_ERROR(CFRDS_STATUS_RESPONSE_ERROR, "No columns");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *cols_arr = json_object_new_array();
                for(size_t col = 0; col < cols; col++) {
                    const char *name = cfrds_sql_resultset_column_name(resultset, col);
                    json_object_array_add(cols_arr, json_object_new_string(name ? name : ""));
                }
                json_object_object_add(obj, "columns", cols_arr);

                struct json_object *rows_arr = json_object_new_array();
                size_t rows = cfrds_sql_resultset_rows(resultset);
                for (size_t row = 0; row < rows; row++) {
                    struct json_object *row_arr = json_object_new_array();
                    for(size_t col = 0; col < cols; col++) {
                        const char *value = cfrds_sql_resultset_value(resultset, row, col);
                        json_object_array_add(row_arr, json_object_new_string(value ? value : ""));
                    }
                    json_object_array_add(rows_arr, row_arr);
                }
                json_object_object_add(obj, "rows", rows_arr);

                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                size_t *sizes = malloc(cols * sizeof(size_t));
                if (sizes == NULL)
                {
                    fprintf(stderr, "No memory\n");
                    return EXIT_FAILURE;
                }

                for(size_t c = 0; c < cols; c++)
                {
                    const char *name  = cfrds_sql_resultset_column_name(resultset, c);
                    sizes[c] = strlen(name);
                }

                size_t rows = cfrds_sql_resultset_rows(resultset);
                for (size_t r = 0; r < rows; r++)
                {
                    for (size_t c = 0; c < cols; c++)
                    {
                        const char *value = cfrds_sql_resultset_value(resultset, r, c);
                        if (strlen(value) > sizes[c])
                        {
                            sizes[c] = strlen(value);
                        }
                    }
                }

                printf((const char*)u8"\u250F");
                for(size_t col = 0; col < cols; col++)
                {
                    size_t size = sizes[col];

                    for(size_t c = 0; c < size; c++)
                    {
                        printf((const char*)u8"\u2501");
                    }

                    if (col == cols - 1)
                    {
                        printf((const char*)u8"\u2513");
                    }
                    else
                    {
                        printf((const char*)u8"\u252F");
                    }
                }
                putchar('\n');

                printf((const char*)u8"\u2503");
                for(size_t col = 0; col < cols; col++)
                {
                    size_t size = sizes[col];

                    const char *value = cfrds_sql_resultset_column_name(resultset, col);

                    printf("%s", value);

                    for(size_t c = strlen(value); c < size; c++)
                    {
                        putchar(' ');
                    }

                    if (col == cols - 1)
                    {
                        printf((const char*)u8"\u2503");
                    }
                    else
                    {
                        printf((const char*)u8"\u2502");
                    }
                }
                putchar('\n');

                if (rows == 0)
                {
                    printf((const char*)u8"\u2517");
                    for(size_t col = 0; col < cols; col++)
                    {
                        size_t size = sizes[col];

                        for(size_t c = 0; c < size; c++)
                        {
                            printf((const char*)u8"\u2501");
                        }

                        if (col == cols - 1)
                        {
                            printf((const char*)u8"\u251B");
                        }
                        else
                        {
                            printf((const char*)u8"\u2537");
                        }
                    }
                } else {
                    printf((const char*)u8"\u2520");
                    for(size_t col = 0; col < cols; col++)
                    {
                        size_t size = sizes[col];

                        for(size_t c = 0; c < size; c++)
                        {
                            printf((const char*)u8"\u2500");
                        }

                        if (col == cols - 1)
                        {
                            printf((const char*)u8"\u2528");
                        }
                        else
                        {
                            printf((const char*)u8"\u253C");
                        }
                    }
                }
                putchar('\n');

                for (size_t row = 0; row < rows; row++)
                {
                    printf((const char*)u8"\u2503");
                    for(size_t col = 0; col < cols; col++)
                    {
                        size_t size = sizes[col];

                        const char *value = cfrds_sql_resultset_value(resultset, row, col);

                        printf("%s", value);

                        for(size_t c = strlen(value); c < size; c++)
                        {
                            putchar(' ');
                        }

                        if (col == cols - 1)
                        {
                            printf((const char*)u8"\u2503");
                        }
                        else
                        {
                            printf((const char*)u8"\u2502");
                        }
                    }
                    putchar('\n');

                    printf((const char*)u8"\u2517");
                    for(size_t col = 0; col < cols; col++)
                    {
                        size_t size = sizes[col];

                        for(size_t c = 0; c < size; c++)
                        {
                            printf((const char*)u8"\u2501");
                        }

                        if (col == cols - 1)
                        {
                            printf((const char*)u8"\u251B");
                        }
                        else
                        {
                            printf((const char*)u8"\u2537");
                        }
                    }
                    putchar('\n');
                }

                free(sizes);
            }
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "sqlmetadata") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_metadata_defer(metadata);
            const char *schema = path + 1;

            const char *sql = argv[3];

            res = cfrds_command_sql_sqlmetadata(server, schema, sql, &metadata);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "sqlmetadata FAILED with error");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *arr = json_object_new_array();
                size_t cnt = cfrds_sql_metadata_count(metadata);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name  = cfrds_sql_metadata_get_name(metadata, c);
                    const char *type  = cfrds_sql_metadata_get_type(metadata, c);
                    const char *jtype = cfrds_sql_metadata_get_jtype(metadata, c);

                    struct json_object *m_obj = json_object_new_object();
                    json_object_object_add(m_obj, "name", json_object_new_string(name ? name : ""));
                    json_object_object_add(m_obj, "type", json_object_new_string(type ? type : ""));
                    json_object_object_add(m_obj, "jtype", json_object_new_string(jtype ? jtype : ""));
                    json_object_array_add(arr, m_obj);
                }
                json_object_object_add(obj, "columns", arr);
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                size_t cnt = cfrds_sql_metadata_count(metadata);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name  = cfrds_sql_metadata_get_name(metadata, c);
                    const char *type  = cfrds_sql_metadata_get_type(metadata, c);
                    const char *jtype = cfrds_sql_metadata_get_jtype(metadata, c);

                    printf("name: '%s', type: '%s', jtype: '%s'\n", name, type, jtype);
                }
            }
        }
        return EXIT_SUCCESS;
    } else if ((strcmp(command, "supportedcommands") == 0) || (strcmp(command, "sqlsupportedcommands") == 0)) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_supportedcommands_defer(supportedcommands);

            res = cfrds_command_sql_getsupportedcommands(server, &supportedcommands);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "supportedcommands FAILED with error");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *arr = json_object_new_array();
                size_t cnt = cfrds_sql_supportedcommands_count(supportedcommands);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *supportedcommand = cfrds_sql_supportedcommands_get(supportedcommands, c);
                    json_object_array_add(arr, json_object_new_string(supportedcommand ? supportedcommand : ""));
                }
                json_object_object_add(obj, "commands", arr);
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                size_t cnt = cfrds_sql_supportedcommands_count(supportedcommands);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *supportedcommand = cfrds_sql_supportedcommands_get(supportedcommands, c);
                    printf("%s\n", supportedcommand);
                }
            }
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbdescription") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_str_defer(dbdescription);
            const char *schema = path + 1;

            res = cfrds_command_sql_dbdescription(server, schema, &dbdescription);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "dbdescription FAILED with error");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                json_object_object_add(obj, "description", json_object_new_string(dbdescription ? dbdescription : ""));
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                printf("%s\n", dbdescription);
            }
        }
        return EXIT_SUCCESS;
    }

    return -1;
}
