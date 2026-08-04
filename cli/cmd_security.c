#include "cmd_common.h"

int handle_cmd_security(cfrds_server *server, const char *command, const char *path)
{
    cfrds_status res;

    if (strcmp(command, "security_analyzer") == 0) {
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
        return EXIT_SUCCESS;
    }

    return -1;
}
