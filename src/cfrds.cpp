#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <cfrds.h>
#include <wddx.h>

#include <json.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


#if defined(__APPLE__) || defined(_WIN32)
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif

#define json_object_defer(var) struct json_object * var __attribute__((cleanup(json_object_cleanup))) = NULL
static void json_object_cleanup(struct json_object **handle)
{
    if (handle)
    {
        json_object_put(*handle);
        *handle = NULL;
    }
}

void cfrds_server_cleanup(cfrds_server **server)
{
    if (*server == NULL)
        return;

    cfrds_server_int *server_int = (cfrds_server_int*)*server;

    server_int->_errno = 0;
    server_int->error_code = 1;

    if (server_int->host)
    {
        free(server_int->host);
        server_int->host = NULL;
    }

    if (server_int->username)
    {
        free(server_int->username);
        server_int->username = NULL;
    }

    if (server_int->orig_password)
    {
        free(server_int->orig_password);
        server_int->orig_password = NULL;
    }

    if (server_int->password)
    {
        free(server_int->password);
        server_int->password = NULL;
    }

    if (server_int->error)
    {
        free(server_int->error);
        server_int->error = NULL;
    }

    free(server_int);
}

void cfrds_server_clear_error(cfrds_server *server)
{
    cfrds_server_int *server_int = (cfrds_server_int*)server;
    if (server_int == NULL)
        return;

    server_int->_errno = 0;
    server_int->error_code = 1;

    if (server_int->error)
    {
        free(server_int->error);
        server_int->error = NULL;
    }
}

void cfrds_server_shutdown_socket(cfrds_server *server)
{
    cfrds_server_int *server_int = (cfrds_server_int*)server;
    if (server_int == NULL)
        return;

    cfrds_sock_shutdown(server_int->socket);
}

static char *cfrds_server_encode_password(const char *password)
{
    char *ret = NULL;

    if (password == NULL)
        return NULL;

    const char hex[] = "0123456789abcdef";
    const char fillup[] = "4p0L@r1$";
    size_t fillup_len = strlen(fillup);

    size_t len = strlen(password);

    ret = (char*)malloc((len * 2) + 1);
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

    ret = malloc(sizeof(cfrds_server_int));
    if (ret == NULL)
        return false;

    explicit_bzero(ret, sizeof(cfrds_server_int));

    ((cfrds_server_int *)ret)->host = strdup(host);
    if (((cfrds_server_int *)ret)->host == NULL)
        return false;

    ((cfrds_server_int *)ret)->port = port;

    ((cfrds_server_int *)ret)->username = strdup(username);
    if (((cfrds_server_int *)ret)->username == NULL)
        return false;

    ((cfrds_server_int *)ret)->orig_password = strdup(password);
    if (((cfrds_server_int *)ret)->orig_password == NULL)
        return false;

    if (strlen(((cfrds_server_int *)ret)->orig_password) > 0)
    {
        ((cfrds_server_int *)ret)->password = cfrds_server_encode_password(password);
        if (((cfrds_server_int *)ret)->password == NULL)
            return false;
    }

    ((cfrds_server_int *)ret)->_errno = 0;
    ((cfrds_server_int *)ret)->error_code = 1;
    ((cfrds_server_int *)ret)->error = NULL;

    *server = ret;
    ret = NULL;

    return true;
}

void cfrds_server_free(cfrds_server *server)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return;

    cfrds_server_clear_error(server);

    server_int = (cfrds_server_int*)server;

    if (server_int->host) free(server_int->host);
    if (server_int->username) free(server_int->username);
    if (server_int->orig_password) free(server_int->orig_password);
    if (server_int->password) free(server_int->password);

    free(server);
}

void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error)
{
    cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return;

    server_int = (cfrds_server_int*)server;

    server_int->error_code = error_code;

    if(server_int->error)
        free(server_int->error);

    server_int->error = strdup(error);
}

const char *cfrds_server_get_error(const cfrds_server *server)
{
    const cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = (cfrds_server_int*)server;

    return server_int->error;
}

const char *cfrds_server_get_host(const cfrds_server *server)
{
    const cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return NULL;

    server_int = (cfrds_server_int*)server;

    return server_int->host;
}

uint16_t cfrds_server_get_port(const cfrds_server *server)
{
    const cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return 0;

    server_int = (cfrds_server_int*)server;

    return server_int->port;
}

const char *cfrds_server_get_username(const cfrds_server *server)
{
    const cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return 0;

    server_int = (cfrds_server_int*)server;

    return server_int->username;
}

const char *cfrds_server_get_password(const cfrds_server *server)
{
    const cfrds_server_int *server_int = NULL;

    if (server == NULL)
        return 0;

    server_int = (cfrds_server_int*)server;

    return server_int->orig_password;
}

