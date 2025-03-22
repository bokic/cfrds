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
           "  - 'tableinfo' - Return ColdFusion data sources.\n"
           "         example: `cfrds tableinfo rds://username:password@host/dns`\n"
           "\n"
           "  - 'sql' - Return ColdFusion data sources.\n"
           "         example: `cfrds sql \"SELECT 1\" rds://username:password@host/dns`\n"
           );
}

static bool init_server_from_uri(const char *uri, char **hostname, uint16_t *port, char **username, char **password, char **path)
{
    char *_hostname = NULL;
    char *_port_str = NULL;
    int _port = 80;
    char *_username = NULL;
    char *_password = NULL;
    char *_path = NULL;

    if (uri == NULL) goto error;
    if (strstr(uri, "rds://") != uri) goto error;

    uri += 6;

    int uri_strlen = strlen(uri);

    const char *path_start = strchr(uri, '/');
    if (path_start) {
        int path_strlen = uri_strlen - (path_start - uri);
        _path = malloc(path_strlen + 1);
        if (!_path) goto error;
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
                if (!_username) goto error;
                memcpy(_username, uri, user_strlen);
                _username[user_strlen] = '\0';
            }
            if (pass_strlen) {
                _password = malloc(pass_strlen + 1);
                if (!_password) goto error;
                memcpy(_password, pass_start + 1, pass_strlen);
                _password[pass_strlen] = '\0';
            }
        } else {
            int user_strlen = login_start - uri;
            _username = malloc(user_strlen + 1);
            if (!_username) goto error;
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
        if (!_hostname) goto error;
        memcpy(_hostname, uri, host_strlen);
        _hostname[host_strlen] = '\0';
        _port_str = malloc(port_strlen + 1);
        if (!_port_str) goto error;
        memcpy(_port_str, port_start + 1, port_strlen);
        _port_str[port_strlen] = '\0';

        long tmp_port = atol(_port_str);
        if ((tmp_port < 0x0000)||(tmp_port > 0xffff)) goto error;
        _port = tmp_port;
    } else {
        int host_strlen = path_start - uri;
        _hostname = malloc(host_strlen);
        if (!_hostname) goto error;
        memcpy(_hostname, uri, host_strlen);
        _path[host_strlen] = '\0';
        _port = 80;
    }

    *hostname = _hostname;
    *port = (uint16_t)_port;
    *username = _username;
    *password = _password;
    *path = _path;
    free(_port_str);

    return true;

