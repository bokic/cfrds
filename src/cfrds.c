#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>


void cfrds_server_cleanup(cfrds_server **server)
{
    if (*server == nullptr)
        return;

    cfrds_server_int *_server = *server;

    _server->_errno = 0;
    _server->error_code = 1;

    if (_server->host)
    {
        free(_server->host);
        _server->host = nullptr;
    }

    if (_server->username)
    {
        free(_server->username);
        _server->username = nullptr;
    }

    if (_server->orig_password)
    {
        free(_server->orig_password);
        _server->orig_password = nullptr;
    }

    if (_server->password)
    {
        free(_server->password);
        _server->password = nullptr;
    }

    if (_server->error)
    {
        free(_server->error);
        _server->error = nullptr;
    }

    free(_server);
}

void cfrds_server_clear_error(cfrds_server *server)
{
    cfrds_server_int *_server = server;
    if (_server == nullptr)
        return;

    _server->_errno = 0;
    _server->error_code = 1;

    if (_server->error)
    {
        free(_server->error);
        _server->error = nullptr;
    }
}

static char *cfrds_server_encode_password(const char *password)
{
    char *ret = nullptr;

    if (password == nullptr)
        return nullptr;

    static const char * const hex = "0123456789abcdef";
    static const char * const fillup = "4p0L@r1$";
    static const size_t fillup_len = sizeof(fillup) - 1;

    size_t len = strlen(password);

    ret = malloc((len * 2) + 1);
    if (ret == nullptr)
        return nullptr;

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
    cfrds_server_int *ret = nullptr;

    if ((server == nullptr)||(host == nullptr)||(port == 0)||(username == nullptr)||(password == nullptr))
        return false;

    ret = malloc(sizeof(cfrds_server_int));

    if (ret == nullptr)
        return false;

    ret->host = strdup(host);
    ret->port = port;
    ret->username = strdup(username);
    ret->orig_password = strdup(password);
    ret->password = cfrds_server_encode_password(password);
    ret->_errno = 0;
    ret->error_code = 1;
    ret->error = nullptr;

    *server = ret;

    return true;
}

void cfrds_server_free(cfrds_server *server)
{
    cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return;

    cfrds_server_clear_error(server);

    server_int = server;

    free(server_int->host);
    free(server_int->username);
    free(server_int->orig_password);
    free(server_int->password);

    free(server);
}

void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error)
{
    cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return;

    server_int = server;

    server_int->error_code = error_code;

    if(server_int->error)
        free(server_int->error);

    server_int->error = strdup(error);
}

const char *cfrds_server_get_error(cfrds_server *server)
{
    const cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return nullptr;

    server_int = server;

    return server_int->error;
}

const char *cfrds_server_get_host(cfrds_server *server)
{
    const cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return nullptr;

    server_int = server;

    return server_int->host;
}

uint16_t cfrds_server_get_port(cfrds_server *server)
{
    const cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return 0;

    server_int = server;

    return server_int->port;
}

const char *cfrds_server_get_username(cfrds_server *server)
{
    const cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return 0;

    server_int = server;

    return server_int->username;
}

const char *cfrds_server_get_password(cfrds_server *server)
{
    const cfrds_server_int *server_int = nullptr;

    if (server == nullptr)
        return 0;

    server_int = server;

    return server_int->orig_password;
}

static enum cfrds_status cfrds_internal_command(cfrds_server *server, cfrds_buffer **response, const char *command, const char *list[])
{
    enum cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_server_int *server_int = nullptr;

    cfrds_buffer_defer(post);
    size_t total_cnt = 0;
    size_t list_cnt = 0;

    if (server == nullptr)
        return CFRDS_STATUS_SERVER_IS_nullptr;

    server_int = server;

    server_int->_errno = 0;

    for(int c = 0; ; c++)
    {
        if (list[c] == nullptr)
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

    ret = cfrds_http_post(server, command, post, response);

    return ret;
}

enum cfrds_status cfrds_command_browse_dir(cfrds_server *server, const char *path, cfrds_browse_dir **out)
{
    enum cfrds_status ret;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(path == nullptr)||(out == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "BROWSEDIR", (const char *[]){ path, "", nullptr});
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