static enum cfrds_status cfrds_send_command(cfrds_server *server, cfrds_buffer **response, const char *command, const char *list[])
{
    enum cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_server_int *server_int = NULL;

    cfrds_buffer_defer(post);
    size_t total_cnt = 0;
    size_t list_cnt = 0;

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    server_int = (cfrds_server_int*)server;

    server_int->_errno = 0;

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

    cfrds_server_clear_error(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, total_cnt);

    for(size_t c = 0; c < list_cnt; c++)
    {
        cfrds_buffer_append_rds_string(post, list[c]);
    }

    if (server_int->username) cfrds_buffer_append_rds_string(post, server_int->username);
    if (server_int->password) cfrds_buffer_append_rds_string(post, server_int->password);

    ret = cfrds_http_post((cfrds_server_int*)server_int, command, post, response);

    return ret;
}

enum cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out)
{
    enum cfrds_status ret;
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

enum cfrds_status cfrds_command_file_read(cfrds_server *server, const char *pathname, cfrds_file_content **out)
{
    enum cfrds_status ret;
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

enum cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length)
{
    enum cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(post);
    size_t total_cnt = 0;

    if ((server == NULL)||(pathname == NULL)||((data == NULL)&&(length > 0)))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

    server_int->_errno = 0;

    total_cnt = 4;

    if (server_int->username) total_cnt++;
    if (server_int->password) total_cnt++;

    cfrds_server_clear_error(server_int);

    cfrds_buffer_create(&post);
    cfrds_buffer_append_rds_count(post, total_cnt);
    cfrds_buffer_append_rds_string(post, pathname);
    cfrds_buffer_append_rds_string(post, "WRITE");
    cfrds_buffer_append_rds_string(post, "");
    cfrds_buffer_append_rds_bytes(post, data, length);

    if (server_int->username) cfrds_buffer_append_rds_string(post, server_int->username);
    if (server_int->password) cfrds_buffer_append_rds_string(post, server_int->password);

    ret = cfrds_http_post((cfrds_server_int*)server_int, "FILEIO", post, NULL);

    return ret;
}

enum cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname)
{
    if ((server == NULL)||(current_pathname == NULL)||(new_pathname == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ current_pathname, "RENAME", "", new_pathname, NULL});
}

enum cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname)
{
    if ((server == NULL)||(pathname == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ pathname, "REMOVE", "", "F", NULL});
}

enum cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path)
{
    if ((server == NULL)||(path == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ path, "REMOVE", "", "D", NULL});
}

enum cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out)
{
    enum cfrds_status ret;

