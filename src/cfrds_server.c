#include <internal/explicit_bzero.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/cfrds_int.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

void cfrds_server_cleanup(cfrds_server **server)
{
    if (server && *server) {
        cfrds_server_free(*server);
        *server = NULL;
    }
}

void cfrds_server_clear_error(cfrds_server *server)
{
    if (server == NULL)
        return;

    server->_errno = 0;
    server->error_code = 1;

    if (server->error)
    {
        free(server->error);
        server->error = NULL;
    }
}

char *cfrds_server_encode_password(const char *password)
{
    char *ret = NULL;

    if (password == NULL)
        return NULL;

    const char hex[] = "0123456789abcdef";
    const char fillup[] = "4p0L@r1$";
    size_t fillup_len = strlen(fillup);

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
    cfrds_server_defer(ret);

    if ((server == NULL)||(host == NULL)||(port == 0)||(username == NULL)||(password == NULL))
        return false;

    ret = malloc(sizeof(cfrds_server));
    if (ret == NULL)
        return false;

    explicit_bzero(ret, sizeof(cfrds_server));

    ret->host = strdup(host);
    if (ret->host == NULL)
        return false;

    ret->port = port;

    ret->username = strdup(username);
    if (ret->username == NULL)
        return false;

    ret->orig_password = strdup(password);
    if (ret->orig_password == NULL)
        return false;

    /* Encode password if non-empty; empty passwords leave ret->password as NULL */
    if (strlen(ret->orig_password) > 0)
    {
        ret->password = cfrds_server_encode_password(password);
        if (ret->password == NULL)
            return false;
    }

    ret->_errno = 0;
    ret->error_code = 1;
    ret->error = NULL;

    *server = ret;
    ret = NULL;

    return true;
}

void cfrds_server_free(cfrds_server *server)
{
    if (server == NULL)
        return;

    cfrds_server_clear_error(server);

    free(server->host);
    free(server->username);
    if (server->orig_password) {
        explicit_bzero(server->orig_password, strlen(server->orig_password));
        free(server->orig_password);
    }
    if (server->password) {
        explicit_bzero(server->password, strlen(server->password));
        free(server->password);
    }

    free(server);
}

void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error)
{
    if (server == NULL)
        return;

    server->error_code = error_code;

    free(server->error);

    if (error)
        server->error = strdup(error);
    else
        server->error = NULL;
}

const char *cfrds_server_get_error(const cfrds_server *server)
{
    if (server == NULL)
        return NULL;

    return server->error;
}

const char *cfrds_server_get_host(const cfrds_server *server)
{
    if (server == NULL)
        return NULL;

    return server->host;
}

uint16_t cfrds_server_get_port(const cfrds_server *server)
{
    if (server == NULL)
        return 0;

    return server->port;
}

const char *cfrds_server_get_username(const cfrds_server *server)
{
    if (server == NULL)
        return NULL;

    return server->username;
}

const char *cfrds_server_get_password(const cfrds_server *server)
{
    if (server == NULL)
        return NULL;

    return server->orig_password;
}

cfrds_status cfrds_send_command(cfrds_server *server, cfrds_buffer **response, const char *command, const char *list[])
{
    cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_buffer_defer(post);
    size_t total_cnt = 0;
    size_t list_cnt = 0;

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    server->_errno = 0;

    for(size_t c = 0; c < 1024; c++)
    {
        if (list[c] == NULL)
        {
            list_cnt = c;
            break;
        }
    }

    if (list[list_cnt] != NULL)
        return CFRDS_STATUS_INVALID_INPUT_PARAMETER;

    total_cnt = list_cnt;

    if (server->username && strlen(server->username) > 0) total_cnt++;
    if (server->password && strlen(server->password) > 0) total_cnt++;

    cfrds_server_clear_error(server);

    if (!cfrds_buffer_create(&post))
        return CFRDS_STATUS_MEMORY_ERROR;

    if (!cfrds_buffer_append_rds_count(post, total_cnt))
        return CFRDS_STATUS_MEMORY_ERROR;

    for(size_t c = 0; c < list_cnt; c++)
    {
        if (!cfrds_buffer_append_rds_string(post, list[c]))
            return CFRDS_STATUS_MEMORY_ERROR;
    }

    if (server->username && strlen(server->username) > 0 && !cfrds_buffer_append_rds_string(post, server->username))
        return CFRDS_STATUS_MEMORY_ERROR;

    if (server->password && strlen(server->password) > 0 && !cfrds_buffer_append_rds_string(post, server->password))
        return CFRDS_STATUS_MEMORY_ERROR;

    ret = cfrds_http_post(server, command, post, response);

    return ret;
}
