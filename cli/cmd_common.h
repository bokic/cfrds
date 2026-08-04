#pragma once

#include <cfrds.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include <json.h>
#include "os.h"

extern bool json_output;

void print_json_error(cfrds_status res, const char *err_msg);
char *base64_encode(const unsigned char *data, size_t input_length);

#define HANDLE_ERROR(status_code, format_str, ...) \
    do { \
        char err_buf[1024]; \
        snprintf(err_buf, sizeof(err_buf), format_str, ##__VA_ARGS__); \
        if (json_output) { \
            print_json_error((status_code), err_buf); \
        } else { \
            fprintf(stderr, "%s\n", err_buf); \
        } \
        return EXIT_FAILURE; \
    } while(0)

#define HANDLE_SERVER_ERROR(status_code, prefix) \
    do { \
        const char *srv_err = cfrds_server_get_error(server); \
        HANDLE_ERROR((status_code), "%s: %s", (prefix), srv_err ? srv_err : ""); \
    } while(0)

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

int handle_cmd_file(cfrds_server *server, const char *command, const char *path, int argc, char *argv[], cfrds_str *cfroot);
int handle_cmd_sql(cfrds_server *server, const char *command, const char *path, int argc, char *argv[]);
int handle_cmd_debugger(cfrds_server *server, const char *command, int argc, char *argv[]);
int handle_cmd_security(cfrds_server *server, const char *command, const char *path);