    if ((pathname == NULL)||(out == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    static const char response_file_not_found_start[] = "The system cannot find the path specified: ";

    cfrds_server_int *server_int = NULL;

    ret = cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ pathname, "EXISTENCE", "", "", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = true;
    } else {
        server_int = (cfrds_server_int*)server;
        if ((server_int->error_code == -1)&&(server_int->error)&&(strncmp(server_int->error, response_file_not_found_start, strlen(response_file_not_found_start)) == 0))
        {
            server_int->error_code = 1;
            free(server_int->error);
            server_int->error = NULL;

            *out = false;

            ret = CFRDS_STATUS_OK;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path)
{
    if ((server == NULL)||(path == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    return cfrds_send_command(server, NULL, "FILEIO", (const char *[]){ path, "CREATE", "", "", NULL});
}

enum cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, char **out)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
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

        server_int = (cfrds_server_int*)server;

        if (!cfrds_buffer_parse_number(&response_data, &response_size, &server_int->error_code))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, out))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(dsninfo == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ "", "DSNINFO", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *dsninfo = cfrds_buffer_to_sql_dsninfo(response);
        if (*dsninfo == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(tableinfo == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "TABLEINFO", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *tableinfo = cfrds_buffer_to_sql_tableinfo(response);
        if (*tableinfo == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(columninfo == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "COLUMNINFO", table_name, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *columninfo = cfrds_buffer_to_sql_columninfo(response);
        if (*columninfo == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(table_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "PRIMARYKEYS", table_name, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *primarykeys = cfrds_buffer_to_sql_primarykeys(response);
        if (*primarykeys == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_foreignkeys **foreignkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(table_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "FOREIGNKEYS", table_name, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *foreignkeys = cfrds_buffer_to_sql_foreignkeys(response);
        if (*foreignkeys == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_importedkeys **importedkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(table_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "IMPORTEDKEYS", table_name, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *importedkeys = cfrds_buffer_to_sql_importedkeys(response);
        if (*importedkeys == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_exportedkeys **exportedkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(table_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "EXPORTEDKEYS", table_name, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *exportedkeys = cfrds_buffer_to_sql_exportedkeys(response);
        if (*exportedkeys == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_resultset **resultset)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(resultset == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "SQLSTMNT", sql, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *resultset = cfrds_buffer_to_sql_sqlstmnt(response);
        if (*resultset == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_metadata **metadata)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(metadata == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "SQLMETADATA", sql, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *metadata = cfrds_buffer_to_sql_metadata(response);
        if (*metadata == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(supportedcommands == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ "", "SUPPORTEDCOMMANDS", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *supportedcommands = cfrds_buffer_to_sql_supportedcommands(response);
        if (*supportedcommands == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, char **description)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if ((server == NULL)||(description == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "DBDESCRIPTION", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *description = cfrds_buffer_to_sql_dbdescription(response);
        if (*description == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

void cfrds_buffer_file_content_free(cfrds_file_content *value)
{
    if (value == NULL)
        return;

    cfrds_file_content_int *_value = (cfrds_file_content_int *)value;

    free(_value->data);
    free(_value->modified);
    free(_value->permission);
    free(_value);
}

const char *cfrds_buffer_file_content_get_data(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->data;
}

int cfrds_buffer_file_content_get_size(const cfrds_file_content *value)
{
    if (value == NULL)
        return -1;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->size;
}

const char *cfrds_buffer_file_content_get_modified(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->modified;
}

const char *cfrds_buffer_file_content_get_permission(const cfrds_file_content *value)
{
    if (value == NULL)
        return NULL;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->permission;
}

void cfrds_buffer_browse_dir_free(cfrds_browse_dir *value)
{
    if (value == NULL)
        return;

    cfrds_browse_dir_int *_value = (cfrds_browse_dir_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        free(_value->items[c].name);
    }

    free(_value);
}

size_t cfrds_buffer_browse_dir_count(const cfrds_browse_dir *value)
{
    if (value == NULL)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    return _value->cnt;
}

char cfrds_buffer_browse_dir_item_get_kind(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == NULL)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].kind;
}

const char *cfrds_buffer_browse_dir_item_get_name(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].name;
}

uint8_t cfrds_buffer_browse_dir_item_get_permissions(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == NULL)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].permissions;
}

size_t cfrds_buffer_browse_dir_item_get_size(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == NULL)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].size;
}

uint64_t cfrds_buffer_browse_dir_item_get_modified(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == NULL)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].modified;
}

void cfrds_buffer_sql_dsninfo_free(cfrds_sql_dsninfo *value)
{
    if (value == NULL)
        return;

    cfrds_sql_dsninfo_int *_value = (cfrds_sql_dsninfo_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        free(_value->names[c]);
    }

    free(_value);
}

size_t cfrds_buffer_sql_dsninfo_count(const cfrds_sql_dsninfo *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_dsninfo_int *_value = (const cfrds_sql_dsninfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_dsninfo_int *_value = (const cfrds_sql_dsninfo_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->names[ndx];
}

void cfrds_buffer_sql_tableinfo_free(cfrds_sql_tableinfo *value)
{
    if (value == NULL)
        return;

    cfrds_sql_tableinfo_int *_value = (cfrds_sql_tableinfo_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].unknown) free(_value->items[c].unknown);
        if(_value->items[c].schema) free(_value->items[c].schema);
        if(_value->items[c].name) free(_value->items[c].name);
        if(_value->items[c].type) free(_value->items[c].type);
    }

    free(_value);
}

size_t cfrds_buffer_sql_tableinfo_count(const cfrds_sql_tableinfo *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_tableinfo_get_unknown(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].unknown;
}

const char *cfrds_buffer_sql_tableinfo_get_schema(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].schema;
}

const char *cfrds_buffer_sql_tableinfo_get_name(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].name;
}

const char *cfrds_buffer_sql_tableinfo_get_type(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].type;
}

void cfrds_buffer_sql_columninfo_free(cfrds_sql_columninfo *value)
{
    if (value == NULL)
        return;

    cfrds_sql_columninfo_int *_value = (cfrds_sql_columninfo_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].schema) free(_value->items[c].schema);
        if(_value->items[c].owner) free(_value->items[c].owner);
        if(_value->items[c].table) free(_value->items[c].table);
        if(_value->items[c].name) free(_value->items[c].name);
        if(_value->items[c].typeStr) free(_value->items[c].typeStr);
    }

    free(_value);
}

size_t cfrds_buffer_sql_columninfo_count(const cfrds_sql_columninfo *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_columninfo_get_schema(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].schema;
}

const char *cfrds_buffer_sql_columninfo_get_owner(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].owner;
}

const char *cfrds_buffer_sql_columninfo_get_table(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].table;
}

const char *cfrds_buffer_sql_columninfo_get_name(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].name;
}

int cfrds_buffer_sql_columninfo_get_type(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].type;
}

const char *cfrds_buffer_sql_columninfo_get_typeStr(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return NULL;

    return _value->items[column].typeStr;
}

int cfrds_buffer_sql_columninfo_get_precision(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].percision;
}

int cfrds_buffer_sql_columninfo_get_length(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].length;
}

int cfrds_buffer_sql_columninfo_get_scale(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].scale;
}

int cfrds_buffer_sql_columninfo_get_radix(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].radix;
}