error:
    if (_hostname) free(_hostname);
    if (_port_str) free(_port_str);
    if (_username) free(_username);
    if (_password) free(_password);
    if (_path) free(_path);

    return false;
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
    cfrds_server *server = NULL;
    cfrds_browse_dir *dir = NULL;
    cfrds_file_content *content = NULL;
    enum cfrds_status res;

    const char *uri = NULL;
    char *hostname = NULL;
    uint16_t port = 80;
    char *username = NULL;
    char *password = NULL;
    char *path = NULL;
    char *cfroot = NULL;
    file_hnd_fd fd = 0;

    if (argc < 3) {
        usage();
        ret = EXIT_FAILURE;
        goto exit;
    }

    const char *command = argv[1];

    if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        if (argc < 4) {
            usage();
            ret = EXIT_FAILURE;
            goto exit;
        }
        uri = argv[3];
    } else {
        uri = argv[2];
    }

    if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
    {
        fprintf(stderr, "init_server_from_uri FAILED!\n");
        ret = EXIT_FAILURE;
        goto exit;
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
        ret = EXIT_FAILURE;
        goto exit;
    }

    if ((strcmp(command, "ls") == 0)||(strcmp(command, "dir") == 0)) {
        res = cfrds_command_browse_dir(server, path, &dir);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "ls/dir FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
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
            ret = EXIT_FAILURE;
            goto exit;
        }

        int to_write = cfrds_buffer_file_content_get_size(content);
        ssize_t written = os_write_to_terminal(cfrds_buffer_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if ((strcmp(command, "get") == 0)||(strcmp(command, "download") == 0)) {
        res = cfrds_command_file_read(server, path, &content);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "get/download FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }

        const char *dest_fname = argv[3];

        fd = os_creat_file(dest_fname);
        if (fd == ERROR_FILE_HND_FD)
        {
            fprintf(stderr, "open FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        int to_write = cfrds_buffer_file_content_get_size(content);
        ssize_t written = os_write(fd, cfrds_buffer_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        const char *src_fname = argv[2];
        size_t src_size = 0;
        void *buf = NULL;

        buf = os_map(src_fname, &src_size);
        if (buf == NULL)
        {
            fprintf(stderr, "mmap FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        res = cfrds_command_file_write(server, path, buf, src_size);
        if (res != CFRDS_STATUS_OK) {
            fprintf(stderr, "write FAILED with error: %s\n", cfrds_server_get_error(server));
        }        
        os_unmap(buf, src_size);
    } else if ((strcmp(command, "rm") == 0)||(strcmp(command, "delete") == 0)) {
        res = cfrds_command_file_remove_file(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "rm/delete FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if (strcmp(command, "mkdir") == 0) {
        res = cfrds_command_file_create_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "mkdir FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if (strcmp(command, "rmdir") == 0) {
        res = cfrds_command_file_remove_dir(server, path);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "rmdir FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if (strcmp(command, "cfroot") == 0) {
        res = cfrds_command_file_get_root_dir(server, &cfroot);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfroot FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }

        puts(cfroot);
    } else if (strcmp(command, "dsninfo") == 0) {
        cfrds_sql_dnsinfo *dnsinfo = NULL;
        res = cfrds_command_sql_dnsinfo(server, &dnsinfo);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "dsninfo FAILED with error: %s\n", cfrds_server_get_error(server));
            ret = EXIT_FAILURE;
            goto exit;
        }

        size_t cnt = cfrds_buffer_sql_dnsinfo_count(dnsinfo);
        for(size_t c = 0; c < cnt; c++)
        {
            const char *item = cfrds_buffer_sql_dnsinfo_item_get_name(dnsinfo, c);
            puts(item);
        }

        cfrds_buffer_sql_dnsinfo_free(dnsinfo);
    } else if (strcmp(command, "tableinfo") == 0) {
        if ((path != NULL)&&(strlen(path) > 1))
        {
            cfrds_sql_tableinfo *tableinfo = NULL;
            const char *dns = path + 1;

            res = cfrds_command_sql_tableinfo(server, dns, &tableinfo);
            if (res != CFRDS_STATUS_OK)
            {
                fprintf(stderr, "tableinfo FAILED with error: %s\n", cfrds_server_get_error(server));
                ret = EXIT_FAILURE;
                goto exit;
            }

            size_t cnt = cfrds_buffer_sql_tableinfo_count(tableinfo);
            for(size_t c = 0; c < cnt; c++)
            {
                const char *name = cfrds_buffer_sql_tableinfo_field(tableinfo, c, 2);
                const char *type = cfrds_buffer_sql_tableinfo_field(tableinfo, c, 3);
                printf("%s, %s\n", name, type);
            }

            cfrds_buffer_sql_tableinfo_free(tableinfo);
        } else {
            fprintf(stderr, "No schema name\n");
        }
    } else if (strcmp(command, "columninfo") == 0) {
    } else if (strcmp(command, "primarykeys") == 0) {
    } else if (strcmp(command, "foreignkeys") == 0) {
    } else if (strcmp(command, "importedkeys") == 0) {
    } else if (strcmp(command, "exportedkeys") == 0) {
    } else if (strcmp(command, "sql") == 0) {
    } else if (strcmp(command, "sqlmetadata") == 0) {
    } else if (strcmp(command, "supportedcommands") == 0) {
    } else if (strcmp(command, "dbdescription") == 0) {
    }

exit:
    if (fd != ERROR_FILE_HND_FD) os_file_close(fd);
    if (cfroot) free(cfroot);
    if (path) free(path);
    if (password) free(password);
    if (username) free(username);
    if (hostname) free(hostname);
    if (dir) cfrds_buffer_browse_dir_free(dir);
    if (server) cfrds_server_free(server);

#ifdef _WIN32
    WSACleanup();
#endif

    return ret;
}
