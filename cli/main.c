#define PCRE2_CODE_UNIT_WIDTH 8

#include <cfrds.h>

#include <pcre2.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
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
           );
}

static bool init_server_from_uri(const unsigned char *uri, char **hostname, uint16_t *port, char **username, char **password, char **path)
{
    char *_hostname = NULL;
    char *_port_str = NULL;
    uint16_t _port = 80;
    char *_username = NULL;
    char *_password = NULL;
    char *_path = NULL;

    PCRE2_SIZE erroroffset = 0;
    int errornumber = 0;
    pcre2_code *re = NULL;
    pcre2_match_data *match_data = NULL;
    PCRE2_SIZE *ovector = NULL;
    int rc = 0;

    if ((uri == NULL)||(hostname == NULL)||(port == NULL)||(username == NULL)||(password == NULL)||(path == NULL))
        return false;

    re = pcre2_compile((PCRE2_SPTR8)R"(^rds:\/\/(?<username>.+):(?<password>.+)@(?<hostname>(?:(?:[0-9]{1,3}\.){3}[0-9]{1,3})|(?:[a-z_\-\.])+(?:[a-z_\-\.])*)(?::(?<port>[1-9]{1}[0-9]{0,4}))?(?<path>\/[0-9a-z_\-\/\.]*)?$)", PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errornumber, &erroroffset, NULL);
    if (re == NULL)
    {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", erroroffset, buffer);
        goto error;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    if (match_data == NULL)
    {
        fprintf(stderr, "pcre2_match_data_create_from_pattern FAILED!\n");
        goto error;
    }

    rc = pcre2_match(
        re,                        /* the compiled pattern */
        uri,                       /* the subject string */
        strlen((const char *)uri), /* the length of the subject */
        0,                         /* start at offset 0 in the subject */
        0,                         /* default options */
        match_data,                /* block for storing the result */
        NULL);                     /* use default match context */
    if (rc < 0)
    {
        fprintf(stderr, "pcre2_match FAILED!\n");
        goto error;
    }

    ovector = pcre2_get_ovector_pointer(match_data);
    if (ovector == NULL)
    {
        fprintf(stderr, "pcre2_get_ovector_pointer FAILED!\n");
        goto error;
    }

    int index = 0;
    PCRE2_SPTR substring_start = NULL;
    size_t substring_length = 0;

    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR) "hostname");
    substring_start = uri + ovector[2*index];
    substring_length = ovector[2*index+1] - ovector[2*index];
    _hostname = malloc(substring_length + 1);
    memcpy(_hostname, substring_start, substring_length);
    _hostname[substring_length] = '\0';

    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR) "port");
    substring_start = uri + ovector[2*index];
    substring_length = ovector[2*index+1] - ovector[2*index];
    if (substring_length > 0)
    {
        _port_str = malloc(substring_length + 1);
        memcpy(_port_str, substring_start, substring_length);
        _port_str[substring_length] = '\0';
        _port = atoi(_port_str);
        free(_port_str); _port_str = NULL;
    }

    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR) "username");
    substring_start = uri + ovector[2*index];
    substring_length = ovector[2*index+1] - ovector[2*index];
    _username = malloc(substring_length + 1);
    memcpy(_username, substring_start, substring_length);
    _username[substring_length] = '\0';

    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR) "password");
    substring_start = uri + ovector[2*index];
    substring_length = ovector[2*index+1] - ovector[2*index];
    _password = malloc(substring_length + 1);
    memcpy(_password, substring_start, substring_length);
    _password[substring_length] = '\0';

    index = pcre2_substring_number_from_name(re, (PCRE2_SPTR) "path");
    substring_start = uri + ovector[2*index];
    substring_length = ovector[2*index+1] - ovector[2*index];
    _path = malloc(substring_length + 1);
    memcpy(_path, substring_start, substring_length);
    _path[substring_length] = '\0';

    if (match_data) pcre2_match_data_free(match_data);
    if (re) pcre2_code_free(re);

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

    if (match_data) pcre2_match_data_free(match_data);
    if (re) pcre2_code_free(re);

    return false;
}


int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;
    cfrds_server *server = NULL;
    cfrds_browse_dir *dir = NULL;
    cfrds_file_content *content = NULL;
    enum cfrds_status res;

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
        const unsigned char *uri = (const unsigned char *)argv[3];
        if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
        {
            fprintf(stderr, "init_server_from_uri FAILED!\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
    } else {
        const unsigned char *uri = (const unsigned char *)argv[2];
        if (init_server_from_uri(uri, &hostname, &port, &username, &password, &path) == false)
        {
            fprintf(stderr, "init_server_from_uri FAILED!\n");
            ret = EXIT_FAILURE;
            goto exit;
        }
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