int cfrds_buffer_sql_columninfo_get_nullable(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].nullable;
}

void cfrds_buffer_sql_primarykeys_free(cfrds_sql_primarykeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_primarykeys_int *_value = (cfrds_sql_primarykeys_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].tableCatalog) free(_value->items[c].tableCatalog);
        if(_value->items[c].tableOwner) free(_value->items[c].tableOwner);
        if(_value->items[c].tableName) free(_value->items[c].tableName);
        if(_value->items[c].colName) free(_value->items[c].colName);
    }

    free(_value);
}

size_t cfrds_buffer_sql_primarykeys_count(const cfrds_sql_primarykeys *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_primarykeys_get_catalog(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].tableCatalog;
}

const char *cfrds_buffer_sql_primarykeys_get_owner(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].tableOwner;
}

const char *cfrds_buffer_sql_primarykeys_get_table(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].tableName;
}

const char *cfrds_buffer_sql_primarykeys_get_column(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].colName;
}

int cfrds_buffer_sql_primarykeys_get_key_sequence(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

void cfrds_buffer_sql_foreignkeys_free(cfrds_sql_foreignkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_foreignkeys_int *_value = (cfrds_sql_foreignkeys_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].pkTableCatalog) free(_value->items[c].pkTableCatalog);
        if(_value->items[c].pkTableOwner) free(_value->items[c].pkTableOwner);
        if(_value->items[c].pkTableName) free(_value->items[c].pkTableName);
        if(_value->items[c].pkColName) free(_value->items[c].pkColName);
        if(_value->items[c].fkTableCatalog) free(_value->items[c].fkTableCatalog);
        if(_value->items[c].fkTableOwner) free(_value->items[c].fkTableOwner);
        if(_value->items[c].fkTableName) free(_value->items[c].fkTableName);
        if(_value->items[c].fkColName) free(_value->items[c].fkColName);
    }

    free(_value);
}

