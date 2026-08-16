#include <stdint.h>

#include <norx/syscall.h>

static int same(const char *left, const char *right)
{
    while (*left && *left == *right) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static norx_word_t neg_errno(unsigned value)
{
    return (norx_word_t)0 - (norx_word_t)value;
}

static int read_exact(norx_word_t fd, void *buffer, norx_word_t length)
{
    for (unsigned attempt = 0; attempt < 1024; ++attempt) {
        norx_word_t result = norx_read(fd, buffer, length);
        if (result == length) {
            return 0;
        }
        if (!norx_is_error(result) || norx_errno_value(result) != NORX_EAGAIN) {
            return -1;
        }
        norx_yield();
    }
    return -1;
}

static int child_main(void)
{
    static const char request[] = "parent";
    static const char response[] = "child";
    char input[sizeof(request) - 1];

    if (norx_fcntl(3, NORX_F_GETFD, 0) != 0 || norx_fcntl(6, NORX_F_GETFD, 0) != 0) {
        return 20;
    }
    if (read_exact(3, input, sizeof(input)) != 0) {
        return 21;
    }
    for (unsigned index = 0; index < sizeof(input); ++index) {
        if (input[index] != request[index]) {
            return 22;
        }
    }
    if (norx_write(6, response, sizeof(response) - 1) != sizeof(response) - 1) {
        return 23;
    }
    return 0;
}

static int parent_main(void)
{
    static const char path[] = "/bin/spawn2-smoke";
    static const char child_argument[] = "child";
    static const char request[] = "parent";
    static const char response[] = "child";
    const char *arguments[] = {path, child_argument};
    norx_pipe_fds_t parent_to_child;
    norx_pipe_fds_t child_to_parent;
    norx_spawn_spec_t spec = {
        .path = (norx_pointer_t)path,
        .path_length = sizeof(path) - 1,
        .argv = (norx_pointer_t)arguments,
        .argc = 2,
        .environment = 0,
        .environment_count = 0,
        .stdin_fd = 0,
        .stdout_fd = 1,
        .stderr_fd = 2,
        .process_group = 0,
        .flags = NORX_SPAWN_INHERIT_OPEN_FDS,
    };
    norx_wait_status_t status;
    char input[sizeof(response) - 1];
    norx_word_t child;

    if (norx_pipe(&parent_to_child, 0) != 0 || norx_pipe(&child_to_parent, 0) != 0) {
        return 30;
    }
    if (parent_to_child.read != 3 || parent_to_child.write != 4
        || child_to_parent.read != 5 || child_to_parent.write != 6) {
        return 31;
    }
    if (norx_fcntl(parent_to_child.read, NORX_F_GETFD, 0) != 0
        || norx_fcntl(parent_to_child.write, NORX_F_GETFD, 0) != 0
        || norx_fcntl(child_to_parent.read, NORX_F_GETFD, 0) != 0
        || norx_fcntl(child_to_parent.write, NORX_F_GETFD, 0) != 0) {
        return 32;
    }
    if (norx_write(parent_to_child.write, request, sizeof(request) - 1)
        != sizeof(request) - 1) {
        return 33;
    }
    child = norx_spawn2(&spec);
    if (norx_is_error(child)) {
        return 34;
    }
    norx_close(parent_to_child.read);
    norx_close(parent_to_child.write);
    norx_close(child_to_parent.write);
    if (read_exact(child_to_parent.read, input, sizeof(input)) != 0) {
        return 35;
    }
    for (unsigned index = 0; index < sizeof(input); ++index) {
        if (input[index] != response[index]) {
            return 36;
        }
    }
    if (norx_wait_status(child, &status, 0) != child || status.kind != NORX_WAIT_EXITED
        || status.code != 0) {
        return 37;
    }

    spec.flags = 1ull << 4;
    if (norx_spawn2(&spec) != neg_errno(NORX_EINVAL)) {
        return 40;
    }
    spec.flags = NORX_SPAWN_INHERIT_OPEN_FDS;
    spec.argc = NORX_SPAWN_MAX_ARGUMENTS + 1;
    if (norx_spawn2(&spec) != neg_errno(NORX_E2BIG)) {
        return 41;
    }
    spec.argc = 2;
    spec.path = 0;
    if (norx_spawn2(&spec) != neg_errno(NORX_EFAULT)) {
        return 42;
    }
    spec.path = (norx_pointer_t)path;
    spec.path_length = 0;
    if (norx_spawn2(&spec) != neg_errno(NORX_EINVAL)) {
        return 43;
    }
    norx_close(parent_to_child.read);
    norx_close(parent_to_child.write);
    norx_close(child_to_parent.read);
    norx_close(child_to_parent.write);
    return 0;
}

int main(int argc, char **argv, char **environment)
{
    static const char marker[] = "[   OK   ] userspace: spawn2-fd-inherit\n";
    (void)environment;
    if (argc > 1 && same(argv[1], "child")) {
        return child_main();
    }
    if (parent_main() != 0) {
        return 1;
    }
    if (norx_write(1, marker, sizeof(marker) - 1) != sizeof(marker) - 1) {
        return 2;
    }
    return 0;
}
