#include "cmd_common.h"

int handle_cmd_file(cfrds_server *server, const char *command, const char *path, int argc, char *argv[], cfrds_str *cfroot)
{
    (void)argc;
    cfrds_status res;
    cfrds_file_content_defer(content);

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

                char permissions_str[] = "------";
                if ((permissions & 0x10) || kind == 'D') permissions_str[0] = 'D';
                if (permissions & 0x01) permissions_str[1] = 'R';
                if (permissions & 0x02) permissions_str[2] = 'H';
                if (permissions & 0x04) permissions_str[3] = 'S';
                if (permissions & 0x20) permissions_str[4] = 'A';
                if (permissions & 0x80) permissions_str[5] = 'N';

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

                char permissions_str[] = "------";
                if ((permissions & 0x10) || kind == 'D') permissions_str[0] = 'D';
                if (permissions & 0x01) permissions_str[1] = 'R';
                if (permissions & 0x02) permissions_str[2] = 'H';
                if (permissions & 0x04) permissions_str[3] = 'S';
                if (permissions & 0x20) permissions_str[4] = 'A';
                if (permissions & 0x80) permissions_str[5] = 'N';

                const time_t timep = (time_t)(modified / 1000);
                const struct tm *newtime = localtime(&timep);
                char modified_str[64] = {0, };
                strftime(modified_str, sizeof(modified_str), "%c", newtime);

                printf("%s %12zu %s %s\n", permissions_str, size, modified_str, name);
            }
        }
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
    } else if (strcmp(command, "cfroot") == 0) {
        res = cfrds_command_file_get_root_dir(server, cfroot);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "cfroot FAILED with error");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "cfroot", json_object_new_string(*cfroot ? *cfroot : ""));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            puts(*cfroot);
        }
        return EXIT_SUCCESS;
    }

    return -1;
}
