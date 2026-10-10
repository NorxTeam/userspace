#include <stdatomic.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include <unistd.h>

int main(void)
{
    static const char marker[] = "[   OK   ] userspace: c-write\n";
    char buffer[8];
    atomic_uint_least64_t counter;
    mtx_t mutex = MTX_INIT;

    atomic_init(&counter, 1);
    if (atomic_fetch_add(&counter, 2) != 1 || atomic_load(&counter) != 3) {
        return 1;
    }
    if (memset(buffer, 'a', sizeof(buffer)) != buffer || strlen("norx") != 4) {
        return 2;
    }
    if (memcmp(buffer, "aaaaaaaa", sizeof(buffer)) != 0) {
        return 3;
    }
    if (mtx_lock(&mutex) != thrd_success || mtx_trylock(&mutex) != thrd_busy) {
        return 4;
    }
    mtx_unlock(&mutex);
    if (!thrd_equal(thrd_current(), thrd_current())) {
        return 5;
    }
    struct timespec invalid = {0, 1000000000LL};
    if (nanosleep(&invalid, 0) != -1 || errno != EINVAL) {
        return 7;
    }
    struct timespec zero = {0, 0};
    struct timespec remainder = {1, 1};
    if (nanosleep(&zero, &remainder) != 0 || remainder.tv_sec != 0 || remainder.tv_nsec != 0) {
        return 8;
    }
    if (write(1, marker, sizeof(marker) - 1) != (ssize_t)(sizeof(marker) - 1)) {
        return 6;
    }
    return 0;
}
