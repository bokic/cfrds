#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


typedef struct {
    char *host;
    uint16_t port;
    char *username;
    char *password;

    char *error;
} cfrds_server_int;

static void cfrds_server_int_clean(cfrds_server_int *server)
{
    if (server == NULL)
        return;

    if (server->error)
    {
        free(server->error);
        server->error = NULL;
    }
}

static char *cfrds_server_encode_password(const char *password)
{
    char *ret = NULL;

    if (password == NULL)
        return NULL;

    static const char * const hex = "0123456789abcdef";
    static const char * const fillup = "4p0L@r1$";
    static const size_t fillup_len = sizeof(fillup) - 1;

    size_t len = strlen(password);

    ret = malloc((len * 2) + 1);
    if (ret == NULL)
        return NULL;

    for (size_t c = 0; c < len; c++)
    {
        char encoded_ch = password[c] ^ fillup[c % fillup_len];
        ret[(c * 2) + 0] = hex[(encoded_ch & 0xf0) >> 4];
        ret[(c * 2) + 1] = hex[(encoded_ch & 0x0f) >> 0];
    }

    ret[len * 2] = '\0';

    return ret;
}

bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password)
{
    cfrds_server_int *ret = NULL;

    if (server == NULL)
        return false;

    ret = malloc(sizeof(cfrds_server_int));

    if (ret == NULL)
        return false;

    ret->host = strdup(host);
    ret->port = port;
    ret->username = strdup(username);
    ret->password = cfrds_server_encode_password(password);
    ret->error = NULL;

    *server = ret;

    return true;
}

void cfrds_server_free(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return;

    cfrds_server_int_clean(server);

    server_int = server;    

    free(server_int->host);
    free(server_int->username);
    free(server_int->password);

    free(server);
}

void cfrds_server_set_error(cfrds_server *server, const char *error)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return;

    server_int = server;

    if(server_int->error)
        free(server_int->error);

    server_int->error = strdup(error);
}

const char *cfrds_server_get_error(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = server;

    return server_int->error;
}

const char *cfrds_server_get_host(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = server;

    return server_int->host;
}

uint16_t cfrds_server_get_port(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return 0;

    server_int = server;

    return server_int->port;
}

static enum cfrds_status cfrds_internal_command(cfrds_server *server, cfrds_buffer **buffer, const char *command, char *list[])
{
    cfrds_server_int *server_int = NULL;
    const char *response_data = NULL;
    cfrds_buffer *post = NULL;
    size_t total_cnt = 0;
    size_t list_cnt = 0;

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    server_int = server;

    for(int c = 0; ; c++)
    {
        if (list[c] == NULL)
        {
            list_cnt = c;
            break;
        }
    }

    total_cnt = list_cnt;

    if (server_int->username) total_cnt++;
    if (server_int->password) total_cnt++;

    cfrds_server_int_clean(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, total_cnt);

    for(int c = 0; c < list_cnt; c++)
    {
        cfrds_buffer_append_rds_string(post, list[c]);
    }

    if (server_int->username) cfrds_buffer_append_rds_string(post, server_int->username);
    if (server_int->password) cfrds_buffer_append_rds_string(post, server_int->password);

    *buffer = cfrds_http_post(server, command, post);

    cfrds_buffer_free(post);

    if (*buffer == NULL)
        return CFRDS_STATUS_COMMAND_FAILED;

    response_data = cfrds_buffer_data(*buffer);

    static const char *good_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";

    if (strncmp(response_data, good_response, strlen(good_response)) != 0)
        return CFRDS_STATUS_RESPONSE_ERROR;

    return CFRDS_STATUS_OK;
}

enum cfrds_status cfrds_browse_dir(cfrds_server *server, void *path, cfrds_browse_dir_t **out)
{
    enum cfrds_status ret;
    cfrds_buffer *response = NULL;

    if (out == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_internal_command(server, &response, "BROWSEDIR", (char *[]){ path, "", NULL});

    if (ret == CFRDS_STATUS_OK)
    {
        *out = cfrds_buffer_to_browse_dir(response);
    }

    cfrds_buffer_free(response);

    return ret;
}

enum cfrds_status cfrds_read_file(cfrds_server *server, void *pathname, cfrds_file_content_t **out)
{
    enum cfrds_status ret;
    cfrds_buffer *response = NULL;

