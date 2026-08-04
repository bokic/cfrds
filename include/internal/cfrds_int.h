#pragma once

#include <cfrds.h>
#include <internal/cfrds_buffer.h>
#include <internal/cfrds_http.h>

/**
 * @brief Sets the error state of a cfrds_server instance.
 * 
 * Sets the numeric error code of the given `cfrds_server` and updates its string error message.
 * It will free any previous error string stored in the server instance before allocating and 
 * copying the new one (using `strdup`).
 * 
 * @param server Pointer to the `cfrds_server` instance whose error state is to be updated.
 *               If NULL, the function does nothing and returns immediately.
 * @param error_code The numeric error code to assign to the server.
 * @param error A null-terminated string describing the error. The string is duplicated internally.
 */
EXPORT_CFRDS void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error);

/**
 * @brief Encodes a plaintext password for ColdFusion RDS protocol transmission.
 */
EXPORT_CFRDS char *cfrds_server_encode_password(const char *password);

/**
 * @brief Sends an RDS command with list of arguments.
 */
EXPORT_CFRDS cfrds_status cfrds_send_command(cfrds_server *server, cfrds_buffer **response, const char *command, const char *list[]);

typedef void *(*cfrds_sql_parser_fn)(cfrds_buffer *buffer);
EXPORT_CFRDS cfrds_status cfrds_execute_sql_cmd(cfrds_server *server, const char *params[], cfrds_sql_parser_fn parser, void **out_result);

