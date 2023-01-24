#include <cfrds_buffer.h>
#include <cfrds_http.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#if defined(_MSC_VER)
    //  Microsoft
    #define EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
    //  GCC
    #define EXPORT __attribute__((visibility("default")))
#else
    //  do nothing and hope for the best?
    #define EXPORT
#endif

typedef struct {
    char *host;
    uint16_t port;
    char *username;
    char *password;

    cfrds_buffer *buffer;
    uint32_t offsets_len;
    uint32_t *offsets;
    char *error;
} cfrds_server_int;

static void cfrds_server_int_clean(cfrds_server_int *server)
{
    if (server == NULL)
        return;

    cfrds_buffer_free(server->buffer);
    free(server->offsets);
    free(server->error);

    server->buffer = NULL;
    server->offsets_len = 0;
    server->offsets = NULL;
    server->error = NULL;
}

static char *cfrds_server_encode_password(const char *password)
{
    char *ret = NULL;

    static const char * const hex = "0123456789abcdef";
    static const char * const fillup = "4p0L@r1$";
    static const size_t fillup_len = sizeof(fillup) - 1;

    size_t len = strlen(password);

    ret = malloc((len * 2) + 1);

    for(size_t c = 0; c < len; c++)
    {
        char encoded_ch = password[c] ^ fillup[c % fillup_len];
        ret[(c * 2) + 0] = hex[(encoded_ch & 0xf0) >> 4];
        ret[(c * 2) + 1] = hex[(encoded_ch & 0x0f) >> 0];
    }

    ret[len * 2] = '\0';

    return ret;
}

EXPORT bool cfrds_server_init(cfrds_server **server, const char *host, uint16_t port, const char *username, const char *password)
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

    ret->buffer = NULL;
    ret->offsets = 0;
    ret->error = NULL;

    *server = ret;

    return true;
}

EXPORT void cfrds_server_free(cfrds_server *server)
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

EXPORT const char *cfrds_server_get_error(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = server;

    return server_int->error;
}

EXPORT const char *cfrds_server_get_host(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = server;

    return server_int->host;
}

EXPORT uint16_t cfrds_server_get_port(cfrds_server *server)
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

    cfrds_server_int_clean(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, list_cnt + 1);

    for(int c = 0; c < list_cnt; c++)
    {
        cfrds_buffer_append_rds_string(post, list[c]);
    }

    cfrds_buffer_append_rds_string(post, server_int->password);

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

EXPORT enum cfrds_status cfrds_browse_dir(cfrds_server *server, void *path, cfrds_buffer_browse_dir **out)
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

EXPORT enum cfrds_status cfrds_read_file(cfrds_server *server, void *pathname, cfrds_buffer_file_content **out)
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

EXPORT enum cfrds_status cfrds_write_file(cfrds_server *server, void *pathname, void *data, size_t length)
{
    cfrds_server_int *server_int = NULL;
    cfrds_buffer *post = NULL;

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    server_int = server;

    cfrds_server_int_clean(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, 5);
    cfrds_buffer_append_rds_string(post, pathname);
    cfrds_buffer_append_rds_string(post, "READ");
    cfrds_buffer_append_rds_bytes(post, data, length);
    cfrds_buffer_append_rds_string(post, "");

    cfrds_buffer_append_rds_string(post, server_int->password);

    server_int->buffer = cfrds_http_post(server, "FILEIO", post);

    cfrds_buffer_free(post);

    if (server_int->buffer == NULL)
        return CFRDS_STATUS_COMMAND_FAILED;

    return CFRDS_STATUS_OK;
}

EXPORT enum cfrds_status cfrds_rename(cfrds_server *server, char *current_name, char *new_name)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ current_name, "RENAME", "", new_name, NULL});

    return ret;
}

EXPORT enum cfrds_status cfrds_remove_file(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ name, "REMOVE", "", "F", NULL});

    return ret;
}

EXPORT enum cfrds_status cfrds_remove_dir(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ name, "REMOVE", "", "D", NULL});

    return ret;
}

EXPORT enum cfrds_status cfrds_exists(cfrds_server *server, char *pathname)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ pathname, "EXISTENCE", "", "", NULL});

    return ret;
}

EXPORT enum cfrds_status cfrds_create_dir(cfrds_server *server, char *name)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ name, "CREATE", "", "", NULL});

    return ret;
}

EXPORT enum cfrds_status cfrds_get_root_dir(cfrds_server *server)
{
    enum cfrds_status ret;
    cfrds_buffer **buffer = NULL;

    ret = cfrds_internal_command(server, buffer, "FILEIO", (char *[]){ "", "CF_DIRECTORY", NULL});

    return ret;
}
