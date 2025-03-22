#include "os.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


file_hnd_fd os_creat_file(const char* pathname)
{
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
}

void os_file_close(file_hnd_fd hnd_fd)
{
    close(hnd_fd);
}

ssize_t os_write(file_hnd_fd hnd_fd, const void *buffer, size_t len)
{
    return write(hnd_fd, buffer, len);
}

void* os_map(const char *pathname, size_t* size)
{
    void* ret = NULL;
    struct stat stat;
    int fd = 0;

    fd = open(pathname, O_RDONLY);
    if (fd == -1) goto exit;

    if (fstat(fd, &stat)) goto exit;

    ret = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

exit:
    if(fd > 0) close(fd);

    return ret;
}

void os_unmap(void* addr, size_t size)
{
    munmap(addr, size);
}

ssize_t os_write_to_terminal(const void *buffer, size_t len)
{
    return write(STDOUT_FILENO, buffer, len);
}
