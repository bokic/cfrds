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


#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

static void usage()
{
    printf("Usage: cfrds <command> [options] <url> [options]\n"
           "commands:\n"
           "  - 'ls', 'dir' - List a server directory.\n"
           "         example: `cfrds ls <rds://[username[:password]@]host[:port]/[path]>`\n"
           "            or    `cfrds dir <rds://[username[:password]@]host[:port]/[path]>`\n"
           "\n"
           "  - 'cat' - Print server file content to stdout.\n"
           "         example: `cfrds cat <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "\n"
           "  - 'get', 'download' - Download a file from server.\n"
           "         example: `cfrds get <rds://[username[:password]@]host[:port]</pathname>> <local_pathname>`\n"
           "            or    `cfrds download <rds://[username[:password]@]host[:port]</pathname>> <local_pathname>`\n"
           "\n"
           "  - 'put', 'upload' - Upload a file to server.\n"
           "         example: `cfrds put <local_pathname> <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "            or    `cfrds upload <local_pathname> <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "\n"
           "  - 'mv', 'move' - Move/rename file or folder.\n"
           "         example: `cfrds mv <rds://[username[:password]@]host[:port]</pathname>> <new_pathname>`\n"
           "            or    `cfrds move <rds://[username[:password]@]host[:port]</pathname>> <new_pathname>`\n"
           "\n"
           "  - 'rm', 'delete' - Delete a file from server.\n"
           "         example: `cfrds rm <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "            or    `cfrds delete <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "\n"
           "  - 'mkdir' - Create a directory on a server.\n"
           "         example: `cfrds mkdir <rds://[username[:password]@]host[:port]</path>>`\n"
           "\n"
           "  - 'rmdir' - Delete a directory from a server.\n"
           "         example: `cfrds rmdir <rds://[username[:password]@]host[:port]</path>>`\n"
           "\n"
           "  - 'cfroot' - Return ColdFusion installation directory.\n"
           "         example: `cfrds cfroot <rds://[username[:password]@]host[:port]>`\n"
           "\n"
           "  - 'dsninfo' - Return ColdFusion data sources.\n"
           "         example: `cfrds dsninfo <rds://[username[:password]@]host[:port]>`\n"
           "\n"
           "  - 'tableinfo' - Return ColdFusion datasource tables info.\n"
           "         example: `cfrds tableinfo <rds://[username[:password]@]host[:port]/<dsn_name>>`\n"
           "\n"
           "  - 'columninfo' - Return ColdFusion datasource table columns info.\n"
           "         example: `cfrds columninfo <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n"
           "\n"
           "  - 'primarykeys' - Return ColdFusion datasource table primary keys info.\n"
           "         example: `cfrds primarykeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n"
           "\n"
           "  - 'foreignkeys' - Return ColdFusion datasource table foreign keys info.\n"
           "         example: `cfrds foreignkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n"
           "\n"
           "  - 'importedkeys' - Return ColdFusion datasource table imported keys info.\n"
           "         example: `cfrds importedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n"
           "\n"
           "  - 'exportedkeys' - Return ColdFusion datasource table exported keys info.\n"
           "         example: `cfrds exportedkeys <rds://[username[:password]@]host[:port]/<dsn_name>/<table_name>>`\n"
           "\n"
           "  - 'sql' - Execute SQL statement on ColdFusion data sources.\n"
           "         example: `cfrds sql <rds://[username[:password]@]host[:port]/<dsn_name>> \"<sql_statement>\"`\n"
           "\n"
           "  - 'sqlmetadata' - Return SQL statement metadata on ColdFusion data sources.\n"
           "         example: `cfrds sqlmetadata <rds://[username[:password]@]host[:port]/<dsn_name>> \"<sql_statement>\"`\n"
           "\n"
           "  - 'sqlsupportedcommands' - Return SQL statement supported commands on ColdFusion data sources.\n"
           "         example: `cfrds sqlsupportedcommands <rds://[username[:password]@]host[:port]/<dsn_name>>`\n"
           "\n"
           "  - 'dbdescription' - Return ColdFusion data sources database info.\n"
           "         example: `cfrds dbdescription <rds://[username[:password]@]host[:port]/<dsn_name>>`\n"
           "\n"
           "  - 'security_analyzer' - Get ColdFusion server information.\n"
           "         example: `cfrds security_analyzer <rds://[username[:password]@]host[:port]</pathname>>`\n"
           "\n"
           "  - 'ide_default' - Get ColdFusion server information.\n"
           "         example: `cfrds ide_default <rds://[username[:password]@]host[:port]> <version>`\n"
           "\n"
           "  - 'adminapi' - Get ColdFusion server information.\n"
           "         examples:\n"
           "           `cfrds adminapi <rds://[username[:password]@]host[:port]> debugging_getlogproperty <log_directory>`\n"
           "           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getcustomtagpaths`\n"
           "           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_setmapping <mapping_name> <mapping_path>`\n"
           "           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_deletemapping <mapping_name>`\n"
           "           `cfrds adminapi <rds://[username[:password]@]host[:port]> extensions_getmappings`\n"
           "\n"
           );
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
        size_t path_strlen = uri_strlen - (path_start - uri);
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
            size_t user_strlen = pass_start - uri;
            size_t pass_strlen = login_start - pass_start - 1;

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
            size_t user_strlen = login_start - uri;
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
        size_t host_strlen = port_start - uri;
        size_t port_strlen = path_start - port_start - 1;

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
        size_t host_strlen = path_start - uri;
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
    if ((argc == 2)&&((strcmp(argv[1], "-v") == 0)||(strcmp(argv[1], "--version") == 0)))
    {
        printf("cfrds version: " CFRDS_VERSION "\n");
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
        usage();
        return EXIT_FAILURE;
    }

    const char *command = argv[1];

    if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        if (argc < 4) {
            usage();
            return EXIT_FAILURE;
        }
        uri = argv[3];
    } else if (strcmp(command, "dbg_brk") == 0) {
        if (argc < 5) {
            usage();
            return EXIT_FAILURE;
        }
        uri = argv[2];
    } else {
        uri = argv[2];
    }

    if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
    {
        fprintf(stderr, "init_server_from_uri FAILED!\n");
        return EXIT_FAILURE;
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
        fprintf(stderr, "cfrds_server_init FAILED!\n");
        return EXIT_FAILURE;
    }

    if ((strcmp(command, "ls") == 0)||(strcmp(command, "dir") == 0)) {
        cfrds_browse_dir_defer(dir);
        res = cfrds_command_browse_dir(server, path, &dir);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "ls/dir FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

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

            const time_t timep = modified / 1000;
            const struct tm *newtime = localtime(&timep);
            char modified_str[64] = {0, };
            strftime(modified_str, sizeof(modified_str), "%c", newtime);

            printf("%s %12zu %s %s\n", permissions_str, size, modified_str, name);
        }
    } else if (strcmp(command, "cat") == 0) {
        res = cfrds_command_file_read(server, path, &content);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cat FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        int to_write = cfrds_file_content_get_size(content);
        ssize_t written = os_write_to_terminal(cfrds_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
    } else if ((strcmp(command, "get") == 0)||(strcmp(command, "download") == 0)) {
        res = cfrds_command_file_read(server, path, &content);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "get/download FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        const char *dest_fname = argv[3];

        os_file_defer(fd);

        fd = os_creat_file(dest_fname);
        if (fd == ERROR_FILE_HND_FD)
        {
            fprintf(stderr, "open FAILED with error: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        int to_write = cfrds_file_content_get_size(content);
        ssize_t written = os_write(fd, cfrds_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
    } else if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        const char *src_fname = argv[2];
        size_t src_size = 0;
        void *buf = NULL;

        buf = os_map(src_fname, &src_size);
        if ((buf == NULL)&&(src_size > 0))
        {
            fprintf(stderr, "mmap FAILED with error: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }

        res = cfrds_command_file_write(server, path, buf, src_size);
        if (res != CFRDS_STATUS_OK) {
            fprintf(stderr, "upload FAILED with error: %s\n", cfrds_server_get_error(server));
        }
        os_unmap(buf, src_size);
    } else if ((strcmp(command, "mv") == 0)||(strcmp(command, "move") == 0)) {
        const char *dest_pathname = argv[3];
        res = cfrds_command_file_rename(server, path, dest_pathname);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "mv/move FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }
    } else if ((strcmp(command, "rm") == 0)||(strcmp(command, "delete") == 0)) {
        res = cfrds_command_file_remove_file(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "rm/delete FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }
    } else if (strcmp(command, "mkdir") == 0) {
        res = cfrds_command_file_create_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "mkdir FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }
    } else if (strcmp(command, "rmdir") == 0) {
        res = cfrds_command_file_remove_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "rmdir FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }
    } else if (strcmp(command, "cfroot") == 0) {
        res = cfrds_command_file_get_root_dir(server, &cfroot);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfroot FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        puts(cfroot);
    } else if (strcmp(command, "dsninfo") == 0) {
        cfrds_sql_dsninfo_defer(dsninfo);
        res = cfrds_command_sql_dsninfo(server, &dsninfo);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "dsninfo FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        size_t cnt = cfrds_sql_dsninfo_count(dsninfo);
        for(size_t c = 0; c < cnt; c++)
        {
            const char *item = cfrds_sql_dsninfo_item_get_name(dsninfo, c);
            puts(item);
        }
    } else if (strcmp(command, "tableinfo") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_tableinfo_defer(tableinfo);
            const char *dsn = path + 1;

            res = cfrds_command_sql_tableinfo(server, dsn, &tableinfo);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "tableinfo FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_sql_tableinfo_count(tableinfo);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *name = cfrds_sql_tableinfo_get_column_name(tableinfo, c);
                const char *type = cfrds_sql_tableinfo_get_column_type(tableinfo, c);
                printf("%s, %s\n", name, type);
            }
        } else {
            fprintf(stderr, "No schema name\n");
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

                tmp_size = table_separator - schema_separator;
                schema = malloc(tmp_size + 1);
                if (schema == NULL)
                {
                    fprintf(stderr, "malloc FAILED!\n");
                    return EXIT_FAILURE;
                }

                memcpy(schema, schema_separator, tmp_size);
                schema[tmp_size] = '\0';

                const char *table = table_separator + 1;

                res = cfrds_command_sql_columninfo(server, schema, table, &columninfo);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "columninfo FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_sql_columninfo_count(columninfo);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name = cfrds_sql_columninfo_get_name(columninfo, c);
                    const char *type = cfrds_sql_columninfo_get_typeStr(columninfo, c);
                    printf("%s, %s\n", name, type);
                }
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
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

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_primarykeys(server, schema, tablename, &primarykeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "primarykeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

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
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
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

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_foreignkeys(server, schema, tablename, &foreignkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "foreignkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

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
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
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

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_importedkeys(server, schema, tablename, &importedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "importedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

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
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
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

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == NULL)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == NULL)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_exportedkeys(server, schema, tablename, &exportedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "exportedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

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
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
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
                fprintf(stderr, "sql FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            int cols = cfrds_sql_resultset_columns(resultset);
            if (cols == 0)
            {
                fprintf(stderr, "No columns\n");
                return EXIT_FAILURE;
            }

            size_t *sizes = malloc(cols * sizeof(size_t));
            if (sizes == NULL)
            {
                fprintf(stderr, "No memory\n");
                return EXIT_FAILURE;
            }

            for(int c = 0; c < cols; c++)
            {
                const char *name  = cfrds_sql_resultset_column_name(resultset, c);
                sizes[c] = strlen(name);
            }

            int rows = cfrds_sql_resultset_rows(resultset);
            for (int r = 0; r < rows; r++)
            {
                for (int c = 0; c < cols; c++)
                {
                    const char *value = cfrds_sql_resultset_value(resultset, r, c);
                    if (strlen(value) > sizes[c])
                    {
                        sizes[c] = strlen(value);
                    }
                }
            }

            printf((const char*)u8"\u250F");
            for(int col = 0; col < cols; col++)
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
            for(int col = 0; col < cols; col++)
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
                for(int col = 0; col < cols; col++)
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
                for(int col = 0; col < cols; col++)
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

            for (int row = 0; row < rows; row++)
            {
                printf((const char*)u8"\u2503");
                for(int col = 0; col < cols; col++)
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
    } else if (strcmp(command, "sqlmetadata") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_metadata_defer(metadata);
            const char *schema = path + 1;

            const char *sql = argv[3];

            res = cfrds_command_sql_sqlmetadata(server, schema, sql, &metadata);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "sqlmetadata FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_sql_metadata_count(metadata);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *name  = cfrds_sql_metadata_get_name(metadata, c);
                const char *type  = cfrds_sql_metadata_get_type(metadata, c);
                const char *jtype = cfrds_sql_metadata_get_jtype(metadata, c);

                printf("name: '%s', type: '%s', jtype: '%s'\n", name, type, jtype);
            }
        }
    } else if (strcmp(command, "supportedcommands") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_supportedcommands_defer(supportedcommands);

            res = cfrds_command_sql_getsupportedcommands(server, &supportedcommands);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "supportedcommands FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_sql_supportedcommands_count(supportedcommands);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *supportedcommand = cfrds_sql_supportedcommands_get(supportedcommands, c);
                printf("%s\n", supportedcommand);
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
                fprintf(stderr, "dbdescription FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            printf("%s\n", dbdescription);
        }
    } else if (strcmp(command, "security_analyzer") == 0) {
        int command_id = 0;
        res = cfrds_command_security_analyzer_scan(server, path, true, 0, &command_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_security_analyzer_scan FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
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
                fprintf(stderr, "cfrds_command_security_analyzer_status FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            if (percentage >= 100)
                break;

            printf("progress: %d%%\r", percentage);
            fflush(stdout);

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
            fprintf(stderr, "cfrds_command_security_analyzer_result FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        cfrds_str_defer(status);
        status = cfrds_security_analyzer_result_status(analyzer_result);
        if (strcmp(status, "success") != 0)
        {
            fprintf(stderr, "cfrds_command_security_analyzer_result has with status: %s\n", status);
            return EXIT_FAILURE;
        }

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

                reason = cfrds_security_analyzer_result_filesnotscanned_item_reason(analyzer_result, ndx);
                filename = cfrds_security_analyzer_result_filesnotscanned_item_filename(analyzer_result, ndx);

                printf("\t%s - %s\n", reason, filename);
            }
        }

        printf("Files scanned:\n");
        total = cfrds_security_analyzer_result_filesscanned_count(analyzer_result);
        for(int ndx = 0; ndx < total; ndx++)
        {
            cfrds_str_defer(result);
            cfrds_str_defer(filename);

            result = cfrds_security_analyzer_result_filesscanned_item_result(analyzer_result, ndx);
            filename = cfrds_security_analyzer_result_filesscanned_item_filename(analyzer_result, ndx);

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

                type = cfrds_security_analyzer_result_errors_item_type(analyzer_result, ndx);
                filename = cfrds_security_analyzer_result_errors_item_filename(analyzer_result, ndx);
                line = cfrds_security_analyzer_result_errors_item_beginline(analyzer_result, ndx);
                error = cfrds_security_analyzer_result_errors_item_error(analyzer_result, ndx);
                errormessage = cfrds_security_analyzer_result_errors_item_errormessage(analyzer_result, ndx);

                printf("\t%s - %s:%d - %s(%s)\n", type, filename, line, error, errormessage);
            }
        }
        res = cfrds_command_security_analyzer_clean(server, command_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_security_analyzer_clean FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

    } else if (strcmp(command, "ide_default") == 0) {
        int num1, num2, num3;

        cfrds_str_defer(server_version);
        cfrds_str_defer(client_version);

        int version = atoi(argv[2]);

        res = cfrds_command_ide_default(server, version, &num1, &server_version, &client_version, &num2, &num3);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "ide_default FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        printf("num1: %d\n", num1);
        printf("server_version: %s\n", server_version);
        printf("client_version: %s\n", client_version);
        printf("num2: %d\n", num2);
        printf("num3: %d\n", num3);
    } else if (strcmp(command, "adminapi") == 0) {
        if (argc < 4)
        {
            fprintf(stderr, "Not enough arguments\n");
            return EXIT_FAILURE;
        }

        const char *subcommand = argv[3];

        if (strcmp(subcommand, "debugging_getlogproperty") == 0)
        {

            if (argc < 5)
            {
                fprintf(stderr, "Not enough arguments\n");
                return EXIT_FAILURE;
            }

            const char *logdirectory = argv[4];
            char *logproperty = NULL;
            res = cfrds_command_adminapi_debugging_getlogproperty(server, logdirectory, &logproperty);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "cfrds_command_adminapi_debugging_getlogproperty FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            printf("logproperty: %s\n", logproperty);
        } else if (strcmp(subcommand, "extensions_getcustomtagpaths") == 0)
        {
            cfrds_adminapi_customtagpaths_defer(result);
            res = cfrds_command_adminapi_extensions_getcustomtagpaths(server, &result);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "cfrds_command_adminapi_extensions_getcustomtagpaths FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            printf("custom tag paths:\n");

            int size = cfrds_adminapi_customtagpaths_count(result);
            for(int c = 0; c < size; c++)
            {
                const char *value = cfrds_adminapi_customtagpaths_at(result, c);
                printf("%s\n", value);
            }
        } else if (strcmp(subcommand, "extensions_setmapping") == 0)
        {
            char **result = NULL;

            if (argc < 6)
            {
                fprintf(stderr, "Not enough arguments\n");
                return EXIT_FAILURE;
            }

            const char *mapping_name = argv[4];
            const char *mapping_path = argv[5];
            res = cfrds_command_adminapi_extensions_setmapping(server, mapping_name, mapping_path);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "cfrds_command_adminapi_extensions_setmappings FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }
        } else if (strcmp(subcommand, "extensions_deletemapping") == 0)
        {
            char **result = NULL;

            if (argc < 5)
            {
                fprintf(stderr, "Not enough arguments\n");
                return EXIT_FAILURE;
            }

            const char *arg = argv[4];
            res = cfrds_command_adminapi_extensions_deletemapping(server, arg);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "cfrds_command_adminapi_extensions_deletemapping FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }
        } else if (strcmp(subcommand, "extensions_getmappings") == 0)
        {
            cfrds_adminapi_mappings_defer(result);

            res = cfrds_command_adminapi_extensions_getmappings(server, &result);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "cfrds_command_adminapi_extensions_getmappings FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            printf("mappings:\n");

            int size = cfrds_adminapi_mappings_count(result);
            for(int c = 0; c < size; c++)
            {
                const char *key = cfrds_adminapi_mappings_key(result, c);
                const char *value = cfrds_adminapi_mappings_value(result, c);
                printf("%s => %s\n", key, value);
            }
        } else {
            fprintf(stderr, "Unknown adminapi subcommand %s\n", subcommand);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
