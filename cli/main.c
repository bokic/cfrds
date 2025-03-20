#include <cfrds.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <regex.h>
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
    uint16_t _port = 80;
    char *_username = NULL;
    char *_password = NULL;
    char *_path = NULL;

    int rc = 0;

    if ((uri == NULL)||(hostname == NULL)||(port == NULL)||(username == NULL)||(password == NULL)||(path == NULL))
        return false;

    regex_t regex;

    // -std=gnu11 GCC >=4.7.1, clang 19.1, MinGW gcc(11.3 - 13.1)
    rc = regcomp(&regex, "^rds:\\/\\/(.+):(.+)@([a-z0-9\\.\\-_]+):?([0-9]{1,5})?\\/?(.*)$", REG_EXTENDED | REG_ICASE);
    if (rc != REG_NOERROR)
    {
        fprintf(stderr, "regcomp compilation failed\n");
        goto error;
    }

    size_t     nmatch = 6;
    regmatch_t pmatch[6] = {0};

    rc = regexec(&regex, uri, nmatch, pmatch, 0);

    regfree(&regex);

    if (rc != REG_NOERROR)
    {
        fprintf(stderr, "Invalid URL!\n");
        goto error;
    }

    if (pmatch[1].rm_eo > pmatch[1].rm_so)
    {
        _username = malloc(pmatch[1].rm_eo - pmatch[1].rm_so + 1);
        _username[pmatch[1].rm_eo - pmatch[1].rm_so] = '\0';
        memcpy(_username, uri + pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so);
    }

    if (pmatch[2].rm_eo > pmatch[2].rm_so)
    {
        _password = malloc(pmatch[2].rm_eo - pmatch[2].rm_so + 1);
        _password[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
        memcpy(_password, uri + pmatch[2].rm_so, pmatch[2].rm_eo - pmatch[2].rm_so);
    }

    if (pmatch[3].rm_eo > pmatch[3].rm_so)
    {
        _hostname = malloc(pmatch[3].rm_eo - pmatch[3].rm_so + 1);
        _hostname[pmatch[3].rm_eo - pmatch[3].rm_so] = '\0';
        memcpy(_hostname, uri + pmatch[3].rm_so, pmatch[3].rm_eo - pmatch[3].rm_so);
    }

    if (pmatch[4].rm_eo > pmatch[4].rm_so)
    {
        _port = atoi(uri + pmatch[4].rm_so);
    }

    if (pmatch[5].rm_eo > pmatch[5].rm_so)
    {
        _path = malloc(pmatch[5].rm_eo - pmatch[5].rm_so + 1);
        _path[pmatch[5].rm_eo - pmatch[5].rm_so] = '\0';
        memcpy(_path, uri + pmatch[5].rm_so, pmatch[5].rm_eo - pmatch[5].rm_so);
    }

    *hostname = _hostname;
    *port = _port;
    *username = _username;
    *password = _password;
    *path = _path;

    return true;

error:
    free(_hostname);
    free(_port_str);
    free(_username);
    free(_password);
    free(_path);

    regfree(&regex);

    return false;
}


int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
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
    int fd = -1;

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
        ssize_t written = write(1, cfrds_buffer_file_content_get_data(content), to_write);
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

        fd = open(dest_fname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (fd == -1)
        {
            fprintf(stderr, "open FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        int to_write = cfrds_buffer_file_content_get_size(content);
        ssize_t written = write(fd, cfrds_buffer_file_content_get_data(content), to_write);
        if (written != to_write)
        {
            fprintf(stderr, "write FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else if ((strcmp(command, "put") == 0)||(strcmp(command, "upload") == 0)) {
        const char *src_fname = argv[2];
        struct stat stat;

        fd = open(src_fname, O_RDONLY);
        if (fd == -1)
        {
            fprintf(stderr, "open FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        if (fstat(fd, &stat))
        {
            fprintf(stderr, "open FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        void *buf = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (buf == NULL)
        {
            fprintf(stderr, "mmap FAILED with error: %m\n");
            ret = EXIT_FAILURE;
            goto exit;
        }

        res = cfrds_command_file_write(server, path, buf, stat.st_size);
        if (res != CFRDS_STATUS_OK) {
            fprintf(stderr, "write FAILED with error: %s\n", cfrds_server_get_error(server));
        }

        munmap(buf, stat.st_size);
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



            cfrds_buffer_sql_tableinfo_free(tableinfo);
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
    if (fd != -1) close(fd);
    if (cfroot) free(cfroot);
    if (path) free(path);
    if (password) free(password);
    if (username) free(username);
    if (hostname) free(hostname);
    if (dir) cfrds_buffer_browse_dir_free(dir);
    if (server) cfrds_server_free(server);

    return ret;
}
