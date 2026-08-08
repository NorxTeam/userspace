#include <limits.h>
#include <stdint.h>
#include <norx/runtime.h>

extern int main(int argc, char **argv, char **environment);

int norx_start(uintptr_t *initial_stack)
{
    int argc = (int)initial_stack[0];
    char **argv = (char **)&initial_stack[1];
    char **environment = &argv[argc + 1];
    return main(argc, argv, environment);
}
