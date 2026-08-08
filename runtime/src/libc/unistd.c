#include <errno.h>
#include <stdint.h>
#include <norx/syscall.h>
#include <time.h>
#include <unistd.h>

static int norx_errno_storage;

int *__errno_location(void)
{
    return &norx_errno_storage;
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
    return norx_is_error(norx_sleep(seconds)) ? seconds : 0;
}

int nanosleep(const struct timespec *request, struct timespec *remainder)
{
    if (request == (const struct timespec *)0 || request->tv_sec < 0 || request->tv_nsec < 0) {
        errno = EINVAL;
        return -1;
    }
    if (remainder != (struct timespec *)0) {
        remainder->tv_sec = 0;
        remainder->tv_nsec = 0;
    }
    return (int)norx_result(norx_sleep((norx_word_t)request->tv_sec));
}
