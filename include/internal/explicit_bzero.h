#pragma once

#include <string.h>


#if defined(__APPLE__) || defined(_WIN32)
/**
 * @brief Volatile-based implementation of explicit_bzero for platforms without standard support (macOS/Windows).
 * 
 * Securely clears a memory area by writing zero bytes to it. It uses a volatile pointer 
 * to guarantee that the compiler's optimizer does not remove the write operations 
 * (which could otherwise happen if the buffer is not read from again before being freed or going out of scope).
 * 
 * @param s Pointer to the start of the memory block to be zeroed.
 * @param n Size of the memory block in bytes.
 */
static void explicit_bzero(void *s, size_t n) {
    volatile unsigned char *ptr = (volatile unsigned char *)s;
    while (n--) {
        *ptr++ = 0;
    }
}
#endif
