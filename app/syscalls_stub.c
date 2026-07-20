#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ti_msp_dl_config.h"

#define C_HEAP_MAIN_STACK_GUARD (4096U)

extern char __heap_start__;
extern char __StackTop;

void *_sbrk(ptrdiff_t increment)
{
    static uintptr_t heap_end;
    uintptr_t heap_start = (uintptr_t) &__heap_start__;
    uintptr_t heap_limit = (uintptr_t) &__StackTop - C_HEAP_MAIN_STACK_GUARD;
    uintptr_t current;
    uintptr_t next;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    current = (heap_end == 0U) ? heap_start : heap_end;

    if (increment >= 0) {
        uintptr_t increase = (uintptr_t) increment;
        if ((current > heap_limit) || (increase > (heap_limit - current))) {
            if (primask == 0U) {
                __enable_irq();
            }
            errno = ENOMEM;
            return (void *) -1;
        }
        next = current + increase;
    } else {
        uintptr_t decrease = (uintptr_t) (-(increment + 1)) + 1U;
        if ((current < heap_start) || (decrease > (current - heap_start))) {
            if (primask == 0U) {
                __enable_irq();
            }
            errno = ENOMEM;
            return (void *) -1;
        }
        next = current - decrease;
    }

    heap_end = next;
    if (primask == 0U) {
        __enable_irq();
    }
    return (void *) current;
}

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
