#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/cfrds_int.h>
#include <cfrds.h>

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

cfrds_status cfrds_execute_sql_cmd(cfrds_server *server, const char *params[], cfrds_sql_parser_fn parser, void **out_result)
{
    cfrds_buffer_defer(response);

    cfrds_status ret = cfrds_send_command(server, &response, "DBFUNCS", params);
    if (ret == CFRDS_STATUS_OK)
    {
        void *res = parser(response);
        if (res == NULL)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
        *out_result = res;
    }

    return ret;
}

cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo)
{
    if ((server == NULL) || (dsninfo == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ "", "DSNINFO", NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_dsninfo, (void **)dsninfo);
}

cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo)
{
    if ((server == NULL) || (connection_name == NULL) || (tableinfo == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "TABLEINFO", NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_tableinfo, (void **)tableinfo);
}

cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo)
{
    if ((server == NULL) || (columninfo == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "COLUMNINFO", table_name, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_columninfo, (void **)columninfo);
}

cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys)
{
    if ((server == NULL) || (table_name == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "PRIMARYKEYS", table_name, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_primarykeys, (void **)primarykeys);
}

cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_foreignkeys **foreignkeys)
{
    if ((server == NULL) || (table_name == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "FOREIGNKEYS", table_name, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_foreignkeys, (void **)foreignkeys);
}

cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_importedkeys **importedkeys)
{
    if ((server == NULL) || (table_name == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "IMPORTEDKEYS", table_name, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_importedkeys, (void **)importedkeys);
}

cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_exportedkeys **exportedkeys)
{
    if ((server == NULL) || (table_name == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "EXPORTEDKEYS", table_name, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_exportedkeys, (void **)exportedkeys);
}

cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_resultset **resultset)
{
    if ((server == NULL) || (resultset == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "SQLSTMNT", sql, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_sqlstmnt, (void **)resultset);
}

cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_metadata **metadata)
{
    if ((server == NULL) || (metadata == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "SQLMETADATA", sql, NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_metadata, (void **)metadata);
}

cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands)
{
    if ((server == NULL) || (supportedcommands == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ "", "SUPPORTEDCOMMANDS", NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_supportedcommands, (void **)supportedcommands);
}

cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, cfrds_str *description)
{
    if ((server == NULL) || (description == NULL))
        return CFRDS_STATUS_PARAM_IS_NULL;

    return cfrds_execute_sql_cmd(server, (const char *[]){ connection_name, "DBDESCRIPTION", NULL }, (cfrds_sql_parser_fn)cfrds_buffer_to_sql_dbdescription, (void **)description);
}
