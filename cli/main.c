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
#include <time.h>


#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

static void usage()
{
    printf("Usage: cfrds <command> {..} <uri> {..}\n"
           "commands:\n"
           "  - 'ls', 'dir' - List a server directorory.\n"
           "         example: `cfrds ls rds://username:password@host/dir`\n"
           "            or    `cfrds dir rds://username:password@host/dir`\n"
           "\n"
           "  - 'cat' - Print server file content to stdout.\n"
           "         example: `cfrds cat rds://username:password@host/pathname`\n"
           "\n"
           "  - 'get', 'download' - Downloads a file from server.\n"
           "         example: `cfrds get rds://username:password@host/pathname local_file`\n"
           "            or    `cfrds download rds://username:password@host/pathname local_file`\n"
           "\n"
           "  - 'put', 'upload' - Uploads a file to server.\n"
           "         example: `cfrds put local_file rds://username:password@host/pathname`\n"
           "            or    `cfrds upload local_file rds://username:password@host/pathname`\n"
           "\n"
           "  - 'mv', 'move' - Moves/renames file or folder.\n"
           "         example: `cfrds mv rds://username:password@host/pathname {new_name}`\n"
           "            or    `cfrds move rds://username:password@host/pathname {new_name}`\n"
           "\n"
           "  - 'rm', 'delete' - Delete a file to server.\n"
           "         example: `cfrds rm rds://username:password@host/pathname`\n"
           "            or    `cfrds delete rds://username:password@host/pathname`\n"
           "\n"
           "  - 'mkdir' - Create a directory on a server.\n"
           "         example: `cfrds mkdir rds://username:password@host/pathname`\n"
           "\n"
           "  - 'rmdir' - Deletes a directory from a server.\n"
           "         example: `cfrds rmdir rds://username:password@host/pathname`\n"
           "\n"
           "  - 'cfroot' - Return ColdFusion installation directory.\n"
           "         example: `cfrds cfroot rds://username:password@host`\n"
           "\n"
           "  - 'dsninfo' - Return ColdFusion data sources.\n"
           "         example: `cfrds dsninfo rds://username:password@host`\n"
           "\n"
           "  - 'tableinfo' - Return ColdFusion datasource tables info.\n"
           "         example: `cfrds tableinfo rds://username:password@host/dsn`\n"
           "\n"
           "  - 'columninfo' - Return ColdFusion datasource table columns info.\n"
           "         example: `cfrds columninfo rds://username:password@host/dsn/table`\n"
           "\n"
           "  - 'primarykeys' - Return ColdFusion datasource table primary keys info.\n"
           "         example: `cfrds primarykeys rds://username:password@host/dsn/table`\n"
           "\n"
           "  - 'foreignkeys' - Return ColdFusion datasource table foreign keys info.\n"
           "         example: `cfrds foreignkeys rds://username:password@host/dsn/table`\n"
           "\n"
           "  - 'importedkeys' - Return ColdFusion datasource table imported keys info.\n"
           "         example: `cfrds importedkeys rds://username:password@host/dsn/table`\n"
           "\n"
           "  - 'exportedkeys' - Return ColdFusion datasource table exported keys info.\n"
           "         example: `cfrds exportedkeys rds://username:password@host/dsn/table`\n"
           "\n"
           "  - 'sql' - Executes SQL statement on ColdFusion data sources.\n"
           "         example: `cfrds sql rds://username:password@host/dsn` \"SELECT 1\"\n"
           "\n"
           "  - 'sqlmetadata' - Return SQL statement metadata on ColdFusion data sources.\n"
           "         example: `cfrds sqlmetadata rds://username:password@host/dsn` \"SELECT 1\"\n"
           "\n"
           "  - 'dbdescription' - Return ColdFusion data sources database info.\n"
           "         example: `cfrds dbdescription rds://username:password@host/dsn`\n"
           "\n"
           "  - 'dbg_start' - Start ColdFusion debugger session.\n"
           "         example: `cfrds dbg_start rds://username:password@host`\n"
           "\n"
           "  - 'dbg_stop' - Stops ColdFusion debugger session.\n"
           "         example: `cfrds dbg_start rds://username:password@host/dbg_session`\n"
           "\n"
           "  - 'dbg_info' - Get ColdFusion debugger server info.\n"
           "         example: `cfrds dbg_info rds://username:password@host`\n"
           "\n"
           "  - 'dbg_brk_on_exception' - Set ColdFusion debugger server to stop on exception.\n"
           "         example: `cfrds dbg_brk_on_exception rds://username:password@host/dbg_session` {true/false}\n"
           );
}

