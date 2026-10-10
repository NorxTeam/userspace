#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <norx/syscall.h>

typedef void (*norx_cxa_function_t)(void *);
typedef void (*norx_atexit_function_t)(void);

typedef union {
    norx_cxa_function_t cxa;
    norx_atexit_function_t atexit;
} norx_exit_function_t;

typedef struct {
    norx_exit_function_t function;
    void *argument;
    void *dso;
    uint64_t sequence;
    unsigned char kind;
    unsigned char active;
} norx_exit_callback_t;

enum {
    NORX_EXIT_FUNCTION_CXA,
    NORX_EXIT_FUNCTION_ATEXIT,
};

/*
 * Norx has no thread-create or DSO-unload ABI yet. This is consequently a
 * process-local, single-threaded registry. Entries are removed before they
 * run, making nested finalization idempotent; registration during finalization
 * is rejected so a callback cannot make exit cleanup unbounded.
 */
static norx_exit_callback_t norx_exit_callbacks[NORX_ATEXIT_MAX_CALLBACKS];
static uint64_t norx_exit_next_sequence = 1;
static unsigned char norx_exit_finalizing;

static int norx_find_free_exit_callback(void)
{
    for (unsigned index = 0; index < NORX_ATEXIT_MAX_CALLBACKS; ++index) {
        if (!norx_exit_callbacks[index].active) {
            return (int)index;
        }
    }
    errno = ENOMEM;
    return -1;
}

static int norx_register_exit_callback(
    norx_exit_function_t function,
    unsigned char kind,
    void *argument,
    void *dso)
{
    if (norx_exit_finalizing) {
        errno = EBUSY;
        return -1;
    }
    if (norx_exit_next_sequence == UINT64_MAX) {
        errno = ENOMEM;
        return -1;
    }
    int index = norx_find_free_exit_callback();
    if (index < 0) {
        return -1;
    }
    norx_exit_callbacks[index] = (norx_exit_callback_t){
        .function = function,
        .argument = argument,
        .dso = dso,
        .sequence = norx_exit_next_sequence++,
        .kind = kind,
        .active = 1,
    };
    return 0;
}

int norx_atexit_register(norx_atexit_function_t function)
{
    if (function == (norx_atexit_function_t)0) {
        errno = EINVAL;
        return -1;
    }
    norx_exit_function_t callback = {.atexit = function};
    return norx_register_exit_callback(
        callback, NORX_EXIT_FUNCTION_ATEXIT, (void *)0, (void *)0);
}

int norx_cxa_atexit_register(norx_cxa_function_t function, void *argument, void *dso)
{
    if (function == (norx_cxa_function_t)0) {
        errno = EINVAL;
        return -1;
    }
    norx_exit_function_t callback = {.cxa = function};
    return norx_register_exit_callback(callback, NORX_EXIT_FUNCTION_CXA, argument, dso);
}

static int norx_latest_exit_callback(void *dso)
{
    int latest = -1;
    uint64_t latest_sequence = 0;
    for (unsigned index = 0; index < NORX_ATEXIT_MAX_CALLBACKS; ++index) {
        norx_exit_callback_t *callback = &norx_exit_callbacks[index];
        if (callback->active && (dso == (void *)0 || callback->dso == dso)
            && (latest < 0 || callback->sequence > latest_sequence)) {
            latest = (int)index;
            latest_sequence = callback->sequence;
        }
    }
    return latest;
}

void norx_cxa_finalize(void *dso)
{
    unsigned char was_finalizing = norx_exit_finalizing;
    norx_exit_finalizing = 1;
    for (;;) {
        int index = norx_latest_exit_callback(dso);
        if (index < 0) {
            break;
        }
        norx_exit_callback_t callback = norx_exit_callbacks[index];
        norx_exit_callbacks[index].active = 0;
        if (callback.kind == NORX_EXIT_FUNCTION_CXA) {
            callback.function.cxa(callback.argument);
        } else {
            callback.function.atexit();
        }
    }
    norx_exit_finalizing = was_finalizing;
}

_Noreturn void _Exit(int status)
{
    norx_exit(status);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

_Noreturn void exit(int status)
{
    norx_cxa_finalize((void *)0);
    _Exit(status);
}

_Noreturn void abort(void)
{
    _Exit(134);
}

int atexit(void (*function)(void))
{
    return norx_atexit_register(function);
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
