#include "cmd_common.h"

bool json_output = false;

void print_json_error(cfrds_status res, const char *err_msg)
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "status", json_object_new_string("error"));
    json_object_object_add(obj, "error", json_object_new_string(err_msg ? err_msg : ""));
    json_object_object_add(obj, "code", json_object_new_int(res));
    printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
    json_object_put(obj);
}

char *base64_encode(const unsigned char *data, size_t input_length) {
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

static void usage(void)
{
    printf("Usage: cfrds <command> <url> [arguments...] [--json]\n");
    printf("options:\n");
    printf("  --json       - Output result in JSON format (tail argument only).\n");
    printf("\n");
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
    printf("  - 'dbg_start', 'dbg_stop', 'dbg_server_stop', 'dbg_info' - Debugger session management.\n");
    printf("         examples:\n");
    printf("           `cfrds dbg_start <rds://[username[:password]@]host[:port]>`\n");
    printf("           `cfrds dbg_stop <rds://[username[:password]@]host[:port]> <session_id>`\n");
    printf("           `cfrds dbg_server_stop <rds://[username[:password]@]host[:port]> <session_id>`\n");
    printf("           `cfrds dbg_info <rds://[username[:password]@]host[:port]> <session_id>`\n");
    printf("\n");
    printf("  - 'dbg_breakpoint', 'dbg_clear_breakpoints' - Breakpoint control.\n");
    printf("         examples:\n");
    printf("           `cfrds dbg_breakpoint <rds://[username[:password]@]host[:port]> <session_id> <filepath> <line> [1|0]`\n");
    printf("           `cfrds dbg_clear_breakpoints <rds://[username[:password]@]host[:port]> <session_id>`\n");
    printf("           `cfrds dbg_break_on_exception <rds://[username[:password]@]host[:port]> <session_id> [1|0]`\n");
    printf("           `cfrds dbg_global_break_on_exception <rds://[username[:password]@]host[:port]> <session_id> [1|0]`\n");
    printf("\n");
    printf("  - 'step_in', 'step_over', 'step_out', 'continue', 'dbg_sync_step_in', 'dbg_sync_step_over', 'dbg_sync_step_out' - Execution stepping.\n");
    printf("         example: `cfrds step_in <rds://[username[:password]@]host[:port]> <session_id> <thread_name>`\n");
    printf("\n");
    printf("  - 'dbg_events', 'dbg_get_cf_variables', 'dbg_watch_expression', 'dbg_set_variable', 'dbg_watch_variables', 'dbg_get_output', 'dbg_set_scope_filter' - State inspection & manipulation.\n");
    printf("         examples:\n");
    printf("           `cfrds dbg_events <rds://[username[:password]@]host[:port]> <session_id> [threads] [watch] [scopes] [cf_trace] [java_trace]`\n");
    printf("           `cfrds dbg_get_cf_variables <rds://[username[:password]@]host[:port]> <session_id> <thread_name>`\n");
    printf("           `cfrds dbg_watch_expression <rds://[username[:password]@]host[:port]> <session_id> <thread_name> <expression>`\n");
    printf("           `cfrds dbg_set_variable <rds://[username[:password]@]host[:port]> <session_id> <thread_name> <var> <val>`\n");
    printf("           `cfrds dbg_watch_variables <rds://[username[:password]@]host[:port]> <session_id> <vars>`\n");
    printf("           `cfrds dbg_get_output <rds://[username[:password]@]host[:port]> <session_id> <thread_name>`\n");
    printf("           `cfrds dbg_set_scope_filter <rds://[username[:password]@]host[:port]> <session_id> <filter>`\n");
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

    const char *port_start = memchr(uri, ':', (size_t)(path_start - uri));
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

    int file_rc = handle_cmd_file(server, command, path, argc, argv, &cfroot);
    if (file_rc >= 0) return file_rc;

    int sql_rc = handle_cmd_sql(server, command, path, argc, argv);
    if (sql_rc >= 0) return sql_rc;

    int dbg_rc = handle_cmd_debugger(server, command, argc, argv);
    if (dbg_rc >= 0) return dbg_rc;

    int sec_rc = handle_cmd_security(server, command, path);
    if (sec_rc >= 0) return sec_rc;

    if (strcmp(command, "ide_default") == 0) {
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
            cfrds_str_defer(logproperty);
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
