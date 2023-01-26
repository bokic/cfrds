#include <cfrds.h>

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>


int main()
{
    int ret = 0;
    cfrds_server *server = NULL;
    cfrds_buffer_browse_dir *dir = NULL;
    cfrds_buffer_file_content *file_content_buffer = NULL;
    enum cfrds_status res;
    bool ok = false;

    // ^rds:\/\/((?:(?:[0-9]{1,3}\.){3}[0-9]{1,3})|(?:[a-z_\-\.])+(?:[a-z_\-\.])*)(?::([1-9]{1}[0-9]{0,4}))?(\/[0-9a-z_\-\/\.]*)?$

    ok = cfrds_server_init(&server, "192.168.63.131", 8500, "admin", "admin");
    if (!ok)
    {
        fprintf(stderr, "Could not create cfrds_server\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_browse_dir(server, "/var/log", &dir);
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing browse dir command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }
    cfrds_buffer_browse_dir_free(dir);
    dir = NULL;

    //res = cfrds_read_file(server, "/var/log/vmware-network.log", &file_content_buffer);
    res = cfrds_read_file(server, "/opt/ColdFusion2021/cfusion/wwwroot/app/t.cfm", &file_content_buffer);
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing read file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }
    cfrds_buffer_file_content_free(file_content_buffer);
    file_content_buffer = NULL;

    const char *write_str = "Boro test 1 2 3\n";
    //res = cfrds_write_file(server, "/opt/ColdFusion2021/cfusion/wwwroot/app/t.cfm", write_str, strlen(write_str));
    res = cfrds_write_file(server, "/cfm/t.cfm", write_str, strlen(write_str));
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing write file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_rename(server, "/cfm/t.cfm", "/cfm/t-new.cfm");
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing write file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    bool exists = false;
    res = cfrds_exists(server, "/cfm/t-new.cfm", &exists);
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing exist file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_exists(server, "/cfm/t-new2.cfm", &exists);
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing exist file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_remove_file(server, "/cfm/t-new.cfm");
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing remove file command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_create_dir(server, "/cfm/new_dir");
    if ((res != CFRDS_STATUS_OK)&&(res != CFRDS_STATUS_DIR_ALREADY_EXISTS))
    {
        fprintf(stderr, "Error issuing create dir command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_remove_dir(server, "/cfm/new_dir");
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing remove dir command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

    res = cfrds_get_root_dir(server);
    if (res != CFRDS_STATUS_OK)
    {
        fprintf(stderr, "Error issuing get root dir command. Error: %s\n", cfrds_server_get_error(server));
        ret = EXIT_FAILURE;
        goto exit;
    }

exit:
    cfrds_server_free(server);
    server = NULL;

    return ret;
}
