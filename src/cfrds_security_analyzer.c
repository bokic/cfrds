#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/cfrds_int.h>
#include <cfrds.h>

#include <json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define json_object_defer(var) struct json_object * var __attribute__((cleanup(json_object_cleanup))) = NULL
static void json_object_cleanup(struct json_object **handle)
{
    if (handle)
    {
        json_object_put(*handle);
        *handle = NULL;
    }
}

static struct json_object *parse_sa_json_response(cfrds_server *server, cfrds_buffer *response)
{
    const char *response_data = cfrds_buffer_data(response);
    size_t response_size = cfrds_buffer_data_size(response);
    int64_t rows = 0;

    if (!cfrds_buffer_parse_number(&response_data, &response_size, &rows) || rows != 1)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "rows != 1");
        return NULL;
    }

    cfrds_str_defer(json);
    if (!cfrds_buffer_parse_string(&response_data, &response_size, &json) || json == NULL || response_size != 0)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid json string in response");
        return NULL;
    }

    struct json_object *json_obj = json_tokener_parse(json);
    if (json_obj == NULL)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_obj == NULL");
        return NULL;
    }

    struct json_object *status = NULL;
    json_object_object_get_ex(json_obj, "status", &status);
    if (status == NULL || json_object_get_type(status) != json_type_string)
    {
        cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid status");
        json_object_put(json_obj);
        return NULL;
    }

    if (strcmp(json_object_get_string(status), "success") != 0)
    {
        struct json_object *errormessage = NULL;
        json_object_object_get_ex(json_obj, "errormessage", &errormessage);
        if (errormessage != NULL && json_object_get_type(errormessage) == json_type_string)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, json_object_get_string(errormessage));
        }
        else
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "strcmp(json_object_get_string(status), \"success\") != 0");
        }
        json_object_put(json_obj);
        return NULL;
    }

    return json_obj;
}

cfrds_status cfrds_command_security_analyzer_scan(cfrds_server *server, const char *pathnames, bool recursively, int cores, int *command_id)
{
    cfrds_status ret;
    char cores_str[32];
    const char *recursively_str = recursively ? "true" : "false";

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    snprintf(cores_str, sizeof(cores_str), "%d", cores);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "scan", pathnames, recursively_str, cores_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        json_object_defer(json_obj);
        json_obj = parse_sa_json_response(server, response);
        if (json_obj == NULL)
            return CFRDS_STATUS_RESPONSE_ERROR;

        struct json_object *value = NULL;
        json_object_object_get_ex(json_obj, "id", &value);
        if (value == NULL || json_object_get_type(value) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid or missing id");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *command_id = json_object_get_int(value);
    }

    return ret;
}

cfrds_status cfrds_command_security_analyzer_cancel(cfrds_server *server, int command_id)
{
    cfrds_status ret;
    char id_str[32];
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "cancel", id_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        json_object_defer(json_obj);
        json_obj = parse_sa_json_response(server, response);
        if (json_obj == NULL)
            return CFRDS_STATUS_RESPONSE_ERROR;
    }

    return ret;
}

cfrds_status cfrds_command_security_analyzer_status(cfrds_server *server, int command_id, int *totalfiles, int *filesvisitedcount, int *percentage, int64_t *lastupdated)
{
    cfrds_status ret;
    char id_str[32];
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "status", id_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        json_object_defer(json_obj);
        json_obj = parse_sa_json_response(server, response);
        if (json_obj == NULL)
            return CFRDS_STATUS_RESPONSE_ERROR;

        struct json_object *totalfiles_obj = NULL;
        json_object_object_get_ex(json_obj, "totalfiles", &totalfiles_obj);
        if (totalfiles_obj == NULL || json_object_get_type(totalfiles_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid totalfiles");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
        *totalfiles = json_object_get_int(totalfiles_obj);

        struct json_object *filesvisitedcount_obj = NULL;
        json_object_object_get_ex(json_obj, "filesvisitedcount", &filesvisitedcount_obj);
        if (filesvisitedcount_obj == NULL || json_object_get_type(filesvisitedcount_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid filesvisitedcount");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
        *filesvisitedcount = json_object_get_int(filesvisitedcount_obj);

        struct json_object *percentage_obj = NULL;
        json_object_object_get_ex(json_obj, "percentage", &percentage_obj);
        if (percentage_obj == NULL || json_object_get_type(percentage_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid percentage");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
        *percentage = json_object_get_int(percentage_obj);

        struct json_object *lastupdated_obj = NULL;
        json_object_object_get_ex(json_obj, "lastupdated", &lastupdated_obj);
        if (lastupdated_obj == NULL || json_object_get_type(lastupdated_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "invalid lastupdated");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
        *lastupdated = json_object_get_int64(lastupdated_obj);
    }

    return ret;
}

cfrds_status cfrds_command_security_analyzer_result(cfrds_server *server, int command_id, cfrds_security_analyzer_result **result)
{
    cfrds_status ret;

    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "result", id_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);
        cfrds_str_defer(json_str);
        int64_t items = 0;

        if (!cfrds_buffer_parse_number(&response_data, &response_size, &items))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (items != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &json_str))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *json_obj = json_tokener_parse(json_str);
        if (json_obj == NULL)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *result = (cfrds_security_analyzer_result *)json_obj;
    }

    return ret;
}

cfrds_status cfrds_command_security_analyzer_clean(cfrds_server *server, int command_id)
{
    cfrds_status ret;

    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "clean", id_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);
        int64_t rows = 0;

        cfrds_buffer_parse_number(&response_data, &response_size, &rows);
        if (rows != 1)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "rows != 1");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        cfrds_str_defer(json);
        cfrds_buffer_parse_string(&response_data, &response_size, &json);
        if (json == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}
