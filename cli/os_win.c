#include "os.h"

#include <Windows.h>
#include <Winbase.h>
#include <Fileapi.h>


file_hnd_fd os_creat_file(const char* pathname)
{
    return CreateFileA(pathname, GENERIC_WRITE, FILE_SHARE_READ, NULL, TRUNCATE_EXISTING, 0, NULL);
}

void os_file_close(file_hnd_fd hnd_fd)
{
    CloseHandle(hnd_fd);
}

ssize_t os_write(file_hnd_fd hnd_fd, const void *buffer, size_t len)
{
    ssize_t ret = 0;
    DWORD written = 0;

    if (WriteFile(hnd_fd, buffer, len, &written, NULL))
        ret = written;

    return ret;
}

void* os_map(const char *pathname, size_t* size)
{
	void* ret = NULL;

	HANDLE hnd = CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE map_hnd = CreateFileMapping(hnd, NULL, PAGE_READONLY, 0, 1, NULL);
    ret = MapViewOfFile(map_hnd, FILE_MAP_READ, 0, 0, 1);

    CloseHandle(map_hnd);
	CloseHandle(hnd);

	return ret;
}

void os_unmap(void* addr, size_t size)
{
	UnmapViewOfFile(addr);
}

ssize_t os_write_to_terminal(const void *buffer, size_t len)
{
    DWORD written = 0;

    HANDLE hnd = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (hnd != INVALID_HANDLE_VALUE) {
        WriteConsole(hnd, buffer, len, &written, NULL);
        CloseHandle(hnd);
    }

    return written;
}

void os_file_cleanup(void *fd) {
    HANDLE *hnd = (HANDLE *) fd;
    if (hnd) {
        CloseHandle(*hnd);
        *hnd = 0;
    }
}
