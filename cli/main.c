#include <cfrds.h>
#include "os.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>


#include <json.h>

static bool json_output = false;

static void print_json_error(cfrds_status res, const char *err_msg)
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "status", json_object_new_string("error"));
    json_object_object_add(obj, "error", json_object_new_string(err_msg ? err_msg : ""));
    json_object_object_add(obj, "code", json_object_new_int(res));
    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
    json_object_put(obj);
}

static char *base64_encode(const unsigned char *data, size_t input_length) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;
    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = (i > input_length + 1) ? '=' : encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = (i > input_length) ? '=' : encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    encoded_data[output_length] = '\0';
    return encoded_data;
}

#define HANDLE_ERROR(status_code, format_str, ...) \
    do { \
        char err_buf[1024]; \
        snprintf(err_buf, sizeof(err_buf), format_str, ##__VA_ARGS__); \
        if (json_output) { \
            print_json_error((status_code), err_buf); \
        } else { \
            fprintf(stderr, "%s\n", err_buf); \
        } \
        return EXIT_FAILURE; \
    } while(0)

#define HANDLE_SERVER_ERROR(status_code, prefix) \
    do { \
        const char *srv_err = cfrds_server_get_error(server); \
        HANDLE_ERROR((status_code), "%s: %s", (prefix), srv_err ? srv_err : ""); \
    } while(0)

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

static void usage(void)
{
    printf("Usage: cfrds <command> [options] <url> [options]\n");
    printf("commands:\n");
    printf("  - 'ls', 'dir' - List a server directory.\n");
    printf("         example: `cfrds ls <rds://[username[:password]@]host[:port]/[path]>`\n");
    printf("            or    `cfrds dir <rds://[username[:password]@]host[:port]/[path]>`\n");
    printf("\n");
    printf("  - 'cat' - Print server file content to stdout.\n");
    printf("         example: `cfrds cat <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("\n");
    printf("  - 'get', 'download' - Download a file from server.\n");
    printf("         example: `cfrds get <rds://[username[:password]@]host[:port]</pathname>> <local_pathname>`\n");
    printf("            or    `cfrds download <rds://[username[:password]@]host[:port]</pathname>> <local_pathname>`\n");
    printf("\n");
    printf("  - 'put', 'upload' - Upload a file to server.\n");
    printf("         example: `cfrds put <local_pathname> <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("            or    `cfrds upload <local_pathname> <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("\n");
    printf("  - 'mv', 'move' - Move/rename file or folder.\n");
    printf("         example: `cfrds mv <rds://[username[:password]@]host[:port]</pathname>> <new_pathname>`\n");
    printf("            or    `cfrds move <rds://[username[:password]@]host[:port]</pathname>> <new_pathname>`\n");
    printf("\n");
    printf("  - 'rm', 'delete' - Delete a file from server.\n");
    printf("         example: `cfrds rm <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("            or    `cfrds delete <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("\n");
    printf("  - 'mkdir' - Create a directory on a server.\n");
    printf("         example: `cfrds mkdir <rds://[username[:password]@]host[:port]</path>>`\n");
    printf("\n");
    printf("  - 'rmdir' - Delete a directory from a server.\n");
    printf("         example: `cfrds rmdir <rds://[username[:password]@]host[:port]</path>>`\n");
    printf("\n");
    printf("  - 'cfroot' - Return ColdFusion installation directory.\n");
    printf("         example: `cfrds cfroot <rds://[username[:password]@]host[:port]>`\n");
    printf("\n");
    printf("  - 'dsninfo' - Return ColdFusion data sources.\n");
    printf("         example: `cfrds dsninfo <rds://[username[:password]@]host[:port]>`\n");
    printf("\n");
    printf("  - 'tableinfo' - Return ColdFusion datasource tables info.\n");
    printf("         example: `cfrds tableinfo <rds://[username[:password]@]host[:port]/<dsn_name>>`\n");
    printf("\n");
    printf("  - 'columninfo' - Return ColdFusion datasource table columns info.\n");
    printf("         example: `cfrds columninfo <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n");
    printf("\n");
    printf("  - 'primarykeys' - Return ColdFusion datasource table primary keys info.\n");
    printf("         example: `cfrds primarykeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n");
    printf("\n");
    printf("  - 'foreignkeys' - Return ColdFusion datasource table foreign keys info.\n");
    printf("         example: `cfrds foreignkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n");
    printf("\n");
    printf("  - 'importedkeys' - Return ColdFusion datasource table imported keys info.\n");
    printf("         example: `cfrds importedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n");
    printf("\n");
    printf("  - 'exportedkeys' - Return ColdFusion datasource table exported keys info.\n");
    printf("         example: `cfrds exportedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n");
    printf("\n");
    printf("  - 'sql' - Execute SQL statement on ColdFusion data sources.\n");
    printf("         example: `cfrds sql <rds://[username[:password]@]host[:port]/<dsn_name>> \"<sql_statement>\"`\n");
    printf("\n");
    printf("  - 'sqlmetadata' - Return SQL statement metadata on ColdFusion data sources.\n");
    printf("         example: `cfrds sqlmetadata <rds://[username[:password]@]host[:port]/<dsn_name>> \"<sql_statement>\"`\n");
    printf("\n");
    printf("  - 'supportedcommands', 'sqlsupportedcommands' - Return SQL statement supported commands on ColdFusion data sources.\n");
    printf("         example: `cfrds supportedcommands <rds://[username[:password]@]host[:port]/<dsn_name>>`\n");
    printf("\n");
    printf("  - 'dbdescription' - Return ColdFusion data sources database info.\n");
    printf("         example: `cfrds dbdescription <rds://[username[:password]@]host[:port]/<dsn_name>>`\n");
    printf("\n");
    printf("  - 'security_analyzer' - Run security analyzer on CFML application.\n");
    printf("         example: `cfrds security_analyzer <rds://[username[:password]@]host[:port]</pathname>>`\n");
    printf("\n");
    printf("  - 'ide_default' - Get ColdFusion server information.\n");
    printf("         example: `cfrds ide_default <rds://[username[:password]@]host[:port]> <version>`\n");
    printf("\n");
    printf("  - 'adminapi' - Get/set ColdFusion server adminapi settings.\n");
    printf("         examples:\n");
    printf("           `cfrds adminapi <rds://[username[:password]@]host[:port]> debugging_getlogproperty <log_directory>`\n");
    printf("           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getcustomtagpaths`\n");
    printf("           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_setmapping <mapping_name> <mapping_path>`\n");
    printf("           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_deletemapping <mapping_name>`\n");
    printf("           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getmappings`\n");
    printf("\n");
    printf("  - 'graphing' - Generate ColdFusion server graph/chart.\n");
    printf("         example: `cfrds graphing <rds://[username[:password]@]host[:port]> <chart_attributes> [out_file.png] [series1] ...`\n");
    printf("\n");
    printf("  - 'step_in', 'step_over', 'step_out', 'continue' - Debugger execution control.\n");
    printf("         example: `cfrds step_in <rds://[username[:password]@]host[:port]> <session_id> <thread_name>`\n");
}


static bool init_server_from_uri(const char *uri, char **hostname, uint16_t *port, char **username, char **password, char **path)
{
    cfrds_str_defer(_hostname);
    cfrds_str_defer(_port_str);
    uint16_t _port = 80;
    cfrds_str_defer(_username);
    cfrds_str_defer(_password);
    cfrds_str_defer(_path);

    if (uri == NULL)
        return false;
    if (strstr(uri, "rds://") != uri)
        return false;

    uri += 6;

    size_t uri_strlen = strlen(uri);

    const char *path_start = strchr(uri, '/');
    if (path_start) {
        size_t delta = (size_t)(path_start - uri);
        size_t path_strlen = uri_strlen - delta;
        _path = malloc(path_strlen + 1);
        if (!_path)
            return false;
        memcpy(_path, path_start, path_strlen);
        _path[path_strlen] = '\0';
    } else {
        _path = strdup("/");
        path_start = uri + uri_strlen;
    }

    const char *login_start = strchr(uri, '@');
    if (login_start) {
        const char *pass_start = strchr(uri, ':');
        if ((pass_start != NULL)&&(pass_start < login_start)) {
            size_t user_strlen = (size_t)(pass_start - uri);
            size_t pass_strlen = (size_t)(login_start - pass_start - 1);

            if (user_strlen) {
                _username = malloc(user_strlen + 1);
                if (!_username)
                    return false;
                memcpy(_username, uri, user_strlen);
                _username[user_strlen] = '\0';
            }
            if (pass_strlen) {
                _password = malloc(pass_strlen + 1);
                if (!_password)
                    return false;
                memcpy(_password, pass_start + 1, pass_strlen);
                _password[pass_strlen] = '\0';
            }
        } else {
            size_t user_strlen = (size_t)(login_start - uri);
            _username = malloc(user_strlen + 1);
            if (!_username)
                return false;
            memcpy(_username, uri, user_strlen);
            _username[user_strlen] = '\0';
        }
        uri = login_start + 1;
    }

    const char *port_start = strchr(uri, ':');
    if (port_start) {
        size_t host_strlen = (size_t)(port_start - uri);
        size_t port_strlen = (size_t)(path_start - port_start - 1);

        _hostname = malloc(host_strlen + 1);
        if (!_hostname)
            return false;
        memcpy(_hostname, uri, host_strlen);
        _hostname[host_strlen] = '\0';
        _port_str = malloc(port_strlen + 1);
        if (!_port_str)
            return false;
        memcpy(_port_str, port_start + 1, port_strlen);
        _port_str[port_strlen] = '\0';

        long tmp_port = atol(_port_str);
        if ((tmp_port < 0x0000)||(tmp_port > 0xffff))
            return false;
        _port = (uint16_t)tmp_port;
    } else {
        size_t host_strlen = (size_t)(path_start - uri);
        _hostname = malloc(host_strlen + 1);
        if (!_hostname)
            return false;
        memcpy(_hostname, uri, host_strlen);
        _hostname[host_strlen] = '\0';
        _port = 80;
    }

    *hostname = _hostname; _hostname = NULL;
    *port = _port;
    *username = _username; _username = NULL;
    *password = _password; _password = NULL;
    *path = _path;         _path = NULL;

    return true;
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[argc - 1], "--json") == 0)
    {
        json_output = true;
        argc--;
        argv[argc] = NULL;
    }

    if ((argc == 2)&&((strcmp(argv[1], "-v") == 0)||(strcmp(argv[1], "--version") == 0)))
    {
        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "version", json_object_new_string(CFRDS_VERSION));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("cfrds version: " CFRDS_VERSION "\n");
        }
        return EXIT_SUCCESS;
    }

#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
    }