    if (out == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_internal_command(server, &response, "FILEIO", (char *[]){ pathname, "READ", "", NULL});

    if (ret == CFRDS_STATUS_OK)
    {
        *out = cfrds_buffer_to_file_content(response);
    }

    cfrds_buffer_free(response);

    return ret;
}

enum cfrds_status cfrds_write_file(cfrds_server *server, void *pathname, const void *data, size_t length)
{
    cfrds_server_int *server_int = NULL;
    const char *response_data = NULL;
    cfrds_buffer *response = NULL;
    cfrds_buffer *post = NULL;
    size_t total_cnt = 0;
    size_t list_cnt = 0;

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    server_int = server;

    total_cnt = 4;

    if (server_int->username) total_cnt++;
    if (server_int->password) total_cnt++;

    cfrds_server_int_clean(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, total_cnt);
    cfrds_buffer_append_rds_string(post, pathname);
    cfrds_buffer_append_rds_string(post, "WRITE");
    cfrds_buffer_append_rds_string(post, "");
    cfrds_buffer_append_rds_bytes(post, data, length);

    if (server_int->username) cfrds_buffer_append_rds_string(post, server_int->username);
    if (server_int->password) cfrds_buffer_append_rds_string(post, server_int->password);

    response = cfrds_http_post(server, "FILEIO", post);

    cfrds_buffer_free(post);

    if (response == NULL)
        return CFRDS_STATUS_COMMAND_FAILED;

    response_data = cfrds_buffer_data(response);

    cfrds_buffer_free(response);

    return CFRDS_STATUS_OK;
}

enum cfrds_status cfrds_rename(cfrds_server *server, char *current_name, char *new_name)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ current_name, "RENAME", "", new_name, NULL});

    return ret;
}

enum cfrds_status cfrds_remove_file(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ name, "REMOVE", "", "F", NULL});

    return ret;
}

enum cfrds_status cfrds_remove_dir(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ name, "REMOVE", "", "D", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(buffer);
        size_t response_size = cfrds_buffer_data_size(buffer);
        if (cfrds_buffer_skip_httpheader((char **)&response_data, &response_size))
        {
            if ((response_size != 4)||(strcmp(response_data, "1:0:") != 0))
            {
                ret = CFRDS_STATUS_COMMAND_FAILED;
            }
        }
    }
    return ret;
}

enum cfrds_status cfrds_exists(cfrds_server *server, char *pathname, bool *out)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ pathname, "EXISTENCE", "", "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(buffer);
        size_t response_size = cfrds_buffer_data_size(buffer);
        if (cfrds_buffer_skip_httpheader((char **)&response_data, &response_size))
        {
            if ((response_size == 4)&&(strcmp(response_data, "1:0:") == 0))
                *out = true;
            else
                *out = false;
        }
    }

    return ret;
}

enum cfrds_status cfrds_create_dir(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ name, "CREATE", "", "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(buffer);
        size_t response_size = cfrds_buffer_data_size(buffer);
        if (cfrds_buffer_skip_httpheader((char **)&response_data, &response_size))
        {
            if ((response_size != 4)||(strcmp(response_data, "1:0:") != 0))
            {
                ret = CFRDS_STATUS_COMMAND_FAILED;
            }
        }
    }

    return ret;
}

enum cfrds_status cfrds_get_root_dir(cfrds_server *server)
{
    enum cfrds_status ret;
    cfrds_buffer *buffer = NULL;

    ret = cfrds_internal_command(server, &buffer, "FILEIO", (char *[]){ "", "CF_DIRECTORY", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        const char *response_data = cfrds_buffer_data(buffer);
        size_t response_size = cfrds_buffer_data_size(buffer);
        if (cfrds_buffer_skip_httpheader((char **)&response_data, &response_size))
        {
            if ((response_size != 4)||(strcmp(response_data, "1:0:") != 0))
            {
                ret = CFRDS_STATUS_COMMAND_FAILED;
            }
        }
    }
    return ret;
}

void cfrds_buffer_file_content_free(cfrds_file_content_t *value)
{
    if (value == NULL)
        return;

    free(value->data);
    free(value->modified);
    free(value->permission);
    free(value);
}

void cfrds_buffer_browse_dir_free(cfrds_browse_dir_t *value)
{
    if (value == NULL)
        return;

    for(size_t c = 0; c< value->cnt; c++)
    {
        free(value->items[c].name);
    }

    free(value);
}
