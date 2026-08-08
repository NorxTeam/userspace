#include <errno.h>
#include <threads.h>
#include <unistd.h>

thrd_t thrd_current(void)
{
    pid_t tid = gettid();
    return tid < 0 ? 0 : (thrd_t)tid;
}

int thrd_equal(thrd_t left, thrd_t right)
{
    return left == right;
}

int thrd_create(thrd_t *thread, thrd_start_t function, void *argument)
{
    (void)thread;
    (void)function;
    (void)argument;
    errno = ENOSYS;
    return thrd_error;
}

int thrd_join(thrd_t thread, int *result)
{
    (void)thread;
    (void)result;
    errno = ENOSYS;
    return thrd_error;
}