#endif
    cfrds_server_defer(server);
    cfrds_file_content_defer(content);
    cfrds_status res;

    const char *uri = NULL;
    cfrds_str_defer(hostname);
    uint16_t port = 80;
    cfrds_str_defer(username);
    cfrds_str_defer(password);
    cfrds_str_defer(path);
    cfrds_str_defer(cfroot);

    if (argc < 3) {
        if (json_output) {
            print_json_error(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Usage: cfrds <command> [options] <url> [options]");
        } else {
            usage();
        }
        return EXIT_FAILURE;
    }

    const char *command = argv[1];

    if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        if (argc < 4) {
            if (json_output) {
                print_json_error(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Usage: cfrds put <local_pathname> <rds_url>");
            } else {
                usage();
            }
            return EXIT_FAILURE;
        }
        uri = argv[3];
    } else if ((strcmp(command, "dbg_brk") == 0) ||
               (strcmp(command, "step_in") == 0) ||
               (strcmp(command, "step_over") == 0) ||
               (strcmp(command, "step_out") == 0) ||
               (strcmp(command, "continue") == 0)) {
        if (argc < 5) {
            if (json_output) {
                print_json_error(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Usage: cfrds debug <command> <rds_url> <session_id> <thread_name>");
            } else {
                usage();
            }
            return EXIT_FAILURE;
        }
        uri = argv[2];
    } else {
        uri = argv[2];
    }

    if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
    {
        HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "init_server_from_uri FAILED!");
    }

    if (path == NULL) {
        path = strdup("/");
    }

    if (username == NULL) {
        username = strdup("");
    }

    if (password == NULL) {
        password = strdup("");
    }

    if (!cfrds_server_init(&server, hostname, port, username, password))
    {
        HANDLE_ERROR(CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED, "cfrds_server_init FAILED!");
    }

    if ((strcmp(command, "ls") == 0)||(strcmp(command, "dir") == 0)) {
        cfrds_browse_dir_defer(dir);
        res = cfrds_command_browse_dir(server, path, &dir);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "ls/dir FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            struct json_object *arr = json_object_new_array();
            size_t cnt = cfrds_browse_dir_count(dir);
            for (size_t c = 0; c < cnt; c++)
            {
                char kind = cfrds_browse_dir_item_get_kind(dir, c);
                const char *name = cfrds_browse_dir_item_get_name(dir, c);
                uint8_t permissions = cfrds_browse_dir_item_get_permissions(dir, c);
                size_t size = cfrds_browse_dir_item_get_size(dir, c);
                uint64_t modified = cfrds_browse_dir_item_get_modified(dir, c);

                char permissions_str[] = "-----";
                if (kind == 'D') permissions_str[0] = 'D';
                if (permissions & 0x01) permissions_str[1] = 'R';
                if (permissions & 0x02) permissions_str[2] = 'H';
                if (permissions & 0x10) permissions_str[3] = 'A';
                if (permissions & 0x80) permissions_str[4] = 'N';

                struct json_object *item = json_object_new_object();
                char kind_str[2] = { kind, '\0' };
                json_object_object_add(item, "name", json_object_new_string(name ? name : ""));
                json_object_object_add(item, "kind", json_object_new_string(kind_str));
                json_object_object_add(item, "permissions", json_object_new_string(permissions_str));
                json_object_object_add(item, "size", json_object_new_int64((int64_t)size));
                json_object_object_add(item, "modified", json_object_new_int64((int64_t)modified));
                json_object_array_add(arr, item);
            }
            json_object_object_add(obj, "items", arr);
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            size_t cnt = cfrds_browse_dir_count(dir);
            for (size_t c = 0; c < cnt; c++)
            {
                char kind = cfrds_browse_dir_item_get_kind(dir, c);
                const char *name = cfrds_browse_dir_item_get_name(dir, c);
                uint8_t permissions = cfrds_browse_dir_item_get_permissions(dir, c);
                size_t size = cfrds_browse_dir_item_get_size(dir, c);
                uint64_t modified = cfrds_browse_dir_item_get_modified(dir, c);

                char permissions_str[] = "-----";
                if (kind == 'D') permissions_str[0] = 'D';
                if (permissions & 0x01) permissions_str[1] = 'R';
                if (permissions & 0x02) permissions_str[2] = 'H';
                if (permissions & 0x10) permissions_str[3] = 'A';
                if (permissions & 0x80) permissions_str[4] = 'N';

                const time_t timep = (time_t)(modified / 1000);
                const struct tm *newtime = localtime(&timep);
                char modified_str[64] = {0, };
                strftime(modified_str, sizeof(modified_str), "%c", newtime);

                printf("%s %12zu %s %s\n", permissions_str, size, modified_str, name);
            }
        }
    } else if (strcmp(command, "cat") == 0) {
        res = cfrds_command_file_read(server, path, &content);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cat FAILED with error");
        }

        size_t to_write = cfrds_file_content_get_size(content);
        if (to_write == (size_t)-1)
        {
            HANDLE_SERVER_ERROR(CFRDS_STATUS_RESPONSE_ERROR, "cat FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            struct json_object *content_str = json_object_new_string_len(cfrds_file_content_get_data(content), (int)to_write);
            json_object_object_add(obj, "content", content_str);
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            ssize_t written = os_write_to_terminal(cfrds_file_content_get_data(content), to_write);
            if ((written < 0) || ((size_t)written != to_write))
            {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "write FAILED with error: %s", strerror(errno));
            }
        }
    } else if ((strcmp(command, "get") == 0)||(strcmp(command, "download") == 0)) {
        res = cfrds_command_file_read(server, path, &content);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "get/download FAILED with error");
        }

        const char *dest_fname = argv[3];

        os_file_defer(fd);

        fd = os_creat_file(dest_fname);
        if (fd == ERROR_FILE_HND_FD)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "open FAILED with error: %s", strerror(errno));
        }

        size_t to_write = cfrds_file_content_get_size(content);
        if (to_write == (size_t)-1)
        {
            HANDLE_SERVER_ERROR(CFRDS_STATUS_RESPONSE_ERROR, "get/download FAILED with error");
        }

        ssize_t written = os_write(fd, cfrds_file_content_get_data(content), to_write);
        if ((written < 0) || ((size_t)written != to_write))
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "write FAILED with error: %s", strerror(errno));
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "local_path", json_object_new_string(dest_fname));
            json_object_object_add(obj, "size", json_object_new_int64((int64_t)to_write));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        const char *src_fname = argv[2];
        size_t src_size = 0;
        void *buf = NULL;

        buf = os_map(src_fname, &src_size);
        if ((buf == NULL)&&(src_size > 0))
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "mmap FAILED with error: %s", strerror(errno));
        }

        res = cfrds_command_file_write(server, path, buf, src_size);
        if (res != CFRDS_STATUS_OK) {
            os_unmap(buf, src_size);
            HANDLE_SERVER_ERROR(res, "upload FAILED with error");
        }
        os_unmap(buf, src_size);

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "remote_path", json_object_new_string(path));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if ((strcmp(command, "mv") == 0)||(strcmp(command, "move") == 0)) {
        const char *dest_pathname = argv[3];
        res = cfrds_command_file_rename(server, path, dest_pathname);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "mv/move FAILED with error");
        }
        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if ((strcmp(command, "rm") == 0)||(strcmp(command, "delete") == 0)) {
        res = cfrds_command_file_remove_file(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "rm/delete FAILED with error");
        }
        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "mkdir") == 0) {
        res = cfrds_command_file_create_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "mkdir FAILED with error");
        }
        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "rmdir") == 0) {
        res = cfrds_command_file_remove_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "rmdir FAILED with error");
        }
        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "cfroot") == 0) {
        res = cfrds_command_file_get_root_dir(server, &cfroot);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cfroot FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "cfroot", json_object_new_string(cfroot ? cfroot : ""));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            puts(cfroot);
        }
    } else if (strcmp(command, "dsninfo") == 0) {
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
    } else if (strcmp(command, "security_analyzer") == 0) {
        int command_id = 0;
        res = cfrds_command_security_analyzer_scan(server, path, true, 0, &command_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cfrds_command_security_analyzer_scan FAILED with error");
        }

        while(1)
        {
            int totalfiles = 0;
            int filesvisitedcount = 0;
            int percentage = 0;
            int64_t lastupdated = 0;

            res = cfrds_command_security_analyzer_status(server, command_id, &totalfiles, &filesvisitedcount, &percentage, &lastupdated);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_security_analyzer_status FAILED with error");
            }

            if (percentage >= 100)
                break;

            if (!json_output)
            {
                printf("progress: %d%%\r", percentage);
                fflush(stdout);
            }

#ifdef _WIN32
            Sleep(250);
#else
            usleep(250000); // 250ms
#endif
        }

        cfrds_security_analyzer_result_defer(analyzer_result);

        res = cfrds_command_security_analyzer_result(server, command_id, &analyzer_result);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cfrds_command_security_analyzer_result FAILED with error");
        }

        cfrds_str_defer(status);
        status = cfrds_security_analyzer_result_status(analyzer_result);
        if (strcmp(status, "success") != 0)
        {
            HANDLE_ERROR(CFRDS_STATUS_RESPONSE_ERROR, "cfrds_command_security_analyzer_result has with status: %s", status);
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "totalfiles", json_object_new_int(cfrds_security_analyzer_result_totalfiles(analyzer_result)));
            json_object_object_add(obj, "filesvisitedcount", json_object_new_int(cfrds_security_analyzer_result_filesvisitedcount(analyzer_result)));

            struct json_object *not_scanned_arr = json_object_new_array();
            int total = cfrds_security_analyzer_result_filesnotscanned_count(analyzer_result);
            for(int ndx = 0; ndx < total; ndx++)
            {
                cfrds_str_defer(reason);
                cfrds_str_defer(filename);
                reason = cfrds_security_analyzer_result_filesnotscanned_item_reason(analyzer_result, (size_t)ndx);
                filename = cfrds_security_analyzer_result_filesnotscanned_item_filename(analyzer_result, (size_t)ndx);

                struct json_object *item = json_object_new_object();
                json_object_object_add(item, "reason", json_object_new_string(reason ? reason : ""));
                json_object_object_add(item, "filename", json_object_new_string(filename ? filename : ""));
                json_object_array_add(not_scanned_arr, item);
            }
            json_object_object_add(obj, "filesnotscanned", not_scanned_arr);

            struct json_object *scanned_arr = json_object_new_array();
            total = cfrds_security_analyzer_result_filesscanned_count(analyzer_result);
            for(int ndx = 0; ndx < total; ndx++)
            {
                cfrds_str_defer(result);
                cfrds_str_defer(filename);
                result = cfrds_security_analyzer_result_filesscanned_item_result(analyzer_result, (size_t)ndx);
                filename = cfrds_security_analyzer_result_filesscanned_item_filename(analyzer_result, (size_t)ndx);

                struct json_object *item = json_object_new_object();
                json_object_object_add(item, "result", json_object_new_string(result ? result : ""));
                json_object_object_add(item, "filename", json_object_new_string(filename ? filename : ""));
                json_object_array_add(scanned_arr, item);
            }
            json_object_object_add(obj, "filesscanned", scanned_arr);

            struct json_object *errors_arr = json_object_new_array();
            total = cfrds_security_analyzer_result_errors_count(analyzer_result);
            for(int ndx = 0; ndx < total; ndx++)
            {
                cfrds_str_defer(type);
                cfrds_str_defer(filename);
                cfrds_str_defer(error);
                cfrds_str_defer(errormessage);
                int line = 0;

                type = cfrds_security_analyzer_result_errors_item_type(analyzer_result, (size_t)ndx);
                filename = cfrds_security_analyzer_result_errors_item_filename(analyzer_result, (size_t)ndx);
                line = cfrds_security_analyzer_result_errors_item_beginline(analyzer_result, (size_t)ndx);
                error = cfrds_security_analyzer_result_errors_item_error(analyzer_result, (size_t)ndx);
                errormessage = cfrds_security_analyzer_result_errors_item_errormessage(analyzer_result, (size_t)ndx);

                struct json_object *item = json_object_new_object();
                json_object_object_add(item, "type", json_object_new_string(type ? type : ""));
                json_object_object_add(item, "filename", json_object_new_string(filename ? filename : ""));
                json_object_object_add(item, "line", json_object_new_int(line));
                json_object_object_add(item, "error", json_object_new_string(error ? error : ""));
                json_object_object_add(item, "errormessage", json_object_new_string(errormessage ? errormessage : ""));
                json_object_array_add(errors_arr, item);
            }
            json_object_object_add(obj, "errors", errors_arr);

            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("totalfiles: %d\n", cfrds_security_analyzer_result_totalfiles(analyzer_result));
            printf("filesvisitedcount: %d\n", cfrds_security_analyzer_result_filesvisitedcount(analyzer_result));

            int total = cfrds_security_analyzer_result_filesnotscanned_count(analyzer_result);
            if (total > 0)
            {
                printf("Files NOT scanned:\n");
                for(int ndx = 0; ndx < total; ndx++)
                {
                    cfrds_str_defer(reason);
                    cfrds_str_defer(filename);

                    reason = cfrds_security_analyzer_result_filesnotscanned_item_reason(analyzer_result, (size_t)ndx);
                    filename = cfrds_security_analyzer_result_filesnotscanned_item_filename(analyzer_result, (size_t)ndx);

                    printf("\t%s - %s\n", reason, filename);
                }
            }

            printf("Files scanned:\n");
            total = cfrds_security_analyzer_result_filesscanned_count(analyzer_result);
            for(int ndx = 0; ndx < total; ndx++)
            {
                cfrds_str_defer(result);
                cfrds_str_defer(filename);

                result = cfrds_security_analyzer_result_filesscanned_item_result(analyzer_result, (size_t)ndx);
                filename = cfrds_security_analyzer_result_filesscanned_item_filename(analyzer_result, (size_t)ndx);

                printf("\t%s - %s\n", result, filename);
            }

            total = cfrds_security_analyzer_result_errors_count(analyzer_result);
            if (total > 0)
            {
                printf("Issues:\n");
                for(int ndx = 0; ndx < total; ndx++)
                {
                    cfrds_str_defer(type);
                    cfrds_str_defer(filename);
                    cfrds_str_defer(error);
                    cfrds_str_defer(errormessage);
                    int line = 0;

                    type = cfrds_security_analyzer_result_errors_item_type(analyzer_result, (size_t)ndx);
                    filename = cfrds_security_analyzer_result_errors_item_filename(analyzer_result, (size_t)ndx);
                    line = cfrds_security_analyzer_result_errors_item_beginline(analyzer_result, (size_t)ndx);
                    error = cfrds_security_analyzer_result_errors_item_error(analyzer_result, (size_t)ndx);
                    errormessage = cfrds_security_analyzer_result_errors_item_errormessage(analyzer_result, (size_t)ndx);

                    printf("\t%s - %s:%d - %s(%s)\n", type, filename, line, error, errormessage);
                }
            }
        }
        res = cfrds_command_security_analyzer_clean(server, command_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cfrds_command_security_analyzer_clean FAILED");
        }

    } else if (strcmp(command, "ide_default") == 0) {
        int num1, num2, num3;

        cfrds_str_defer(server_version);
        cfrds_str_defer(client_version);

        int version = atoi(argv[2]);

        res = cfrds_command_ide_default(server, version, &num1, &server_version, &client_version, &num2, &num3);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "ide_default FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "num1", json_object_new_int(num1));
            json_object_object_add(obj, "server_version", json_object_new_string(server_version ? server_version : ""));
            json_object_object_add(obj, "client_version", json_object_new_string(client_version ? client_version : ""));
            json_object_object_add(obj, "num2", json_object_new_int(num2));
            json_object_object_add(obj, "num3", json_object_new_int(num3));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("num1: %d\n", num1);
            printf("server_version: %s\n", server_version);
            printf("client_version: %s\n", client_version);
            printf("num2: %d\n", num2);
            printf("num3: %d\n", num3);
        }
    } else if (strcmp(command, "adminapi") == 0) {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments");
        }

        const char *subcommand = argv[3];

        if (strcmp(subcommand, "debugging_getlogproperty") == 0)
        {
            if (argc < 5)
            {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments");
            }

            const char *logdirectory = argv[4];
            char *logproperty = NULL;
            res = cfrds_command_adminapi_debugging_getlogproperty(server, logdirectory, &logproperty);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_adminapi_debugging_getlogproperty FAILED");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                json_object_object_add(obj, "logproperty", json_object_new_string(logproperty ? logproperty : ""));
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                printf("logproperty: %s\n", logproperty);
            }
        } else if (strcmp(subcommand, "extensions_getcustomtagpaths") == 0)
        {
            cfrds_adminapi_customtagpaths_defer(result);
            res = cfrds_command_adminapi_extensions_getcustomtagpaths(server, &result);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_adminapi_extensions_getcustomtagpaths FAILED");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *arr = json_object_new_array();
                int size = cfrds_adminapi_customtagpaths_count(result);
                for(int c = 0; c < size; c++)
                {
                    const char *value = cfrds_adminapi_customtagpaths_at(result, (size_t)c);
                    json_object_array_add(arr, json_object_new_string(value ? value : ""));
                }
                json_object_object_add(obj, "customtagpaths", arr);
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                printf("custom tag paths:\n");
                int size = cfrds_adminapi_customtagpaths_count(result);
                for(int c = 0; c < size; c++)
                {
                    const char *value = cfrds_adminapi_customtagpaths_at(result, (size_t)c);
                    printf("%s\n", value);
                }
            }
        } else if (strcmp(subcommand, "extensions_setmapping") == 0)
        {
            if (argc < 6)
            {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments");
            }

            const char *mapping_name = argv[4];
            const char *mapping_path = argv[5];
            res = cfrds_command_adminapi_extensions_setmapping(server, mapping_name, mapping_path);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_adminapi_extensions_setmappings FAILED");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
        } else if (strcmp(subcommand, "extensions_deletemapping") == 0)
        {
            if (argc < 5)
            {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments");
            }

            const char *arg = argv[4];
            res = cfrds_command_adminapi_extensions_deletemapping(server, arg);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_adminapi_extensions_deletemapping FAILED");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
        } else if (strcmp(subcommand, "extensions_getmappings") == 0)
        {
            cfrds_adminapi_mappings_defer(result);

            res = cfrds_command_adminapi_extensions_getmappings(server, &result);
            if (res != CFRDS_STATUS_OK)
            {
                HANDLE_SERVER_ERROR(res, "cfrds_command_adminapi_extensions_getmappings FAILED");
            }

            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                struct json_object *map_obj = json_object_new_object();
                int size = cfrds_adminapi_mappings_count(result);
                for(int c = 0; c < size; c++)
                {
                    const char *key = cfrds_adminapi_mappings_key(result, (size_t)c);
                    const char *value = cfrds_adminapi_mappings_value(result, (size_t)c);
                    json_object_object_add(map_obj, key ? key : "", json_object_new_string(value ? value : ""));
                }
                json_object_object_add(obj, "mappings", map_obj);
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                printf("mappings:\n");
                int size = cfrds_adminapi_mappings_count(result);
                for(int c = 0; c < size; c++)
                {
                    const char *key = cfrds_adminapi_mappings_key(result, (size_t)c);
                    const char *value = cfrds_adminapi_mappings_value(result, (size_t)c);
                    printf("%s => %s\n", key, value);
                }
            }
        } else {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Unknown adminapi subcommand %s", subcommand);
        }
    } else if (strcmp(command, "test_debugger") == 0)
    {
        cfrds_str_defer(debugger_session_id);
        res = cfrds_command_debugger_start(server, &debugger_session_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_start FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        printf("debugger_session_id: %s\n", debugger_session_id);

        res = cfrds_command_debugger_set_scope_filter(server, debugger_session_id, "VARIABLES,SESSION");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_set_scope_filter FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_watch_variables(server, debugger_session_id, "VARIABLES.A");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_watch_variables FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_clear_all_breakpoints(server, debugger_session_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_clear_all_breakpoints FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_breakpoint(server, debugger_session_id, "/app/test.cfm", 3, true);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_breakpoint FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_all_fetch_flags_enabled(server, debugger_session_id, true, true, true, true, true, &event);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_all_fetch_flags_enabled FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        int cf_trace_count = cfrds_debugger_event_get_cf_trace_count(event);
        printf("cf_trace_count: %d\n", cf_trace_count);
        for(int c = 0; c < cf_trace_count; c++)
        {
            const char *cf_trace = cfrds_debugger_event_get_cf_trace_item(event, (size_t)c);
            printf("cf_trace: %s\n", cf_trace);
        }

        int java_trace_count = cfrds_debugger_event_get_java_trace_count(event);
        printf("java_trace_count: %d\n", java_trace_count);
        for(int c = 0; c < java_trace_count; c++)
        {
            const char *java_trace = cfrds_debugger_event_get_java_trace_item(event, (size_t)c);
            printf("java_trace: %s\n", java_trace);
        }

        const char *thread_name = cfrds_debugger_event_breakpoint_get_thread_name(event);
        if (thread_name == NULL) thread_name = "main";
        printf("thread_name: %s\n", thread_name);

        res = cfrds_command_debugger_watch_expression(server, debugger_session_id, thread_name, "arrayNew(1)");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_watch_expression FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_set_variable(server, debugger_session_id, thread_name, "VARIABLES.A", "200");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_set_variable FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        const char *source = cfrds_debugger_event_breakpoint_get_source(event);
        printf("source: %s\n", source);

        int line = cfrds_debugger_event_breakpoint_get_line(event);
        printf("line: %d\n", line);

        int scopes_count = cfrds_debugger_event_get_scopes_count(event);
        printf("scopes_count: %d\n", scopes_count);
        for(int c = 0; c < scopes_count; c++)
        {
            const char *scopes_item = cfrds_debugger_event_get_scopes_item(event, (size_t)c);
            printf("scopes_item: %s\n", scopes_item ? scopes_item : "(null)");
        }

        int threads_count = cfrds_debugger_event_get_threads_count(event);
        printf("threads_count: %d\n", threads_count);
        for(int c = 0; c < threads_count; c++)
        {
            const char *threads_item = cfrds_debugger_event_get_threads_item(event, (size_t)c);
            printf("threads_item: %s\n", threads_item ? threads_item : "(null)");
        }

        int watch_count = cfrds_debugger_event_get_watch_count(event);
        printf("watch_count: %d\n", watch_count);
        for(int c = 0; c < watch_count; c++)
        {
            const char *watch_item = cfrds_debugger_event_get_watch_item(event, (size_t)c);
            printf("watch_item: %s\n", watch_item ? watch_item : "(null)");
        }

        res = cfrds_command_debugger_step_over(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_step_over FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_step_in(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_step_in FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_step_out(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_step_out FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_continue(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_continue FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_breakpoint(server, debugger_session_id, "/app/test.cfm", 3, false);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_breakpoint FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_stop(server, debugger_session_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_stop FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

    } else if (strcmp(command, "step_in") == 0)
    {
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        res = cfrds_command_debugger_step_in(server, session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "step_in FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "step_over") == 0)
    {
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        res = cfrds_command_debugger_step_over(server, session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "step_over FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "step_out") == 0)
    {
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        res = cfrds_command_debugger_step_out(server, session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "step_out FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "continue") == 0)
    {
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        res = cfrds_command_debugger_continue(server, session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "continue FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
    } else if (strcmp(command, "graphing") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Usage: graphing <rds_url> <chart_attributes> [out_file.png] [series1] ...");
        }

        const char *chart_attr = argv[3];
        const char *out_path = (argc >= 5) ? argv[4] : NULL;
        size_t num_series = (argc >= 6) ? (size_t)(argc - 5) : 0;
        const char **series_data = (num_series > 0) ? (const char **)&argv[5] : NULL;

        cfrds_buffer_defer(chart_buf);
        res = cfrds_command_graphing(server, &chart_buf, chart_attr, num_series, series_data);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "graphing FAILED");
        }

        if (out_path && chart_buf)
        {
            FILE *f = fopen(out_path, "wb");
            if (f)
            {
                fwrite(cfrds_buffer_data(chart_buf), 1, cfrds_buffer_data_size(chart_buf), f);
                fclose(f);

                if (json_output)
                {
                    struct json_object *obj = json_object_new_object();
                    json_object_object_add(obj, "status", json_object_new_string("success"));
                    json_object_object_add(obj, "local_path", json_object_new_string(out_path));
                    json_object_object_add(obj, "size", json_object_new_int64((int64_t)cfrds_buffer_data_size(chart_buf)));
                    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                    json_object_put(obj);
                }
                else
                {
                    printf("Saved graph image to %s (%zu bytes)\n", out_path, cfrds_buffer_data_size(chart_buf));
                }
            }
            else
            {
                HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Failed to open %s for writing", out_path);
            }
        }
        else if (chart_buf)
        {
            if (json_output)
            {
                struct json_object *obj = json_object_new_object();
                json_object_object_add(obj, "status", json_object_new_string("success"));
                json_object_object_add(obj, "size", json_object_new_int64((int64_t)cfrds_buffer_data_size(chart_buf)));
                char *b64 = base64_encode((const unsigned char *)cfrds_buffer_data(chart_buf), cfrds_buffer_data_size(chart_buf));
                if (b64)
                {
                    json_object_object_add(obj, "image_b64", json_object_new_string(b64));
                    free(b64);
                }
                printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
                json_object_put(obj);
            }
            else
            {
                printf("Graph rendered successfully (%zu bytes)\n", cfrds_buffer_data_size(chart_buf));
            }
        }
    } else {
        HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Unknown command %s", command);
    }

    return EXIT_SUCCESS;
}
