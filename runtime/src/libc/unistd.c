#include <errno.h>
#include <stdint.h>
#include <norx/syscall.h>
#include <time.h>
#include <unistd.h>

#define NORX_ERRNO_SLOTS 64u

struct norx_errno_slot {
    norx_word_t thread_id;
    int value;
};

static struct norx_errno_slot norx_errno_slots[NORX_ERRNO_SLOTS];
static int norx_errno_bootstrap;

int *__errno_location(void)
{
    const norx_word_t thread_id = norx_gettid();
    if (norx_is_error(thread_id)) {
        return &norx_errno_bootstrap;
    }

    const unsigned int slot = (unsigned int)(thread_id & 0xffffu);
    if (slot == 0 || slot > NORX_ERRNO_SLOTS) {
        return &norx_errno_bootstrap;
    }

    struct norx_errno_slot *storage = &norx_errno_slots[slot - 1u];
    if (storage->thread_id != thread_id) {
        storage->thread_id = thread_id;
        storage->value = 0;
    }
    return &storage->value;
}

static long long norx_result(norx_word_t result)
{
    if (norx_is_error(result)) {
        errno = norx_errno_value(result);
        return -1;
    }
    return (long long)result;
}

ssize_t read(int fd, void *buffer, size_t length)
{
    return (ssize_t)norx_result(norx_read((norx_word_t)fd, buffer, length));
}

ssize_t write(int fd, const void *buffer, size_t length)
{
    return (ssize_t)norx_result(norx_write((norx_word_t)fd, buffer, length));
}

int close(int fd)
{
    return (int)norx_result(norx_close((norx_word_t)fd));
}

pid_t getpid(void)
{
    return (pid_t)norx_result(norx_getpid());
}

pid_t gettid(void)
{
    return (pid_t)norx_result(norx_gettid());
}

int sched_yield(void)
{
    return (int)norx_result(norx_yield());
}

unsigned int sleep(unsigned int seconds)
{
    const norx_word_t result = norx_sleep(seconds);
    if (norx_is_error(result)) {
        errno = norx_errno_value(result);
        return seconds;
    }
    return 0;
}

int nanosleep(const struct timespec *request, struct timespec *remainder)
{
    if (request == (const struct timespec *)0) {
        errno = EINVAL;
        return -1;
    }
    const struct timespec requested = *request;
    if (requested.tv_sec < 0 || requested.tv_nsec < 0 || requested.tv_nsec >= 1000000000LL) {
        errno = EINVAL;
        return -1;
    }
    return (int)norx_result(norx_nanosleep(
        requested.tv_sec, requested.tv_nsec, (norx_timespec_t *)remainder));
}
