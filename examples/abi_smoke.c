#include <norx/syscall.h>

int main(void)
{
    norx_word_t pid = norx_getpid();
    norx_word_t tid = norx_gettid();

    if (norx_is_error(pid) || norx_is_error(tid)) {
        norx_exit(1);
    }
    if (norx_is_error(norx_yield())) {
        norx_exit(2);
    }

    norx_exit(0);
    return 0;
}
