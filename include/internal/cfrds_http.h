#pragma once

#include <cfrds.h>
#include "cfrds_buffer.h"

#include <stdbool.h>


/**
 * @brief Sends an HTTP POST request to the target RDS server and retrieves the response.
 * 
 * Formats and sends a custom HTTP POST request with "compatible; Macromedia RDS Client" User-Agent.
 * Resolves the server host, connects to it (establishing a TCP connection via socket), sets send/receive timeouts
 * to 30 seconds, and transmits the request headers and payload. Then, it reads the HTTP response, verifies the 
 * status code is 200 (supporting HTTP/1.0 and HTTP/1.1), skips the HTTP headers, parses the response-specific RDS error 
 * code, and sets server-level errors if any failures occur during host resolution, connection, socket IO, or status parsing.
 * 
 * @param server Pointer to the `cfrds_server` containing connection configurations (host, port, etc.) and error state.
 * @param command The API action string appended to the URL (e.g. `ACTION=command`).
 * @param payload The `cfrds_buffer` representing the POST body data to be transmitted.
 * @param response Output pointer where a pointer to the parsed HTTP response body `cfrds_buffer` is returned.
 *                 Must be freed by the caller via `cfrds_buffer_free` on success. Ignored if NULL.
 * @return `cfrds_status` indicating success (`CFRDS_STATUS_OK`) or specific failure code:
 *         - `CFRDS_STATUS_MEMORY_ERROR` on allocation/snprintf failure.
 *         - `CFRDS_STATUS_SOCKET_HOST_NOT_FOUND` on name resolution failure.
 *         - `CFRDS_STATUS_CONNECTION_TO_SERVER_FAILED` on TCP connection or socket timeout configuration failure.
 *         - `CFRDS_STATUS_WRITING_TO_SOCKET_FAILED` or `CFRDS_STATUS_READING_FROM_SOCKET_FAILED` on socket transmission errors.
 *         - `CFRDS_STATUS_RESPONSE_TOO_LARGE` if the response exceeds `CFRDS_MAX_RESPONSE_SIZE` (100MB).
 *         - `CFRDS_STATUS_RESPONSE_ERROR` if the HTTP status is not 200 or parsing error code fails or RDS error code indicates failure (< 0).
 *         - `CFRDS_STATUS_HTTP_RESPONSE_NOT_FOUND` if the header end separator `\r\n\r\n` cannot be found.
 */
cfrds_status cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response);
