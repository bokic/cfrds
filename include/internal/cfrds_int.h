#ifndef __CFRDS_INT_H
#define __CFRDS_INT_H

#include "../cfrds.h"


#ifdef __cplusplus
extern "C"
{
#endif

void cfrds_server_set_error(cfrds_server *server, int64_t error_code, const char *error);

#ifdef __cplusplus
}
#endif

#endif //  __CFRDS_INT_H
