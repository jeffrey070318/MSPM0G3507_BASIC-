#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

int _close(int file)
{
    (void) file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void) file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void) file;
    return 1;
}

off_t _lseek(int file, off_t offset, int whence)
{
    (void) file;
    (void) offset;
    (void) whence;
    errno = ESPIPE;
    return (off_t) -1;
}

ssize_t _read(int file, void *ptr, size_t len)
{
    (void) file;
    (void) ptr;
    (void) len;
    errno = EIO;
    return -1;
}

ssize_t _write(int file, const void *ptr, size_t len)
{
    (void) file;
    (void) ptr;
    return (ssize_t) len;
}
