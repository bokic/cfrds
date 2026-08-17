#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/cfrds_int.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out)
{
    cfrds_status ret;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(path == NULL)||(out == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "BROWSEDIR", (const char *[]){ path, "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = cfrds_buffer_to_browse_dir(response);
    }

    return ret;
}

cfrds_status cfrds_command_file_read(cfrds_server *server, const char *pathname, cfrds_file_content **out)
{
    cfrds_status ret;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(pathname == NULL)||(out == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "FILEIO", (const char *[]){ pathname, "READ", "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = cfrds_buffer_to_file_content(response);
    }

    return ret;
}

cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length)
{
    cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_buffer_defer(post);
    size_t total_cnt = 0;

    if ((server == NULL)||(pathname == NULL)||((data == NULL)&&(length > 0)))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    server->_errno = 0;

    total_cnt = 4;

    if (server->username && strlen(server->username) > 0) total_cnt++;
    if (server->password && strlen(server->password) > 0) total_cnt++;

    cfrds_server_clear_error(server);

    if (!cfrds_buffer_create(&post))
        return CFRDS_STATUS_MEMORY_ERROR;

    if (!cfrds_buffer_append_rds_count(post, total_cnt))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_rds_string(post, pathname))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_rds_string(post, "WRITE"))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_rds_string(post, ""))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (!cfrds_buffer_append_rds_bytes(post, data, length))
        return CFRDS_STATUS_MEMORY_ERROR;

    if (server->username && strlen(server->username) > 0 && !cfrds_buffer_append_rds_string(post, server->username))
        return CFRDS_STATUS_MEMORY_ERROR;
    if (server->password && strlen(server->password) > 0 && !cfrds_buffer_append_rds_string(post, server->password))
        return CFRDS_STATUS_MEMORY_ERROR;

    ret = cfrds_http_post(server, "FILEIO", post, NULL);

    return ret;
}

cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname)
{
    if ((server == NULL)||(current_pathname == NULL)||(new_pathname == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ current_pathname, "RENAME", "", new_pathname, NULL});
}

cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname)
{
    if ((server == NULL)||(pathname == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ pathname, "REMOVE", "", "F", NULL});
}

cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path)
{
    if ((server == NULL)||(path == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ path, "REMOVE", "", "D", NULL});
}

cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out)
{
    cfrds_status ret;

    if ((pathname == NULL)||(out == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    static const char response_file_not_found_start[] = "The system cannot find the path specified: ";

    ret = cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ pathname, "EXISTENCE", "", "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = true;
    } else {
        if ((server->error_code == -1)&&(server->error)&&(strncmp(server->error, response_file_not_found_start, strlen(response_file_not_found_start)) == 0))
        {
            server->error_code = 1;
            free(server->error);
            server->error = NULL;

            *out = false;

            ret = CFRDS_STATUS_OK;
        }
    }

    return ret;
}

cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path)
{
    if ((server == NULL)||(path == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ path, "CREATE", "", "", NULL});
}

cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, cfrds_str *out)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if ((server == NULL)||(out == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "FILEIO", (const char *[]){ "", "CF_DIRECTORY", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, out))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}
