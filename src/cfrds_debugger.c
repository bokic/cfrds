#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>
#include <internal/cfrds_int.h>
#include <internal/wddx.h>
#include <cfrds.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

void cfrds_debugger_event_free(cfrds_debugger_event *event)
{
    wddx_cleanup(&event);
}

cfrds_debugger_type cfrds_debugger_event_get_type(const cfrds_debugger_event *event)
{
    if (event == NULL)
    {
        return CFRDS_DEBUGGER_EVENT_UNKNOWN;
    }

    const char *event_name = wddx_get_string(event, "0,EVENT");

    if (event_name == NULL)
        return CFRDS_DEBUGGER_EVENT_UNKNOWN;

    if (strcmp(event_name, "CF_BREAKPOINT_SET") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT_SET;
    else if (strcmp(event_name, "BREAKPOINT") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_BREAKPOINT;
    else if (strcmp(event_name, "STEP") == 0)
        return CFRDS_DEBUGGER_EVENT_TYPE_STEP;

    return CFRDS_DEBUGGER_EVENT_UNKNOWN;
}

const char *cfrds_debugger_event_breakpoint_get_source(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,SOURCE");
}

int cfrds_debugger_event_breakpoint_get_line(const cfrds_debugger_event *event)
{
    return (int)wddx_get_number(event, "0,LINE", NULL);
}

const cfrds_variable *cfrds_debugger_event_breakpoint_get_scopes(const cfrds_debugger_event *event)
{
    return wddx_get_var(event, "0,SCOPES");
}

const char *cfrds_debugger_event_breakpoint_get_thread_name(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,THREAD");
}

const char *cfrds_debugger_event_breakpoint_set_get_pathname(const cfrds_debugger_event *event)
{
    return wddx_get_string(event, "0,CFML_PATH");
}

int cfrds_debugger_event_breakpoint_set_get_req_line(const cfrds_debugger_event *event)
{
    return (int)wddx_get_number(event, "0,REQ_LINE_NUM", NULL);
}

int cfrds_debugger_event_breakpoint_set_get_act_line(const cfrds_debugger_event *event)
{
    return (int)wddx_get_number(event, "0,ACTUAL_LINE_NUM", NULL);
}

int cfrds_debugger_event_get_scopes_count(const cfrds_debugger_event *event)
{
    return wddx_node_array_size(wddx_get_var(event, "0,SCOPES"));
}

const char *cfrds_debugger_event_get_scopes_item_name(const cfrds_debugger_event *event, size_t ndx)
{
    const char *ret = NULL;

    if (event == NULL)
        return NULL;

    const WDDX_NODE *struct_node = wddx_get_var(event, "0,SCOPES");
    if (struct_node == NULL)
        return NULL;

    wddx_node_struct_at(struct_node, ndx, &ret);

    return ret;
}

const WDDX_NODE *cfrds_debugger_event_get_scopes_item_value(const cfrds_debugger_event *event, size_t ndx)
{
    if (event == NULL)
        return NULL;

    const WDDX_NODE *struct_node = wddx_get_var(event, "0,SCOPES");
    if (struct_node == NULL)
        return NULL;


    return wddx_node_struct_at(struct_node, ndx, NULL);
}

int cfrds_debugger_event_get_threads_count(const cfrds_debugger_event *event)
{
    return wddx_node_array_size(wddx_get_var(event, "0,THREADS"));
}

const char *cfrds_debugger_event_get_threads_item_name(const cfrds_debugger_event *event, size_t ndx)
{
    char key[32];
    int n;

    if (event == NULL)
        return NULL;

    n = snprintf(key, sizeof(key), "0,THREADS,%zu,0", ndx);
    if (n < 0)
        return NULL;

    const WDDX_NODE *node = wddx_get_var(event, key);

    if (node == NULL)
        return NULL;

    if (wddx_node_type(node) != WDDX_STRING)
        return NULL;

    return wddx_node_string(node);
}

const char *cfrds_debugger_event_get_threads_item_state(const cfrds_debugger_event *event, size_t ndx)
{
    char key[32];
    int n;

    if (event == NULL)
        return NULL;

    n = snprintf(key, sizeof(key), "0,THREADS,%zu,1", ndx);
    if (n < 0)
        return NULL;

    const WDDX_NODE *node = wddx_get_var(event, key);

    if (node == NULL)
        return NULL;

    if (wddx_node_type(node) != WDDX_STRING)
        return NULL;

    return wddx_node_string(node);
}

int cfrds_debugger_event_get_watch_count(const cfrds_debugger_event *event)
{
    return wddx_node_array_size(wddx_get_var(event, "0,WATCH"));
}

const char *cfrds_debugger_event_get_watch_item(const cfrds_debugger_event *event, size_t ndx)
{
    if (event == NULL)
        return NULL;

    const WDDX_NODE *node = wddx_get_var(event, "0,WATCH");
    if (node == NULL)
        return NULL;

    if (wddx_node_type(node) == WDDX_STRUCT)
    {
        const char *ret = NULL;
        wddx_node_struct_at(node, ndx, &ret);
        return ret;
    }

    const WDDX_NODE *item = wddx_node_array_at(node, ndx);
    if (wddx_node_type(item) != WDDX_STRING)
        return NULL;

    return wddx_node_string(item);
}

int cfrds_debugger_event_get_cf_trace_count(const cfrds_debugger_event *event)
{
    return wddx_node_array_size(wddx_get_var(event, "0,CF_TRACE"));
}

const char *cfrds_debugger_event_get_cf_trace_item(const cfrds_debugger_event *event, size_t ndx)
{
    char key[32];
    int n;

    if (event == NULL)
        return NULL;

    n = snprintf(key, sizeof(key), "0,CF_TRACE,%zu", ndx);
    if (n < 0)
        return NULL;

    const WDDX_NODE *node = wddx_get_var(event, key);

    if (wddx_node_type(node) != WDDX_STRING)
        return NULL;

    return wddx_node_string(node);
}

int cfrds_debugger_event_get_java_trace_count(const cfrds_debugger_event *event)
{
    return wddx_node_array_size(wddx_get_var(event, "0,JAVA_TRACE"));
}

const char *cfrds_debugger_event_get_java_trace_item(const cfrds_debugger_event *event, size_t ndx)
{
    char key[32];
    int n;

    if (event == NULL)
        return NULL;

    n = snprintf(key, sizeof(key), "0,JAVA_TRACE,%zu", ndx);
    if (n < 0)
        return NULL;

    const WDDX_NODE *node = wddx_get_var(event, key);

    if (wddx_node_type(node) != WDDX_STRING)
        return NULL;

    return wddx_node_string(node);
}

cfrds_status cfrds_command_debugger_start(cfrds_server *server, cfrds_str *session_id)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "0,REMOTE_SESSION", true);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_START", wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *session_id = cfrds_buffer_to_debugger_start(response);
        if (*session_id == NULL)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_stop(cfrds_server *server, const char *session_id)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_STOP", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        if (cfrds_buffer_to_debugger_stop(response) == false)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_get_server_info(cfrds_server *server, const char *session_id, uint16_t *port)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(port == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_GET_DEBUG_SERVER_INFO", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        int val = cfrds_buffer_to_debugger_info(response);
        if ((val < 0) || (val > UINT16_MAX))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        *port = (uint16_t)val;
    }

    return ret;
}