static bool init_server_from_uri(const char *uri, char **hostname, uint16_t *port, char **username, char **password, char **path)
{
    cfrds_str_defer(_hostname);
    cfrds_str_defer(_port_str);
    int _port = 80;
    cfrds_str_defer(_username);
    cfrds_str_defer(_password);
    cfrds_str_defer(_path);

    if (uri == nullptr)
        return false;
    if (strstr(uri, "rds://") != uri)
        return false;

    uri += 6;

    int uri_strlen = strlen(uri);

    const char *path_start = strchr(uri, '/');
    if (path_start) {
        int path_strlen = uri_strlen - (path_start - uri);
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
        if ((pass_start)&&(pass_start < login_start)) {
            int user_strlen = pass_start - uri;
            int pass_strlen = login_start - pass_start - 1;

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
            int user_strlen = login_start - uri;
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
        int host_strlen = port_start - uri;
        int port_strlen = path_start - port_start - 1;

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
        _port = tmp_port;
    } else {
        int host_strlen = path_start - uri;
        _hostname = malloc(host_strlen);
        if (!_hostname)
            return false;
        memcpy(_hostname, uri, host_strlen);
        _path[host_strlen] = '\0';
        _port = 80;
    }

    *hostname = _hostname; _hostname = nullptr;
    *port = (uint16_t)_port;
    *username = _username; _username = nullptr;
    *password = _password; _password = nullptr;
    *path = _path;         _path = nullptr;

    return true;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
    }
#endif
    cfrds_server_defer(server);
    cfrds_file_content *content = nullptr;
    enum cfrds_status res;

    const char *uri = nullptr;
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

    if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)||(strcmp(command, "dbg_brk_on_exception") == 0)) {
        if (argc < 4) {
            usage();
            return EXIT_FAILURE;
        }
        uri = argv[3];
    } else {
        uri = argv[2];
    }

    if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
    {
        fprintf(stderr, "init_server_from_uri FAILED!\n");
        return EXIT_FAILURE;
    }

    if (path == nullptr) {
        path = strdup("/");
    }

    if (username == nullptr) {
        username = strdup("");
    }

    if (password == nullptr) {
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

        size_t cnt = cfrds_buffer_browse_dir_count(dir);
        for (size_t c = 0; c < cnt; c++)
        {
            char kind = cfrds_buffer_browse_dir_item_get_kind(dir, c);
            const char *name = cfrds_buffer_browse_dir_item_get_name(dir, c);
            uint8_t permissions = cfrds_buffer_browse_dir_item_get_permissions(dir, c);
            size_t size = cfrds_buffer_browse_dir_item_get_size(dir, c);
            uint64_t modified = cfrds_buffer_browse_dir_item_get_modified(dir, c);

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

        int to_write = cfrds_buffer_file_content_get_size(content);
        ssize_t written = os_write_to_terminal(cfrds_buffer_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %m\n");
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
            fprintf(stderr, "open FAILED with error: %m\n");
            return EXIT_FAILURE;
        }

        int to_write = cfrds_buffer_file_content_get_size(content);
        ssize_t written = os_write(fd, cfrds_buffer_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %m\n");
            return EXIT_FAILURE;
        }
    } else if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        const char *src_fname = argv[2];
        size_t src_size = 0;
        void *buf = nullptr;

        buf = os_map(src_fname, &src_size);
        if ((buf == nullptr)&&(src_size > 0))
        {
            fprintf(stderr, "mmap FAILED with error: %m\n");
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
            fprintf(stderr, "rm/delete FAILED with error: %s\n", cfrds_server_get_error(server));
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

        size_t cnt = cfrds_buffer_sql_dsninfo_count(dsninfo);
        for(size_t c = 0; c < cnt; c++)
        {
            const char *item = cfrds_buffer_sql_dsninfo_item_get_name(dsninfo, c);
            puts(item);
        }
    } else if (strcmp(command, "tableinfo") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            cfrds_sql_tableinfo_defer(tableinfo);
            const char *dsn = path + 1;

            res = cfrds_command_sql_tableinfo(server, dsn, &tableinfo);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "tableinfo FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_buffer_sql_tableinfo_count(tableinfo);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *name = cfrds_buffer_sql_tableinfo_get_name(tableinfo, c);
                const char *type = cfrds_buffer_sql_tableinfo_get_type(tableinfo, c);
                printf("%s, %s\n", name, type);
            }
        } else {
            fprintf(stderr, "No schema name\n");
        }
    } else if (strcmp(command, "columninfo") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_columninfo_defer(columninfo);
                int tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == nullptr)
                {
                    fprintf(stderr, "malloc FAILED!\n");
                    return EXIT_FAILURE;
                }

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                table = strdup(table + 1);
                if (table == nullptr)
                {
                    fprintf(stderr, "strdup FAILED!\n");
                    return EXIT_FAILURE;
                }

                res = cfrds_command_sql_columninfo(server, schema, table, &columninfo);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "columninfo FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_buffer_sql_columninfo_count(columninfo);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *name = cfrds_buffer_sql_columninfo_get_name(columninfo, c);
                    const char *type = cfrds_buffer_sql_columninfo_get_typeStr(columninfo, c);
                    printf("%s, %s\n", name, type);
                }
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
        }
    } else if (strcmp(command, "primarykeys") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_primarykeys_defer(primarykeys);
                cfrds_str_defer(tablename);
                int tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == nullptr)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == nullptr)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_primarykeys(server, schema, tablename, &primarykeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "primarykeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_buffer_sql_primarykeys_count(primarykeys);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *col_name = cfrds_buffer_sql_primarykeys_get_catalog(primarykeys, c);
                    const char *col_owner = cfrds_buffer_sql_primarykeys_get_owner(primarykeys, c);
                    const char *col_table = cfrds_buffer_sql_primarykeys_get_table(primarykeys, c);
                    const char *col_column = cfrds_buffer_sql_primarykeys_get_column(primarykeys, c);
                    int col_key_sequence = cfrds_buffer_sql_primarykeys_get_key_sequence(primarykeys, c);

                    printf("name: '%s', owner: '%s', table: '%s', column: '%s', key_sequence: %d\n", col_name, col_owner, col_table, col_column, col_key_sequence);
                }
            } else {
                fprintf(stderr, "No table name\n");
            }
        } else {
            fprintf(stderr, "No schema name\n");
        }
    } else if (strcmp(command, "foreignkeys") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_foreignkeys_defer(foreignkeys);
                cfrds_str_defer(tablename);
                int tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == nullptr)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == nullptr)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_foreignkeys(server, schema, tablename, &foreignkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "foreignkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_buffer_sql_foreignkeys_count(foreignkeys);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *pk_catalog = cfrds_buffer_sql_foreignkeys_get_pkcatalog(foreignkeys, c);
                    const char *pk_owner = cfrds_buffer_sql_foreignkeys_get_pkowner(foreignkeys, c);
                    const char *pk_table = cfrds_buffer_sql_foreignkeys_get_pktable(foreignkeys, c);
                    const char *pk_column = cfrds_buffer_sql_foreignkeys_get_pkcolumn(foreignkeys, c);
                    const char *fk_catalog = cfrds_buffer_sql_foreignkeys_get_fkcatalog(foreignkeys, c);
                    const char *fk_owner = cfrds_buffer_sql_foreignkeys_get_fkowner(foreignkeys, c);
                    const char *fk_table = cfrds_buffer_sql_foreignkeys_get_fktable(foreignkeys, c);
                    const char *fk_column = cfrds_buffer_sql_foreignkeys_get_fkcolumn(foreignkeys, c);
                    int key_sequence = cfrds_buffer_sql_foreignkeys_get_key_sequence(foreignkeys, c);
                    int updaterule = cfrds_buffer_sql_foreignkeys_get_updaterule(foreignkeys, c);
                    int deleterule = cfrds_buffer_sql_foreignkeys_get_deleterule(foreignkeys, c);

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
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_importedkeys_defer(importedkeys);
                cfrds_str_defer(tablename);
                int tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == nullptr)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == nullptr)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_importedkeys(server, schema, tablename, &importedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "importedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_buffer_sql_importedkeys_count(importedkeys);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *pk_catalog = cfrds_buffer_sql_importedkeys_get_pkcatalog(importedkeys, c);
                    const char *pk_owner = cfrds_buffer_sql_importedkeys_get_pkowner(importedkeys, c);
                    const char *pk_table = cfrds_buffer_sql_importedkeys_get_pktable(importedkeys, c);
                    const char *pk_column = cfrds_buffer_sql_importedkeys_get_pkcolumn(importedkeys, c);
                    const char *fk_catalog = cfrds_buffer_sql_importedkeys_get_fkcatalog(importedkeys, c);
                    const char *fk_owner = cfrds_buffer_sql_importedkeys_get_fkowner(importedkeys, c);
                    const char *fk_table = cfrds_buffer_sql_importedkeys_get_fktable(importedkeys, c);
                    const char *fk_column = cfrds_buffer_sql_importedkeys_get_fkcolumn(importedkeys, c);
                    int key_sequence = cfrds_buffer_sql_importedkeys_get_key_sequence(importedkeys, c);
                    int updaterule = cfrds_buffer_sql_importedkeys_get_updaterule(importedkeys, c);
                    int deleterule = cfrds_buffer_sql_importedkeys_get_deleterule(importedkeys, c);

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
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *schema = path + 1;
            const char *table = strchr(schema, '/');
            if(table) {
                cfrds_sql_exportedkeys_defer(exportedkeys);
                cfrds_str_defer(tablename);
                int tmp_size = 0;
                cfrds_str_defer(tmp);

                tmp_size = table - schema;
                tmp = malloc(tmp_size + 1);
                if (tmp == nullptr)
                    return EXIT_FAILURE;

                memcpy(tmp, schema, tmp_size);
                tmp[tmp_size] = '\0';
                schema = tmp;

                tablename = strdup(table + 1);
                if (tablename == nullptr)
                    return EXIT_FAILURE;

                res = cfrds_command_sql_exportedkeys(server, schema, tablename, &exportedkeys);
                if (res != CFRDS_STATUS_OK)
                {
                    fprintf(stderr, "exportedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                    return EXIT_FAILURE;
                }

                size_t cnt = cfrds_buffer_sql_exportedkeys_count(exportedkeys);
                for(size_t c = 0; c < cnt; c++)
                {
                    const char *pk_catalog = cfrds_buffer_sql_exportedkeys_get_pkcatalog(exportedkeys, c);
                    const char *pk_owner = cfrds_buffer_sql_exportedkeys_get_pkowner(exportedkeys, c);
                    const char *pk_table = cfrds_buffer_sql_exportedkeys_get_pktable(exportedkeys, c);
                    const char *pk_column = cfrds_buffer_sql_exportedkeys_get_pkcolumn(exportedkeys, c);
                    const char *fk_catalog = cfrds_buffer_sql_exportedkeys_get_fkcatalog(exportedkeys, c);
                    const char *fk_owner = cfrds_buffer_sql_exportedkeys_get_fkowner(exportedkeys, c);
                    const char *fk_table = cfrds_buffer_sql_exportedkeys_get_fktable(exportedkeys, c);
                    const char *fk_column = cfrds_buffer_sql_exportedkeys_get_fkcolumn(exportedkeys, c);
                    int key_sequence = cfrds_buffer_sql_exportedkeys_get_key_sequence(exportedkeys, c);
                    int updaterule = cfrds_buffer_sql_exportedkeys_get_updaterule(exportedkeys, c);
                    int deleterule = cfrds_buffer_sql_exportedkeys_get_deleterule(exportedkeys, c);

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
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            cfrds_sql_resultset_defer(resultset);
            const char *schema = path + 1;

            const char *sql = argv[3];

            res = cfrds_command_sql_sqlstmnt(server, schema, sql, &resultset);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "exportedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            // TODO: Implement printing of SQL resultset
        }
    } else if (strcmp(command, "sqlmetadata") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            cfrds_sql_metadata_defer(metadata);
            const char *schema = path + 1;

            const char *sql = argv[3];

            res = cfrds_command_sql_sqlmetadata(server, schema, sql, &metadata);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "exportedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_buffer_sql_metadata_count(metadata);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *name  = cfrds_buffer_sql_metadata_get_name(metadata, c);
                const char *type  = cfrds_buffer_sql_metadata_get_type(metadata, c);
                const char *jtype = cfrds_buffer_sql_metadata_get_jtype(metadata, c);

                printf("name: '%s', type: '%s', jtype: '%s'\n", name, type, jtype);
            }
        }
    } else if (strcmp(command, "supportedcommands") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            cfrds_sql_supportedcommands_defer(supportedcommands);

            res = cfrds_command_sql_getsupportedcommands(server, &supportedcommands);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "exportedkeys FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }

            size_t cnt = cfrds_buffer_sql_supportedcommands_count(supportedcommands);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *supportedcommand = cfrds_buffer_sql_supportedcommands_get(supportedcommands, c);
                printf("%s\n", supportedcommand);
            }
        }
    } else if (strcmp(command, "dbdescription") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
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
    } else if (strcmp(command, "dbg_start") == 0) {
        cfrds_str_defer(dbg_session);

        res = cfrds_command_debugger_start(server, &dbg_session);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "dbg_start FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        printf("%s\n", dbg_session);
    } else if (strcmp(command, "dbg_stop") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            res = cfrds_command_debugger_stop(server, path);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "dbg_stop FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }
        }
    } else if (strcmp(command, "dbg_info") == 0) {
        uint16_t debuggerPort;
        cfrds_str_defer(dbg_session);

        res = cfrds_command_debugger_start(server, &dbg_session);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "dbg_start FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }

        res = cfrds_command_debugger_get_server_info(server, dbg_session, &debuggerPort);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_get_server_info FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
        }

        printf("%d\n", debuggerPort);

        res = cfrds_command_debugger_stop(server, dbg_session);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "dbg_stop FAILED with error: %s\n", cfrds_server_get_error(server));
            return EXIT_FAILURE;
        }
    } else if (strcmp(command, "dbg_brk_on_exception") == 0) {
        if ((path != nullptr)&&(strlen(path) > 1))
        {
            const char *strValue = argv[4];
            bool value;

            if (strcmp(strValue, "true") == 0)
                value = true;
            else if (strcmp(strValue, "false") == 0)
                value = false;
            else
            {
                fprintf(stderr, "Invalid value(%s) dbg_brk_on_exception command. Valid are 'true' or 'false'.\n", strValue);
                return EXIT_FAILURE;
            }

            res = cfrds_command_debugger_breakpoint_on_exception(server, path, value);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "dbg_brk_on_exception FAILED with error: %s\n", cfrds_server_get_error(server));
                return EXIT_FAILURE;
            }
        }
    } else {
    }

    return ret;
}
