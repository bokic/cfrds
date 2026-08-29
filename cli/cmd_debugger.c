#include "cmd_common.h"

static bool parse_bool_arg(const char *arg, bool default_val)
{
    if (!arg) return default_val;
    if (strcmp(arg, "1") == 0 || strcasecmp(arg, "true") == 0 || strcasecmp(arg, "yes") == 0) return true;
    if (strcmp(arg, "0") == 0 || strcasecmp(arg, "false") == 0 || strcasecmp(arg, "no") == 0) return false;
    return default_val;
}

static void print_debugger_event(const cfrds_debugger_event *event, bool json_output)
{
    if (!event) return;
    if (json_output)
    {
        struct json_object *obj = json_object_new_object();
        json_object_object_add(obj, "status", json_object_new_string("success"));
        json_object_object_add(obj, "event_type", json_object_new_int(cfrds_debugger_event_get_type(event)));

        const char *source = cfrds_debugger_event_breakpoint_get_source(event);
        if (source) json_object_object_add(obj, "source", json_object_new_string(source));
        json_object_object_add(obj, "line", json_object_new_int(cfrds_debugger_event_breakpoint_get_line(event)));

        const char *thread_name = cfrds_debugger_event_breakpoint_get_thread_name(event);
        if (thread_name) json_object_object_add(obj, "thread_name", json_object_new_string(thread_name));

        const char *bp_pathname = cfrds_debugger_event_breakpoint_set_get_pathname(event);
        if (bp_pathname)
        {
            json_object_object_add(obj, "breakpoint_set_pathname", json_object_new_string(bp_pathname));
            json_object_object_add(obj, "breakpoint_set_req_line", json_object_new_int(cfrds_debugger_event_breakpoint_set_get_req_line(event)));
            json_object_object_add(obj, "breakpoint_set_act_line", json_object_new_int(cfrds_debugger_event_breakpoint_set_get_act_line(event)));
        }

        int scopes_count = cfrds_debugger_event_get_scopes_count(event);
        struct json_object *scopes_arr = json_object_new_array();
        for (int i = 0; i < scopes_count; i++)
        {
            const char *item = cfrds_debugger_event_get_scopes_item_name(event, (size_t)i);
            json_object_array_add(scopes_arr, json_object_new_string(item ? item : ""));
        }
        json_object_object_add(obj, "scopes", scopes_arr);

        int threads_count = cfrds_debugger_event_get_threads_count(event);
        struct json_object *threads_arr = json_object_new_array();
        for (int i = 0; i < threads_count; i++)
        {
            struct json_object *th = json_object_new_object();
            const char *tname = cfrds_debugger_event_get_threads_item_name(event, (size_t)i);
            const char *tstate = cfrds_debugger_event_get_threads_item_state(event, (size_t)i);
            json_object_object_add(th, "name", json_object_new_string(tname ? tname : ""));
            json_object_object_add(th, "state", json_object_new_string(tstate ? tstate : ""));
            json_object_array_add(threads_arr, th);
        }
        json_object_object_add(obj, "threads", threads_arr);

        int watch_count = cfrds_debugger_event_get_watch_count(event);
        struct json_object *watch_arr = json_object_new_array();
        for (int i = 0; i < watch_count; i++)
        {
            const char *item = cfrds_debugger_event_get_watch_item(event, (size_t)i);
            json_object_array_add(watch_arr, json_object_new_string(item ? item : ""));
        }
        json_object_object_add(obj, "watch", watch_arr);

        int cf_count = cfrds_debugger_event_get_cf_trace_count(event);
        struct json_object *cf_arr = json_object_new_array();
        for (int i = 0; i < cf_count; i++)
        {
            const char *item = cfrds_debugger_event_get_cf_trace_item(event, (size_t)i);
            json_object_array_add(cf_arr, json_object_new_string(item ? item : ""));
        }
        json_object_object_add(obj, "cf_trace", cf_arr);

        int java_count = cfrds_debugger_event_get_java_trace_count(event);
        struct json_object *java_arr = json_object_new_array();
        for (int i = 0; i < java_count; i++)
        {
            const char *item = cfrds_debugger_event_get_java_trace_item(event, (size_t)i);
            json_object_array_add(java_arr, json_object_new_string(item ? item : ""));
        }
        json_object_object_add(obj, "java_trace", java_arr);

        printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
        json_object_put(obj);
    }
    else
    {
        printf("event_type: %d\n", cfrds_debugger_event_get_type(event));
        const char *src = cfrds_debugger_event_breakpoint_get_source(event);
        if (src) printf("source: %s\n", src);
        printf("line: %d\n", cfrds_debugger_event_breakpoint_get_line(event));
        const char *tname = cfrds_debugger_event_breakpoint_get_thread_name(event);
        if (tname) printf("thread_name: %s\n", tname);

        int scopes_count = cfrds_debugger_event_get_scopes_count(event);
        printf("scopes (%d):\n", scopes_count);
        for (int i = 0; i < scopes_count; i++)
        {
            const char *item = cfrds_debugger_event_get_scopes_item_name(event, (size_t)i);
            printf("  - %s\n", item ? item : "(null)");
        }

        int threads_count = cfrds_debugger_event_get_threads_count(event);
        printf("threads (%d):\n", threads_count);
        for (int i = 0; i < threads_count; i++)
        {
            const char *tn = cfrds_debugger_event_get_threads_item_name(event, (size_t)i);
            const char *ts = cfrds_debugger_event_get_threads_item_state(event, (size_t)i);
            printf("  - %s: %s\n", tn ? tn : "(null)", ts ? ts : "(null)");
        }

        int watch_count = cfrds_debugger_event_get_watch_count(event);
        printf("watch (%d):\n", watch_count);
        for (int i = 0; i < watch_count; i++)
        {
            const char *item = cfrds_debugger_event_get_watch_item(event, (size_t)i);
            printf("  - %s\n", item ? item : "(null)");
        }

        int cf_count = cfrds_debugger_event_get_cf_trace_count(event);
        printf("cf_trace (%d):\n", cf_count);
        for (int i = 0; i < cf_count; i++)
        {
            const char *item = cfrds_debugger_event_get_cf_trace_item(event, (size_t)i);
            printf("  - %s\n", item ? item : "(null)");
        }
    }
}

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

        cfrds_debugger_event_defer(event1);
        res = cfrds_command_debugger_all_fetch_flags_enabled(server, debugger_session_id, true, true, true, true, true, &event1);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_all_fetch_flags_enabled FAILED with error: %s\n", cfrds_server_get_error(server));
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
        if (thread_name == NULL)
        {
            goto test_debugger_exit;
        }
        printf("thread_name: %s\n", thread_name);

        res = cfrds_command_debugger_watch_expression(server, debugger_session_id, thread_name, "arrayNew(1)");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_watch_expression FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_set_variable(server, debugger_session_id, thread_name, "VARIABLES.A", "200");
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_set_variable FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        const char *source = cfrds_debugger_event_breakpoint_get_source(event);
        printf("source: %s\n", source);

        int line = cfrds_debugger_event_breakpoint_get_line(event);
        printf("line: %d\n", line);

        int scopes_count = cfrds_debugger_event_get_scopes_count(event);
        printf("scopes_count: %d\n", scopes_count);
        for(int c = 0; c < scopes_count; c++)
        {
            const char *scopes_item_name = cfrds_debugger_event_get_scopes_item_name(event, (size_t)c);
            printf("scopes_item_name: %s\n", scopes_item_name ? scopes_item_name : "(null)");
        }

        int threads_count = cfrds_debugger_event_get_threads_count(event);
        printf("threads_count: %d\n", threads_count);
        for(int c = 0; c < threads_count; c++)
        {
            const char *threads_name = cfrds_debugger_event_get_threads_item_name(event, (size_t)c);
            const char *threads_state = cfrds_debugger_event_get_threads_item_state(event, (size_t)c);
            printf("threads: name: %s, state: %s\n", threads_name ? threads_name : "(null)", threads_state ? threads_state : "(null)");
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
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_step_in(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_step_in FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_step_out(server, debugger_session_id, thread_name);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_step_out FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_breakpoint(server, debugger_session_id, "/app/test.cfm", 3, false);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_breakpoint FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_clear_all_breakpoints(server, debugger_session_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_clear_all_breakpoints FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_continue(server, debugger_session_id, thread_name);
        thread_name = NULL;
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_continue FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        res = cfrds_command_debugger_stop(server, debugger_session_id);
        if (res != CFRDS_STATUS_OK)
        {
            fprintf(stderr, "cfrds_command_debugger_stop FAILED with error: %s\n", cfrds_server_get_error(server));
            goto test_debugger_exit;
        }

        return EXIT_SUCCESS;

test_debugger_exit:
        if (debugger_session_id)
        {
            if (thread_name)
            {
                cfrds_command_debugger_continue(server, debugger_session_id, thread_name);
            }
            cfrds_command_debugger_clear_all_breakpoints(server, debugger_session_id);
            cfrds_command_debugger_stop(server, debugger_session_id);
        }

        return EXIT_FAILURE;

    } else if (strcmp(command, "dbg_start") == 0 || strcmp(command, "debugger_start") == 0)
    {
        cfrds_str_defer(session_id);
        res = cfrds_command_debugger_start(server, &session_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_start FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "session_id", json_object_new_string(session_id ? session_id : ""));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("session_id: %s\n", session_id ? session_id : "");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_stop") == 0 || strcmp(command, "debugger_stop") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id required");
        }
        const char *session_id = argv[3];
        res = cfrds_command_debugger_stop(server, session_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_stop FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Debugger session %s stopped\n", session_id);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_server_stop") == 0 || strcmp(command, "debugger_server_stop") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id required");
        }
        const char *session_id = argv[3];
        res = cfrds_command_debugger_server_stop(server, session_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_server_stop FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Debugger server stopped for session %s\n", session_id);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_info") == 0 || strcmp(command, "debugger_get_server_info") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id required");
        }
        const char *session_id = argv[3];
        uint16_t port = 0;
        res = cfrds_command_debugger_get_server_info(server, session_id, &port);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_info FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "port", json_object_new_int(port));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("debugger_port: %u\n", (unsigned int)port);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_breakpoint") == 0 || strcmp(command, "debugger_breakpoint") == 0)
    {
        if (argc < 6)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id filepath line [enable]");
        }
        const char *session_id = argv[3];
        const char *filepath = argv[4];
        int line = atoi(argv[5]);
        bool enable = (argc >= 7) ? parse_bool_arg(argv[6], true) : true;

        res = cfrds_command_debugger_breakpoint(server, session_id, filepath, line, enable);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_breakpoint FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Breakpoint %s at %s:%d\n", enable ? "set" : "unset", filepath, line);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_clear_breakpoints") == 0 || strcmp(command, "debugger_clear_all_breakpoints") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id required");
        }
        const char *session_id = argv[3];
        res = cfrds_command_debugger_clear_all_breakpoints(server, session_id);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_clear_breakpoints FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("All breakpoints cleared for session %s\n", session_id);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_break_on_exception") == 0 || strcmp(command, "debugger_breakpoint_on_exception") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id [enable]");
        }
        const char *session_id = argv[3];
        bool enable = (argc >= 5) ? parse_bool_arg(argv[4], true) : true;

        res = cfrds_command_debugger_breakpoint_on_exception(server, session_id, enable);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_break_on_exception FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Session break on exception %s for session %s\n", enable ? "enabled" : "disabled", session_id);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_global_break_on_exception") == 0 || strcmp(command, "debugger_global_breakpoint_on_exception") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id [enable]");
        }
        const char *session_id = argv[3];
        bool enable = (argc >= 5) ? parse_bool_arg(argv[4], true) : true;

        res = cfrds_command_debugger_global_breakpoint_on_exception(server, session_id, enable);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_global_break_on_exception FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Global break on exception %s for session %s\n", enable ? "enabled" : "disabled", session_id);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_events") == 0 || strcmp(command, "debugger_get_debug_events") == 0)
    {
        if (argc < 4)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id [threads] [watch] [scopes] [cf_trace] [java_trace]");
        }
        const char *session_id = argv[3];
        bool threads = (argc >= 5) ? parse_bool_arg(argv[4], true) : true;
        bool watch = (argc >= 6) ? parse_bool_arg(argv[5], true) : true;
        bool scopes = (argc >= 7) ? parse_bool_arg(argv[6], true) : true;
        bool cf_trace = (argc >= 8) ? parse_bool_arg(argv[7], true) : true;
        bool java_trace = (argc >= 9) ? parse_bool_arg(argv[8], true) : true;

        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_all_fetch_flags_enabled(server, session_id, threads, watch, scopes, cf_trace, java_trace, &event);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_events FAILED");
        }

        print_debugger_event(event, json_output);
        return EXIT_SUCCESS;
    } else if (strcmp(command, "step_in") == 0 || strcmp(command, "dbg_step_in") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
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
        else
        {
            printf("Stepped in thread %s\n", thread_name);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "step_over") == 0 || strcmp(command, "dbg_step_over") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
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
        else
        {
            printf("Stepped over in thread %s\n", thread_name);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "step_out") == 0 || strcmp(command, "dbg_step_out") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
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
        else
        {
            printf("Stepped out in thread %s\n", thread_name);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "continue") == 0 || strcmp(command, "dbg_continue") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
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
        else
        {
            printf("Continued thread %s\n", thread_name);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_sync_step_in") == 0 || strcmp(command, "debugger_sync_step_in") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_sync_step_in(server, session_id, thread_name, &event);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_sync_step_in FAILED");
        }

        print_debugger_event(event, json_output);
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_sync_step_over") == 0 || strcmp(command, "debugger_sync_step_over") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_sync_step_over(server, session_id, thread_name, &event);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_sync_step_over FAILED");
        }

        print_debugger_event(event, json_output);
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_sync_step_out") == 0 || strcmp(command, "debugger_sync_step_out") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_sync_step_out(server, session_id, thread_name, &event);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_sync_step_out FAILED");
        }

        print_debugger_event(event, json_output);
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_get_cf_variables") == 0 || strcmp(command, "debugger_get_cf_variables") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        cfrds_debugger_event_defer(event);
        res = cfrds_command_debugger_get_cf_variables(server, session_id, thread_name, &event);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_get_cf_variables FAILED");
        }

        print_debugger_event(event, json_output);
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_watch_expression") == 0 || strcmp(command, "debugger_watch_expression") == 0)
    {
        if (argc < 6)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name expression");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        const char *expression = argv[5];
        res = cfrds_command_debugger_watch_expression(server, session_id, thread_name, expression);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_watch_expression FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Watch expression configured for thread %s: %s\n", thread_name, expression);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_set_variable") == 0 || strcmp(command, "debugger_set_variable") == 0)
    {
        if (argc < 7)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name variable value");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        const char *variable = argv[5];
        const char *value = argv[6];
        res = cfrds_command_debugger_set_variable(server, session_id, thread_name, variable, value);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_set_variable FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Variable %s set to %s in thread %s\n", variable, value, thread_name);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_watch_variables") == 0 || strcmp(command, "debugger_watch_variables") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id variables");
        }
        const char *session_id = argv[3];
        const char *variables = argv[4];
        res = cfrds_command_debugger_watch_variables(server, session_id, variables);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_watch_variables FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Watch variables updated for session %s: %s\n", session_id, variables);
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_get_output") == 0 || strcmp(command, "debugger_get_output") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id thread_name");
        }
        const char *session_id = argv[3];
        const char *thread_name = argv[4];
        cfrds_str_defer(output);
        res = cfrds_command_debugger_get_output(server, session_id, thread_name, &output);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_get_output FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            json_object_object_add(obj, "output", json_object_new_string(output ? output : ""));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("thread_output:\n%s\n", output ? output : "");
        }
        return EXIT_SUCCESS;
    } else if (strcmp(command, "dbg_set_scope_filter") == 0 || strcmp(command, "debugger_set_scope_filter") == 0)
    {
        if (argc < 5)
        {
            HANDLE_ERROR(CFRDS_STATUS_INVALID_INPUT_PARAMETER, "Not enough arguments: session_id filter");
        }
        const char *session_id = argv[3];
        const char *filter = argv[4];
        res = cfrds_command_debugger_set_scope_filter(server, session_id, filter);
        if (res != CFRDS_STATUS_OK)
        {
            HANDLE_SERVER_ERROR(res, "dbg_set_scope_filter FAILED");
        }

        if (json_output)
        {
            struct json_object *obj = json_object_new_object();
            json_object_object_add(obj, "status", json_object_new_string("success"));
            printf("%s\n", json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PRETTY));
            json_object_put(obj);
        }
        else
        {
            printf("Scope filter updated for session %s: %s\n", session_id, filter);
        }
        return EXIT_SUCCESS;
    }

    (void)argc;
    return -1;
}