size_t cfrds_buffer_sql_foreignkeys_count(const cfrds_sql_foreignkeys *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkowner(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_foreignkeys_get_pktable(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkowner(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_foreignkeys_get_fktable(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_foreignkeys_get_key_sequence(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_foreignkeys_get_updaterule(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_foreignkeys_get_deleterule(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_importedkeys_free(cfrds_sql_importedkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_importedkeys_int *_value = (cfrds_sql_importedkeys_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].pkTableCatalog) free(_value->items[c].pkTableCatalog);
        if(_value->items[c].pkTableOwner) free(_value->items[c].pkTableOwner);
        if(_value->items[c].pkTableName) free(_value->items[c].pkTableName);
        if(_value->items[c].pkColName) free(_value->items[c].pkColName);
        if(_value->items[c].fkTableCatalog) free(_value->items[c].fkTableCatalog);
        if(_value->items[c].fkTableOwner) free(_value->items[c].fkTableOwner);
        if(_value->items[c].fkTableName) free(_value->items[c].fkTableName);
        if(_value->items[c].fkColName) free(_value->items[c].fkColName);
    }

    free(_value);
}

size_t cfrds_buffer_sql_importedkeys_count(const cfrds_sql_importedkeys *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_importedkeys_get_pkcatalog(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_importedkeys_get_pkowner(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_importedkeys_get_pktable(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_importedkeys_get_pkcolumn(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_importedkeys_get_fkcatalog(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_importedkeys_get_fkowner(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_importedkeys_get_fktable(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_importedkeys_get_fkcolumn(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_importedkeys_get_key_sequence(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_importedkeys_get_updaterule(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_importedkeys_get_deleterule(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_exportedkeys_free(cfrds_sql_exportedkeys *value)
{
    if (value == NULL)
        return;

    cfrds_sql_exportedkeys_int *_value = (cfrds_sql_exportedkeys_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if(_value->items[c].pkTableCatalog) free(_value->items[c].pkTableCatalog);
        if(_value->items[c].pkTableOwner) free(_value->items[c].pkTableOwner);
        if(_value->items[c].pkTableName) free(_value->items[c].pkTableName);
        if(_value->items[c].pkColName) free(_value->items[c].pkColName);
        if(_value->items[c].fkTableCatalog) free(_value->items[c].fkTableCatalog);
        if(_value->items[c].fkTableOwner) free(_value->items[c].fkTableOwner);
        if(_value->items[c].fkTableName) free(_value->items[c].fkTableName);
        if(_value->items[c].fkColName) free(_value->items[c].fkColName);
    }

    free(_value);
}

size_t cfrds_buffer_sql_exportedkeys_count(const cfrds_sql_exportedkeys *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkowner(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_exportedkeys_get_pktable(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkowner(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_exportedkeys_get_fktable(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_exportedkeys_get_key_sequence(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_exportedkeys_get_updaterule(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_exportedkeys_get_deleterule(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_resultset_free(cfrds_sql_resultset *value)
{
    if (value == NULL)
        return;

    cfrds_sql_resultset_int *_value = (cfrds_sql_resultset_int *)value;

    for(size_t c = 0; c < (_value->rows + 1) * _value->columns; c++)
    {
        if (_value->values[c])
            free(_value->values[c]);
    }

    free(_value);
}

void cfrds_buffer_sql_metadata_free(cfrds_sql_metadata *value)
{
    if (value == NULL)
        return;

    cfrds_sql_metadata_int *_value = (cfrds_sql_metadata_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if (_value->items[c].name)  {free(_value->items[c].name); _value->items[c].name = NULL; }
        if (_value->items[c].type)  {free(_value->items[c].type); _value->items[c].type = NULL; }
        if (_value->items[c].jtype) {free(_value->items[c].jtype); _value->items[c].jtype = NULL; }
    }

    free(_value);
}

size_t cfrds_buffer_sql_metadata_count(const cfrds_sql_metadata *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_metadata_get_name(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].name;
}

const char *cfrds_buffer_sql_metadata_get_type(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].type;
}

const char *cfrds_buffer_sql_metadata_get_jtype(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->items[ndx].jtype;
}

size_t cfrds_buffer_sql_resultset_columns(const cfrds_sql_resultset *value)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_resultset_int *_value = (const cfrds_sql_resultset_int *)value;

    return _value->columns;
}

size_t cfrds_buffer_sql_resultset_rows(const cfrds_sql_resultset *value)
{
    if (value == NULL)
        return -1;

    const cfrds_sql_resultset_int *_value = (const cfrds_sql_resultset_int *)value;

    return _value->rows;
}

const char *cfrds_buffer_sql_resultset_column_name(const cfrds_sql_resultset *value, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_resultset_int *_value = (const cfrds_sql_resultset_int *)value;

    if (column >= _value->columns)
        return NULL;

    return _value->values[column];
}

const char *cfrds_buffer_sql_resultset_value(const cfrds_sql_resultset *value, size_t row, size_t column)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_resultset_int *_value = (const cfrds_sql_resultset_int *)value;

    if ((row >= _value->rows)||(column >= _value->columns))
        return NULL;

    return _value->values[(row + 1) * _value->columns + column];
}

void cfrds_buffer_sql_supportedcommands_free(cfrds_sql_supportedcommands *value)
{
    if (value == NULL)
        return;

    cfrds_sql_supportedcommands_int *_value = (cfrds_sql_supportedcommands_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        free(_value->commands[c]);
        _value->commands[c] = NULL;
    }

    free(_value);
}

size_t cfrds_buffer_sql_supportedcommands_count(const cfrds_sql_supportedcommands *value)
{
    if (value == NULL)
        return 0;

    const cfrds_sql_supportedcommands_int *_value = (const cfrds_sql_supportedcommands_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_supportedcommands_get(const cfrds_sql_supportedcommands *value, size_t ndx)
{
    if (value == NULL)
        return NULL;

    const cfrds_sql_supportedcommands_int *_value = (const cfrds_sql_supportedcommands_int *)value;

    if (ndx >= _value->cnt)
        return NULL;

    return _value->commands[ndx];
}

enum cfrds_status cfrds_command_debugger_start(cfrds_server *server, char **session_id)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "0,REMOTE_SESSION", true);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_START", wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *session_id = cfrds_buffer_to_debugger_start(response);
        if (*session_id == NULL)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_stop(cfrds_server *server, const char *session_id)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_STOP", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        if (cfrds_buffer_to_debugger_stop(response) == false)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_get_server_info(cfrds_server *server, const char *session_id, uint16_t *port)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(port == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_GET_DEBUG_SERVER_INFO", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        int val = cfrds_buffer_to_debugger_info(response);
        if (val == -1)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *port = val;
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_breakpoint_on_exception(cfrds_server *server, const char *session_id, bool value)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "0,BREAK_ON_EXCEPTION", value);
    wddx_put_string(wddx, "0,COMMAND", "SESSION_BREAK_ON_EXCEPTION");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_GET_DEBUG_SERVER_INFO", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        int val = cfrds_buffer_to_debugger_info(response);
        if (val == -1)
        {
            server_int = (cfrds_server_int*)server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_breakpoint(cfrds_server *server, const char *session_id, const char *filepath, int line, bool enable)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(filepath == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_number(wddx, "0,Y", line);
    if (enable)
        wddx_put_string(wddx, "0,COMMAND", "SET_BREAKPOINT");
    else
        wddx_put_string(wddx, "0,COMMAND", "UNSET_BREAKPOINT");
    wddx_put_string(wddx, "0,FILE", filepath);
    wddx_put_number(wddx, "0,SEQ", 1.0);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_clear_all_breakpoints(cfrds_server *server, const char *session_id)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "UNSET_ALL_BREAKPOINTS");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_get_debug_events(cfrds_server *server, const char *session_id, cfrds_debugger_event **event)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(event == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_EVENTS", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *event = cfrds_buffer_to_debugger_event(response);
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_all_fetch_flags_enabled(cfrds_server *server, const char *session_id, bool threads, bool watch, bool scopes, bool cf_trace, bool java_trace, cfrds_debugger_event **event)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "THREADS", threads);
    wddx_put_bool(wddx, "WATCH", watch);
    wddx_put_bool(wddx, "SCOPES", scopes);
    wddx_put_bool(wddx, "CF_TRACE", cf_trace);
    wddx_put_bool(wddx, "JAVA_TRACE", java_trace);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_EVENTS", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *event = cfrds_buffer_to_debugger_event(response);
    }

    return ret;
}

enum cfrds_status cfrds_command_debugger_step_in(cfrds_server *server, const char *session_id, const char *thread_name)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "STEP_IN");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_step_over(cfrds_server *server, const char *session_id, const char *thread_name)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "STEP_OVER");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_step_out(cfrds_server *server, const char *session_id, const char *thread_name)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "STEP_OUT");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_continue(cfrds_server *server, const char *session_id, const char *thread_name)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "CONTINUE");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

void cfrds_buffer_debugger_event_free(cfrds_debugger_event *event)
{
    wddx_cleanup(&event);
}

enum cfrds_debugger_type cfrds_buffer_debugger_event_get_type(const cfrds_debugger_event *event)
{
    if (event == NULL)
    {
        return CFRDS_DEBUGGER_EVENT_UNKNOWN;
    }

    const char *event_name = wddx_get_string(event, "0,EVENT");

    if (strcmp(event_name, "CF_BREAKPOINT_SET") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET;
    else if (strcmp(event_name, "BREAKPOINT") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT;
    else if (strcmp(event_name, "STEP") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_STEP;

    return CFRDS_DEBUGGER_EVENT_UNKNOWN;
}

const char *cfrds_buffer_debugger_event_breakpoint_get_source(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,GET_SOURCE");
}

int cfrds_buffer_debugger_event_breakpoint_get_line(const cfrds_debugger_event *event)
{
    return wddx_get_number(event, "0,LINE", NULL);
}

const char *cfrds_buffer_debugger_event_breakpoint_get_thread_name(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,THREAD");
}

const char *cfrds_buffer_debugger_event_breakpoint_set_get_pathname(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,CFML_PATH");
}

int cfrds_buffer_debugger_event_breakpoint_set_get_req_line(const cfrds_debugger_event *event)
{
    return wddx_get_number(event, "0,REQ_LINE_NUM", NULL);
}

int cfrds_buffer_debugger_event_breakpoint_set_get_act_line(const cfrds_debugger_event *event)
{
    return wddx_get_number(event, "0,ACTUAL_LINE_NUM", NULL);
}

const WDDX_NODE *cfrds_buffer_debugger_event_get_scopes(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,SCOPES");
}

const WDDX_NODE *cfrds_buffer_debugger_event_get_threads(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,THREADS");
}

const WDDX_NODE *cfrds_buffer_debugger_event_get_watch(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,WATCH");
}

const WDDX_NODE *cfrds_buffer_debugger_event_get_cf_trace(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,CF_TRACE");
}

const WDDX_NODE *cfrds_buffer_debugger_event_get_java_trace(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,JAVA_TRACE");
}

enum cfrds_status cfrds_command_debugger_watch_expression(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,VARIABLE_NAME", variable);
    wddx_put_string(wddx, "0,COMMAND", "GET_SINGLE_CF_VARIABLE");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_set_variable(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable, const char *value)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,VARIABLE_VALUE", value);
    wddx_put_string(wddx, "0,VARIABLE_NAME", variable);
    wddx_put_string(wddx, "0,COMMAND", "SET_VARIABLE_VALUE");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_watch_variables(cfrds_server *server, const char *session_id, const char *variables)
{
    enum cfrds_status ret;
    char command[32];
    size_t index = 0;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(variables == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "SET_WATCH_VARIABLES");

    while(strlen(variables) > 0)
    {
        cfrds_str_defer(variable);

        const char *delimiter = strchr(variables, ',');
        if (delimiter == NULL)
        {
            variable = strdup(variables);
            if (variable == NULL) return CFRDS_STATUS_MEMORY_ERROR;
            variables += strlen(variables);
        } else {
            size_t len = delimiter - variables;
            if (len == 0)
            {
                variables++;
                continue;
            }
            variable = (char*)malloc(len + 1);
            if (variable == NULL) return CFRDS_STATUS_MEMORY_ERROR;
            memcpy(variable, variables, len);
            variable[len] = '\0';
            variables += len + 1;
        }

        if (strlen(variable) > 0)
        {
            snprintf(command, sizeof(command), "0,WATCH,%zu", index++);
            wddx_put_string(wddx, command, variable);
        }
    }

    if (index == 0)
        return CFRDS_STATUS_INVALID_INPUT_PARAMETER;

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_get_output(cfrds_server *server, const char *session_id, const char *thread_name)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "0,BODY_ONLY", true);
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_debugger_set_scope_filter(cfrds_server *server, const char *session_id, const char *filter)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,FILTER", filter);
    wddx_put_string(wddx, "0,COMMAND", "SET_SCOPE_FILTER");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

enum cfrds_status cfrds_command_security_analyzer_scan(cfrds_server *server, const char *pathnames, bool recursively, int cores, int *command_id)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char cores_str[32];
    char *recursively_str = NULL;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

    if (recursively)
        recursively_str = "true";
    else
        recursively_str = "false";

    snprintf(cores_str, sizeof(cores_str), "%d", cores);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "scan", pathnames, recursively_str, cores_str, NULL});
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

        if (response_size != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "response_size != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        json_object_defer(json_obj);
        json_obj = json_tokener_parse(json);
        if (json_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *status = NULL;
        json_object_object_get_ex(json_obj, "status", &status);
        if (status == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "status == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(status) != json_type_string)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(status) != json_type_string");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strcmp(json_object_get_string(status), "success") != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "strcmp(json_object_get_string(status), \"success\") != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *value = NULL;
        json_object_object_get_ex(json_obj, "id", &value);
        if (value == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "value == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(value) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(value) != json_type_int");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *command_id = json_object_get_int(value);
    }

    return ret;
}

enum cfrds_status cfrds_command_security_analyzer_cancel(cfrds_server *server, int command_id)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "cancel", id_str, NULL});
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

        if (response_size != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "response_size != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        json_object_defer(json_obj);
        json_obj = json_tokener_parse(json);
        if (json_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *status = NULL;
        json_object_object_get_ex(json_obj, "status", &status);
        if (status == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "status == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(status) != json_type_string)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(status) != json_type_string");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strcmp(json_object_get_string(status), "success") != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "strcmp(json_object_get_string(status), \"success\") != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_security_analyzer_status(cfrds_server *server, int command_id, int *totalfiles, int *filesvisitedcount, int *percentage, int64_t *lastupdated)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "status", id_str, NULL});
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

        if (response_size != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "response_size != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        json_object_defer(json_obj);
        json_obj = json_tokener_parse(json);
        if (json_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *status = NULL;
        json_object_object_get_ex(json_obj, "status", &status);
        if (status == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "status == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(status) != json_type_string)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(status) != json_type_string");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strcmp(json_object_get_string(status), "success") != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "strcmp(json_object_get_string(status), \"success\") != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *totalfiles_obj = NULL;
        json_object_object_get_ex(json_obj, "totalfiles", &totalfiles_obj);
        if (totalfiles_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "totalfiles_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(totalfiles_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(totalfiles_obj) != json_type_int");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *totalfiles = json_object_get_int(totalfiles_obj);

        struct json_object *filesvisitedcount_obj = NULL;
        json_object_object_get_ex(json_obj, "filesvisitedcount", &filesvisitedcount_obj);
        if (filesvisitedcount_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "filesvisitedcount_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(filesvisitedcount_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(filesvisitedcount_obj) != json_type_int");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *filesvisitedcount = json_object_get_int(filesvisitedcount_obj);

        struct json_object *percentage_obj = NULL;
        json_object_object_get_ex(json_obj, "percentage", &percentage_obj);
        if (percentage_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "percentage_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(percentage_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(percentage_obj) != json_type_int");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *percentage = json_object_get_int(percentage_obj);

        struct json_object *lastupdated_obj = NULL;
        json_object_object_get_ex(json_obj, "lastupdated", &lastupdated_obj);
        if (lastupdated_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "lastupdated_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(lastupdated_obj) != json_type_int)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(lastupdated_obj) != json_type_int");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *lastupdated = json_object_get_int64(lastupdated_obj);
    }

    return ret;
}

enum cfrds_status cfrds_command_security_analyzer_result(cfrds_server *server, int command_id, cfrds_security_analyzer_result **result)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

    snprintf(id_str, sizeof(id_str), "%d", command_id);

    ret = cfrds_send_command(server, &response, "SECURITYANALYZER", (const char *[]){ "result", id_str, NULL});
    if (ret == CFRDS_STATUS_OK)
    {

    }

    return ret;
}

enum cfrds_status cfrds_command_security_analyzer_clean(cfrds_server *server, int command_id)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char id_str[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

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

        if (response_size != 0)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "response_size != 0");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        json_object_defer(json_obj);
        json_obj = json_tokener_parse(json);
        if (json_obj == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_obj == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        struct json_object *status = NULL;
        json_object_object_get_ex(json_obj, "status", &status);
        if (status == NULL)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "status == NULL");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (json_object_get_type(status) != json_type_string)
        {
            cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "json_object_get_type(status) != json_type_string");
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strcmp(json_object_get_string(status), "success") != 0)
        {
            struct json_object *errormessage = NULL;
            json_object_object_get_ex(json_obj, "errormessage", &errormessage);
            if ((errormessage == NULL)||(json_object_get_type(errormessage) != json_type_string))
            {
                cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, "strcmp(json_object_get_string(status), \"success\") != 0");
            }
            else
            {
                cfrds_server_set_error(server, CFRDS_STATUS_RESPONSE_ERROR, json_object_get_string(errormessage));
            }
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_ide_default(cfrds_server *server, int version, int *num1, char **server_version, char **client_version, int *num2, int *num3)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = NULL;
    char param[32];

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    server_int = (cfrds_server_int*)server;

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
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 5)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num1))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_server_version))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_client_version))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num2))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &_num3))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server_int->error_code = -1;
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

enum cfrds_status cfrds_command_adminapi_debugging_getlogproperty(cfrds_server *server, const char *logdirectory, char **result)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.debugging", "getlogproperty", logdirectory, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_server_int *server_int = NULL;

        cfrds_str_defer(xml);

        server_int = (cfrds_server_int*)server;

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server_int->error_code = -1;
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

enum cfrds_status cfrds_command_adminapi_extensions_getcustomtagpaths(cfrds_server *server, WDDX **result)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "getcustomtagpaths", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_server_int *server_int = NULL;

        cfrds_str_defer(xml);

        server_int = (cfrds_server_int*)server;

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *result = wddx_from_xml(xml);
    }

    return ret;
}

enum cfrds_status cfrds_command_adminapi_extensions_setmapping(cfrds_server *server, const char *name, const char *path)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);
    cfrds_buffer_defer(arg);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    cfrds_buffer_create(&arg);
    cfrds_buffer_append(arg, "name:");
    cfrds_buffer_append(arg, name);
    cfrds_buffer_append(arg, ";path:");
    cfrds_buffer_append(arg, path);

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "setmappings", cfrds_buffer_data(arg), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_server_int *server_int = NULL;

        cfrds_str_defer(xml);

        server_int = (cfrds_server_int*)server;

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server_int->error_code = -1;
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

enum cfrds_status cfrds_command_adminapi_extensions_deletemappings(cfrds_server *server, const char *mapping)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "deleltemappings", mapping, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_server_int *server_int = NULL;

        cfrds_str_defer(xml);

        server_int = (cfrds_server_int*)server;

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server_int->error_code = -1;
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

enum cfrds_status cfrds_command_adminapi_extensions_getmappings(cfrds_server *server, WDDX **result)
{
    enum cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "ADMINAPI", (const char *[]){ "cfide.adminapi.extensions", "getmappings", NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_server_int *server_int = NULL;

        cfrds_str_defer(xml);

        server_int = (cfrds_server_int*)server;

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (response_size != 0)
        {
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *result = wddx_from_xml(xml);
    }

    return ret;
}
