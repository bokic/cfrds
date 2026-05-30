#pragma once

#include <cfrds.h>
#include "cfrds_buffer.h"

#include <stdbool.h>


cfrds_status cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response);
