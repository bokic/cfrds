#pragma once

#include "../cfrds.h"
#include "cfrds_buffer.h"

#include <stdbool.h>


enum cfrds_status cfrds_http_post(cfrds_server_int *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response);
void cfrds_sock_shutdown(cfrds_socket sock);
