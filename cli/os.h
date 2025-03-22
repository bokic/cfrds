#pragma once

#include <stddef.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define file_hnd_fd HANDLE
#define FILE_HND_FD_NULL NULL
#define ERROR_FILE_HND_FD NULL
typedef SIZE_T size_t;
typedef SSIZE_T ssize_t;
#else
#include <unistd.h>
#define file_hnd_fd int
#define FILE_HND_FD_NULL 0
#define ERROR_FILE_HND_FD -1
#endif

file_hnd_fd os_creat_file(const char* pathname);
void os_file_close(file_hnd_fd hnd_fd);

ssize_t os_write(file_hnd_fd hnd_fd, const void *buffer, size_t len);

void* os_map(const char* pathname, size_t* size);
void os_unmap(void *addr, size_t size);

ssize_t os_write_to_terminal(const void *buffer, size_t len);
