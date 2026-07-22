#pragma once

#include <cfrds.h>


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
void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error);