cfrds_status cfrds_command_debugger_breakpoint_on_exception(cfrds_server *server, const char *session_id, bool value)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "0,BREAK_ON_EXCEPTION", value);
    wddx_put_string(wddx, "0,COMMAND", "SESSION_BREAK_ON_EXCEPTION");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        int val = cfrds_buffer_to_debugger_info(response);
        if (val == -1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

static bool cfrds_buffer_to_debugger_response_ok(cfrds_buffer *buffer)
{
    if (buffer == NULL)
        return false;

    const char *data = cfrds_buffer_data(buffer);
    size_t size = cfrds_buffer_data_size(buffer);
    int64_t rows = 0;

    if (!cfrds_buffer_parse_number(&data, &size, &rows))
        return false;

    /* 0: means empty void success response */
    if (rows == 0)
        return true;

    if (rows == 1)
    {
        cfrds_str_defer(xml);
        if (!cfrds_buffer_parse_string(&data, &size, &xml))
            return false;

        WDDX_defer(result);
        result = wddx_from_xml(xml);
        if (!result)
            return false;

        bool ok = false;
        double val = wddx_get_number(result, "0,VALUE", &ok);
        if (ok && val == -1)
            return false;

        return true;
    }

    return false;
}

cfrds_status cfrds_command_debugger_breakpoint(cfrds_server *server, const char *session_id, const char *filepath, int line, bool enable)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(filepath == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_number(wddx, "0,Y", line);
    if (enable)
        wddx_put_string(wddx, "0,COMMAND", "SET_BREAKPOINT");
    else
        wddx_put_string(wddx, "0,COMMAND", "UNSET_BREAKPOINT");
    wddx_put_string(wddx, "0,FILE", filepath);
    wddx_put_number(wddx, "0,SEQ", 1.0);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        if (!cfrds_buffer_to_debugger_response_ok(response))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_clear_all_breakpoints(cfrds_server *server, const char *session_id)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "UNSET_ALL_BREAKPOINTS");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        if (!cfrds_buffer_to_debugger_response_ok(response))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_get_debug_events(cfrds_server *server, const char *session_id, cfrds_debugger_event **event)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(event == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_EVENTS", session_id, NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *event = cfrds_buffer_to_debugger_event(response);
    }

    return ret;
}

