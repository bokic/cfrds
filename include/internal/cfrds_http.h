#ifndef __CFRDS_HTTP_H
#define __CFRDS_HTTP_H

#include <internal/cfrds_buffer.h>

#include <stdbool.h>
#include <stdint.h>

#define cfrds_server void

cfrds_buffer *cfrds_http_post(cfrds_server *server, const char *command, cfrds_buffer *payload);

#endif //  __CFRDS_HTTP_H
