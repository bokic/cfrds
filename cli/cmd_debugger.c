#include "cmd_common.h"

int handle_cmd_debugger(cfrds_server *server, const char *command, int argc, char *argv[])
{
    cfrds_status res;

    if (strcmp(command, "test_debugger") == 0)
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
    }

    (void)argc;
    return -1;
}
