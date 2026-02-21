#pragma once

#include <string.h>


#if defined(__APPLE__) || defined(_WIN32)
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif
