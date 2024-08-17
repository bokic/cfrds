#ifndef __CFRDS_HTTP_H
#define __CFRDS_HTTP_H

#include "../cfrds.h"
#include "cfrds_buffer.h"

#include <stdbool.h>


#define cfrds_server void

enum cfrds_status cfrds_http_post(cfrds_server_int *server, const char *command, cfrds_buffer *payload, cfrds_buffer **response);

#endif //  __CFRDS_HTTP_H
