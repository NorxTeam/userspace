#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <norx/syscall.h>

_Noreturn void _Exit(int status)
{
    norx_exit(status);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

_Noreturn void exit(int status)
{
    _Exit(status);
}

_Noreturn void abort(void)
{
    _Exit(134);
}

int atexit(void (*function)(void))
{
    (void)function;
    return 0;
}

void *malloc(size_t size)
{
    (void)size;
    errno = ENOMEM;
    return (void *)0;
}

void *calloc(size_t count, size_t size)
{
    (void)count;
    (void)size;
    errno = ENOMEM;
    return (void *)0;
}

void *realloc(void *pointer, size_t size)
{
    (void)pointer;
    (void)size;
    errno = ENOMEM;
    return (void *)0;
}

void free(void *pointer)
{
    (void)pointer;
}