    if ((server == nullptr)||(pathname == nullptr)||(out == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "FILEIO", (const char *[]){ pathname, "READ", "", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = cfrds_buffer_to_file_content(response);
    }

    return ret;
}

enum cfrds_status cfrds_command_file_write(cfrds_server *server, const char *pathname, const void *data, size_t length)
{
    enum cfrds_status ret = CFRDS_STATUS_OK;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(post);
    size_t total_cnt = 0;

    if ((server == nullptr)||(pathname == nullptr)||(data == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    server_int = server;

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

    ret = cfrds_http_post(server, "FILEIO", post, nullptr);

    return ret;
}

enum cfrds_status cfrds_command_file_rename(cfrds_server *server, const char *current_pathname, const char *new_pathname)
{
    if ((server == nullptr)||(current_pathname == nullptr)||(new_pathname == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    return cfrds_internal_command(server, nullptr, "FILEIO", (const char *[]){ current_pathname, "RENAME", "", new_pathname, nullptr});
}

enum cfrds_status cfrds_command_file_remove_file(cfrds_server *server, const char *pathname)
{
    if ((server == nullptr)||(pathname == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    return cfrds_internal_command(server, nullptr, "FILEIO", (const char *[]){ pathname, "REMOVE", "", "F", nullptr});
}

enum cfrds_status cfrds_command_file_remove_dir(cfrds_server *server, const char *path)
{
    if ((server == nullptr)||(path == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    return cfrds_internal_command(server, nullptr, "FILEIO", (const char *[]){ path, "REMOVE", "", "D", nullptr});
}

enum cfrds_status cfrds_command_file_exists(cfrds_server *server, const char *pathname, bool *out)
{
    enum cfrds_status ret;

    if ((pathname == nullptr)||(out == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    if (server == nullptr)
    {
        return CFRDS_STATUS_SERVER_IS_nullptr;
    }

    static const char response_file_not_found_start[] = "The system cannot find the path specified: ";

    cfrds_server_int *server_int = nullptr;

    ret = cfrds_internal_command(server, nullptr, "FILEIO", (const char *[]){ pathname, "EXISTENCE", "", "", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *out = true;
    } else {
        server_int = server;
        if ((server_int->error_code == -1)&&(server_int->error)&&(strncmp(server_int->error, response_file_not_found_start, strlen(response_file_not_found_start)) == 0))
        {
            server_int->error_code = 1;
            free(server_int->error);
            server_int->error = nullptr;

            *out = false;

            ret = CFRDS_STATUS_OK;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_file_create_dir(cfrds_server *server, const char *path)
{
    if ((server == nullptr)||(path == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    return cfrds_internal_command(server, nullptr, "FILEIO", (const char *[]){ path, "CREATE", "", "", nullptr});
}

enum cfrds_status cfrds_command_file_get_root_dir(cfrds_server *server, char **out)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(out == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "FILEIO", (const char *[]){ "", "CF_DIRECTORY", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);
        cfrds_buffer_append_char(response, '\0');

        server_int = server;

        if (cfrds_buffer_skip_httpheader(&response_data, &response_size))
        {
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
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_dsninfo(cfrds_server *server, cfrds_sql_dsninfo **dsninfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(dsninfo == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ "", "DSNINFO", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *dsninfo = cfrds_buffer_to_sql_dsninfo(response);
        if (*dsninfo == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_tableinfo(cfrds_server *server, const char *connection_name, cfrds_sql_tableinfo **tableinfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(tableinfo == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "TABLEINFO", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *tableinfo = cfrds_buffer_to_sql_tableinfo(response);
        if (*tableinfo == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_columninfo(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_columninfo **columninfo)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(columninfo == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "COLUMNINFO", table_name, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *columninfo = cfrds_buffer_to_sql_columninfo(response);
        if (*columninfo == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_primarykeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_primarykeys **primarykeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(table_name == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "PRIMARYKEYS", table_name, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *primarykeys = cfrds_buffer_to_sql_primarykeys(response);
        if (*primarykeys == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_foreignkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_foreignkeys **foreignkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(table_name == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "FOREIGNKEYS", table_name, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *foreignkeys = cfrds_buffer_to_sql_foreignkeys(response);
        if (*foreignkeys == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_importedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_importedkeys **importedkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(table_name == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "IMPORTEDKEYS", table_name, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *importedkeys = cfrds_buffer_to_sql_importedkeys(response);
        if (*importedkeys == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_exportedkeys(cfrds_server *server, const char *connection_name, const char *table_name, cfrds_sql_exportedkeys **exportedkeys)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(table_name == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "EXPORTEDKEYS", table_name, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *exportedkeys = cfrds_buffer_to_sql_exportedkeys(response);
        if (*exportedkeys == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_sqlstmnt(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_resultset **resultset)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if (server == nullptr)
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "SQLSTMNT", sql, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *resultset = cfrds_buffer_to_sql_sqlstmnt(response);
        if (*resultset == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_sqlmetadata(cfrds_server *server, const char *connection_name, const char *sql, cfrds_sql_metadata **metadata)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(metadata == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "SQLMETADATA", sql, nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *metadata = cfrds_buffer_to_sql_metadata(response);
        if (*metadata == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_getsupportedcommands(cfrds_server *server, cfrds_sql_supportedcommands **supportedcommands)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(supportedcommands == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ "", "SUPPORTEDCOMMANDS", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *supportedcommands = cfrds_buffer_to_sql_supportedcommands(response);
        if (*supportedcommands == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

enum cfrds_status cfrds_command_sql_dbdescription(cfrds_server *server, const char *connection_name, char **description)
{
    enum cfrds_status ret;

    cfrds_server_int *server_int = nullptr;
    cfrds_buffer_defer(response);

    if ((server == nullptr)||(description == nullptr))
    {
        return CFRDS_STATUS_PARAM_IS_nullptr;
    }

    ret = cfrds_internal_command(server, &response, "DBFUNCS", (const char *[]){ connection_name, "DBDESCRIPTION", nullptr});
    if (ret == CFRDS_STATUS_OK)
    {
        *description = cfrds_buffer_to_sql_dbdescription(response);
        if (*description == nullptr)
        {
            server_int = server;
            server_int->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

void cfrds_buffer_file_content_free(cfrds_file_content *value)
{
    if (value == nullptr)
        return;

    cfrds_file_content_int *_value = (cfrds_file_content_int *)value;

    free(_value->data);
    free(_value->modified);
    free(_value->permission);
    free(_value);
}

const char *cfrds_buffer_file_content_get_data(const cfrds_file_content *value)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->data;
}

int cfrds_buffer_file_content_get_size(const cfrds_file_content *value)
{
    if (value == nullptr)
        return -1;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->size;
}

const char *cfrds_buffer_file_content_get_modified(const cfrds_file_content *value)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->modified;
}

const char *cfrds_buffer_file_content_get_permission(const cfrds_file_content *value)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_file_content_int *_value = (const cfrds_file_content_int *)value;

    return _value->permission;
}

void cfrds_buffer_browse_dir_free(cfrds_browse_dir *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    return _value->cnt;
}

char cfrds_buffer_browse_dir_item_get_kind(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == nullptr)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].kind;
}

const char *cfrds_buffer_browse_dir_item_get_name(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].name;
}

uint8_t cfrds_buffer_browse_dir_item_get_permissions(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == nullptr)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].permissions;
}

size_t cfrds_buffer_browse_dir_item_get_size(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == nullptr)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].size;
}

uint64_t cfrds_buffer_browse_dir_item_get_modified(const cfrds_browse_dir *value, size_t ndx)
{
    if (value == nullptr)
        return 0;

    const cfrds_browse_dir_int *_value = (const cfrds_browse_dir_int *)value;

    if (ndx >= _value->cnt)
        return 0;

    return _value->items[ndx].modified;
}

void cfrds_buffer_sql_dsninfo_free(cfrds_sql_dsninfo *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_dsninfo_int *_value = (const cfrds_sql_dsninfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_dsninfo_item_get_name(const cfrds_sql_dsninfo *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_dsninfo_int *_value = (const cfrds_sql_dsninfo_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->names[ndx];
}

void cfrds_buffer_sql_tableinfo_free(cfrds_sql_tableinfo *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_tableinfo_get_unknown(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].unknown;
}

const char *cfrds_buffer_sql_tableinfo_get_schema(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].schema;
}

const char *cfrds_buffer_sql_tableinfo_get_name(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].name;
}

const char *cfrds_buffer_sql_tableinfo_get_type(const cfrds_sql_tableinfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_tableinfo_int *_value = (const cfrds_sql_tableinfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].type;
}

void cfrds_buffer_sql_columninfo_free(cfrds_sql_columninfo *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_columninfo_get_schema(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].schema;
}

const char *cfrds_buffer_sql_columninfo_get_owner(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].owner;
}

const char *cfrds_buffer_sql_columninfo_get_table(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].table;
}

const char *cfrds_buffer_sql_columninfo_get_name(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].name;
}

int cfrds_buffer_sql_columninfo_get_type(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].type;
}

const char *cfrds_buffer_sql_columninfo_get_typeStr(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return nullptr;

    return _value->items[column].typeStr;
}

int cfrds_buffer_sql_columninfo_get_precision(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].percision;
}

int cfrds_buffer_sql_columninfo_get_length(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].length;
}

int cfrds_buffer_sql_columninfo_get_scale(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].scale;
}

int cfrds_buffer_sql_columninfo_get_radix(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].radix;
}

int cfrds_buffer_sql_columninfo_get_nullable(const cfrds_sql_columninfo *value, size_t column)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_columninfo_int *_value = (const cfrds_sql_columninfo_int *)value;

    if (column >= _value->cnt)
        return -1;

    return _value->items[column].nullable;
}

void cfrds_buffer_sql_primarykeys_free(cfrds_sql_primarykeys *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_primarykeys_get_catalog(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].tableCatalog;
}

const char *cfrds_buffer_sql_primarykeys_get_owner(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].tableOwner;
}

const char *cfrds_buffer_sql_primarykeys_get_table(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].tableName;
}

const char *cfrds_buffer_sql_primarykeys_get_column(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].colName;
}

int cfrds_buffer_sql_primarykeys_get_key_sequence(const cfrds_sql_primarykeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_primarykeys_int *_value = (const cfrds_sql_primarykeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

void cfrds_buffer_sql_foreignkeys_free(cfrds_sql_foreignkeys *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkowner(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_foreignkeys_get_pktable(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_foreignkeys_get_pkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkcatalog(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkowner(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_foreignkeys_get_fktable(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_foreignkeys_get_fkcolumn(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_foreignkeys_get_key_sequence(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_foreignkeys_get_updaterule(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_foreignkeys_get_deleterule(const cfrds_sql_foreignkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_foreignkeys_int *_value = (const cfrds_sql_foreignkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_importedkeys_free(cfrds_sql_importedkeys *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_importedkeys_get_pkcatalog(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_importedkeys_get_pkowner(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_importedkeys_get_pktable(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_importedkeys_get_pkcolumn(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_importedkeys_get_fkcatalog(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_importedkeys_get_fkowner(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_importedkeys_get_fktable(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_importedkeys_get_fkcolumn(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_importedkeys_get_key_sequence(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_importedkeys_get_updaterule(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_importedkeys_get_deleterule(const cfrds_sql_importedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_importedkeys_int *_value = (const cfrds_sql_importedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_exportedkeys_free(cfrds_sql_exportedkeys *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return 0;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableCatalog;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkowner(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableOwner;
}

const char *cfrds_buffer_sql_exportedkeys_get_pktable(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkTableName;
}

const char *cfrds_buffer_sql_exportedkeys_get_pkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].pkColName;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkcatalog(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableCatalog;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkowner(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableOwner;
}

const char *cfrds_buffer_sql_exportedkeys_get_fktable(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkTableName;
}

const char *cfrds_buffer_sql_exportedkeys_get_fkcolumn(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].fkColName;
}

int cfrds_buffer_sql_exportedkeys_get_key_sequence(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].keySequence;
}

int cfrds_buffer_sql_exportedkeys_get_updaterule(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].updateRule;
}

int cfrds_buffer_sql_exportedkeys_get_deleterule(const cfrds_sql_exportedkeys *value, size_t ndx)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_exportedkeys_int *_value = (const cfrds_sql_exportedkeys_int *)value;

    if (ndx >= _value->cnt)
        return -1;

    return _value->items[ndx].deleteRule;
}

void cfrds_buffer_sql_resultset_free(cfrds_sql_resultset *value)
{
    if (value == nullptr)
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
    if (value == nullptr)
        return;

    cfrds_sql_metadata_int *_value = (cfrds_sql_metadata_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        if (_value->items[c].name)  {free(_value->items[c].name); _value->items[c].name = nullptr; }
        if (_value->items[c].type)  {free(_value->items[c].type); _value->items[c].type = nullptr; }
        if (_value->items[c].jtype) {free(_value->items[c].jtype); _value->items[c].jtype = nullptr; }
    }

    free(_value);
}

size_t cfrds_buffer_sql_metadata_count(const cfrds_sql_metadata *value)
{
    if (value == nullptr)
        return 0;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_metadata_get_name(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].name;
}

const char *cfrds_buffer_sql_metadata_get_type(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].type;
}

const char *cfrds_buffer_sql_metadata_get_jtype(const cfrds_sql_metadata *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_metadata_int *_value = (const cfrds_sql_metadata_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->items[ndx].jtype;
}

size_t cfrds_buffer_sql_resultset_columns(cfrds_sql_resultset *value)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_resultset_int *_value = (cfrds_sql_resultset_int *)value;

    return _value->columns;
}

size_t cfrds_buffer_sql_resultset_rows(cfrds_sql_resultset *value)
{
    if (value == nullptr)
        return -1;

    const cfrds_sql_resultset_int *_value = (cfrds_sql_resultset_int *)value;

    return _value->rows;
}

const char *cfrds_buffer_sql_resultset_column_name(cfrds_sql_resultset *value, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_resultset_int *_value = (cfrds_sql_resultset_int *)value;

    if (column >= _value->columns)
        return nullptr;

    return _value->values[column];
}

const char *cfrds_buffer_sql_resultset_value(cfrds_sql_resultset *value, size_t row, size_t column)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_resultset_int *_value = (cfrds_sql_resultset_int *)value;

    if ((row >= _value->rows)||(column >= _value->columns))
        return nullptr;

    return _value->values[(row + 1) * _value->columns + column];
}

void cfrds_buffer_sql_supportedcommands_free(cfrds_sql_supportedcommands *value)
{
    if (value == nullptr)
        return;

    cfrds_sql_supportedcommands_int *_value = (cfrds_sql_supportedcommands_int *)value;

    for(size_t c = 0; c < _value->cnt; c++)
    {
        free(_value->commands[c]);
        _value->commands[c] = nullptr;
    }

    free(_value);
}

size_t cfrds_buffer_sql_supportedcommands_count(const cfrds_sql_supportedcommands *value)
{
    if (value == nullptr)
        return 0;

    const cfrds_sql_supportedcommands_int *_value = (const cfrds_sql_supportedcommands_int *)value;

    return _value->cnt;
}

const char *cfrds_buffer_sql_supportedcommands_get(const cfrds_sql_supportedcommands *value, size_t ndx)
{
    if (value == nullptr)
        return nullptr;

    const cfrds_sql_supportedcommands_int *_value = (const cfrds_sql_supportedcommands_int *)value;

    if (ndx >= _value->cnt)
        return nullptr;

    return _value->commands[ndx];
}