cfrds_status cfrds_command_debugger_all_fetch_flags_enabled(cfrds_server *server, const char *session_id, bool threads, bool watch, bool scopes, bool cf_trace, bool java_trace, cfrds_debugger_event **event)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_bool(wddx, "THREADS", threads);
    wddx_put_bool(wddx, "WATCH", watch);
    wddx_put_bool(wddx, "SCOPES", scopes);
    wddx_put_bool(wddx, "CF_TRACE", cf_trace);
    wddx_put_bool(wddx, "JAVA_TRACE", java_trace);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_EVENTS", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        *event = cfrds_buffer_to_debugger_event(response);
    }

    return ret;
}

static cfrds_status cfrds_command_debugger_thread_action(cfrds_server *server, const char *session_id, const char *thread_name, const char *action)
{
    cfrds_status ret;
    cfrds_buffer_defer(response);

    if (server == NULL)
        return CFRDS_STATUS_SERVER_IS_NULL;

    if (session_id == NULL || thread_name == NULL)
        return CFRDS_STATUS_PARAM_IS_NULL;

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", action);
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        if (!cfrds_buffer_to_debugger_response_ok(response))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_step_in(cfrds_server *server, const char *session_id, const char *thread_name)
{
    return cfrds_command_debugger_thread_action(server, session_id, thread_name, "STEP_IN");
}

cfrds_status cfrds_command_debugger_step_over(cfrds_server *server, const char *session_id, const char *thread_name)
{
    return cfrds_command_debugger_thread_action(server, session_id, thread_name, "STEP_OVER");
}

cfrds_status cfrds_command_debugger_step_out(cfrds_server *server, const char *session_id, const char *thread_name)
{
    return cfrds_command_debugger_thread_action(server, session_id, thread_name, "STEP_OUT");
}

cfrds_status cfrds_command_debugger_continue(cfrds_server *server, const char *session_id, const char *thread_name)
{
    return cfrds_command_debugger_thread_action(server, session_id, thread_name, "CONTINUE");
}

cfrds_status cfrds_command_debugger_watch_expression(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,VARIABLE_NAME", variable);
    wddx_put_string(wddx, "0,COMMAND", "GET_SINGLE_CF_VARIABLE");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

cfrds_status cfrds_command_debugger_set_variable(cfrds_server *server, const char *session_id, const char *thread_name, const char *variable, const char *value)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,VARIABLE_VALUE", value);
    wddx_put_string(wddx, "0,VARIABLE_NAME", variable);
    wddx_put_string(wddx, "0,COMMAND", "SET_VARIABLE_VALUE");
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

cfrds_status cfrds_command_debugger_watch_variables(cfrds_server *server, const char *session_id, const char *variables)
{
    cfrds_status ret;
    char command[32];
    size_t index = 0;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(variables == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "SET_WATCH_VARIABLES");

    while(strlen(variables) > 0)
    {
        cfrds_str_defer(variable);

        const char *delimiter = strchr(variables, ',');
        if (delimiter == NULL)
        {
            variable = strdup(variables);
            if (variable == NULL) return CFRDS_STATUS_MEMORY_ERROR;
            variables += strlen(variables);
        } else {
            size_t len = (size_t)(delimiter - variables);
            if (len == 0)
            {
                variables++;
                continue;
            }
            variable = malloc(len + 1);
            if (variable == NULL) return CFRDS_STATUS_MEMORY_ERROR;
            memcpy(variable, variables, len);
            variable[len] = '\0';
            variables += len + 1;
        }

        if (strlen(variable) > 0)
        {
            snprintf(command, sizeof(command), "0,WATCH,%zu", index++);
            wddx_put_string(wddx, command, variable);
        }
    }

    if (index == 0)
        return CFRDS_STATUS_INVALID_INPUT_PARAMETER;

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}

cfrds_status cfrds_command_debugger_get_output(cfrds_server *server, const char *session_id, const char *thread_name, cfrds_str *output)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if ((session_id == NULL)||(thread_name == NULL)||(output == NULL))
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,COMMAND", "GET_OUTPUT");
    wddx_put_bool(wddx, "0,BODY_ONLY", true);
    wddx_put_string(wddx, "0,THREAD", thread_name);

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});
    if (ret == CFRDS_STATUS_OK)
    {
        cfrds_str_defer(xml);

        const char *response_data = cfrds_buffer_data(response);
        size_t response_size = cfrds_buffer_data_size(response);

        int64_t count = 0;
        if (!cfrds_buffer_parse_number(&response_data, &response_size, &count))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (count != 1)
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (!cfrds_buffer_parse_string(&response_data, &response_size, &xml))
        {
            server->error_code = -1;
            return CFRDS_STATUS_RESPONSE_ERROR;
        }

        if (strlen(xml) > 0)
        {
            WDDX_defer(result);
            result = wddx_from_xml(xml);
            if (!result)
            {
                server->error_code = -1;
                return CFRDS_STATUS_RESPONSE_ERROR;
            }

            const char *val_str = wddx_get_string(result, "0,VALUE");
            if (val_str)
            {
                *output = strdup(val_str);
            }
            else
            {
                *output = strdup("");
            }
        }
        else
        {
            *output = strdup("");
        }
    }

    return ret;
}

cfrds_status cfrds_command_debugger_set_scope_filter(cfrds_server *server, const char *session_id, const char *filter)
{
    cfrds_status ret;

    cfrds_buffer_defer(response);

    if (server == NULL)
    {
        return CFRDS_STATUS_SERVER_IS_NULL;
    }

    if (session_id == NULL)
    {
        return CFRDS_STATUS_PARAM_IS_NULL;
    }

    WDDX_defer(wddx);
    wddx = wddx_create();
    wddx_put_string(wddx, "0,FILTER", filter);
    wddx_put_string(wddx, "0,COMMAND", "SET_SCOPE_FILTER");

    ret = cfrds_send_command(server, &response, "DBGREQUEST", (const char *[]){ "DBG_REQUEST", session_id, wddx_to_xml(wddx), NULL});

    return ret;
}
