#ifndef NORX_SYSCALL_H
#define NORX_SYSCALL_H

#include <stdint.h>

#define NORX_ABI_VERSION 2u
#define NORX_MAX_ARGS 6u
#define NORX_ERROR_MAX 4095u
#define NORX_SCHEDULER_HZ 100u

typedef uint64_t norx_word_t;
typedef uint64_t norx_pointer_t;

enum norx_syscall_number {
    NORX_SYS_READ = 0,
    NORX_SYS_WRITE = 1,
    NORX_SYS_CLOSE = 3,
    NORX_SYS_WAIT = 61,
    NORX_SYS_EXIT = 60,
    NORX_SYS_GETPID = 39,
    NORX_SYS_GETTID = 186,
    NORX_SYS_YIELD = 24,
    NORX_SYS_SLEEP = 35,
    NORX_SYS_OPEN = 2,
    NORX_SYS_PIPE = 22,
    NORX_SYS_DUP2 = 33,
    NORX_SYS_SPAWN = 400,
    NORX_SYS_WAIT_STATUS = 402,
    NORX_SYS_SPAWN2 = 401,
    NORX_SYS_SETPGID = 403,
    NORX_SYS_GETPGID = 404,
    NORX_SYS_KILLPG = 405,
    NORX_SYS_TTY_GET_FOREGROUND = 406,
    NORX_SYS_TTY_SET_FOREGROUND = 407,
    NORX_SYS_TTY_GET_INFO = 408,
    NORX_SYS_TTY_SET_WINDOW = 409,
    NORX_SYS_SET_SESSION = 410,
    NORX_SYS_GET_CREDENTIALS = 411,
    NORX_SYS_MKDIR = 420,
    NORX_SYS_RMDIR = 421,
    NORX_SYS_UNLINK = 422,
    NORX_SYS_RENAME = 423,
    NORX_SYS_LINK = 424,
    NORX_SYS_STAT = 425,
    NORX_SYS_READ_DIR = 426,
};

enum norx_errno {
    NORX_EPERM = 1,
    NORX_ENOENT = 2,
    NORX_EBADF = 9,
    NORX_ECHILD = 10,
    NORX_EINTR = 4,
    NORX_EAGAIN = 11,
    NORX_EFAULT = 14,
    NORX_EINVAL = 22,
    NORX_ENOMEM = 12,
    NORX_ENOTTY = 25,
    NORX_EPIPE = 32,
    NORX_ENOSYS = 38,
    NORX_EOVERFLOW = 75,
    NORX_EEXIST = 17,
    NORX_ENOTDIR = 20,
    NORX_EISDIR = 21,
    NORX_ENOSPC = 28,
    NORX_ENOTEMPTY = 39,
    NORX_ENAMETOOLONG = 36,
    NORX_EXDEV = 18,
    NORX_EROFS = 30,
    NORX_ENOTSUP = 95,
};

#define NORX_OPEN_READ (1ull << 0)
#define NORX_OPEN_WRITE (1ull << 1)
#define NORX_OPEN_CREATE (1ull << 2)
#define NORX_OPEN_TRUNCATE (1ull << 3)
#define NORX_OPEN_APPEND (1ull << 4)
#define NORX_PIPE_NONBLOCK (1ull << 0)
#define NORX_SPAWN_NEW_PROCESS_GROUP (1ull << 0)
#define NORX_SPAWN_FOREGROUND (1ull << 1)
#define NORX_SPAWN_INHERIT_CREDENTIALS (1ull << 2)
#define NORX_WAIT_NONBLOCK (1ull << 0)

enum norx_wait_kind {
    NORX_WAIT_EXITED = 1u,
    NORX_WAIT_SIGNALED = 2u,
    NORX_WAIT_STOPPED = 3u,
    NORX_WAIT_CONTINUED = 4u,
};

#define NORX_TTY_FLAG_AVAILABLE (1u << 0)
#define NORX_TTY_FLAG_SERIAL (1u << 1)

typedef struct {
    uint32_t flags;
    uint32_t controlling_process;
    uint32_t foreground_group;
    uint16_t columns;
    uint16_t rows;
    uint32_t reserved;
} norx_tty_info_t;

typedef struct norx_pipe_fds {
    norx_word_t read;
    norx_word_t write;
} norx_pipe_fds_t;

typedef struct norx_wait_status {
    uint32_t kind;
    int32_t code;
    uint32_t signal;
    uint32_t reserved;
} norx_wait_status_t;

typedef struct norx_spawn_spec {
    norx_pointer_t path;
    norx_word_t path_length;
    norx_pointer_t argv;
    norx_word_t argc;
    norx_pointer_t environment;
    norx_word_t environment_count;
    norx_word_t stdin_fd;
    norx_word_t stdout_fd;
    norx_word_t stderr_fd;
    norx_word_t process_group;
    norx_word_t flags;
} norx_spawn_spec_t;

typedef struct norx_credentials {
    uint32_t real_uid;
    uint32_t effective_uid;
    uint32_t saved_uid;
    uint32_t real_gid;
    uint32_t effective_gid;
    uint32_t saved_gid;
    uint64_t capabilities;
} norx_credentials_t;

typedef struct norx_session_spec {
    norx_credentials_t credentials;
    norx_pointer_t cwd;
    norx_word_t cwd_length;
    uint32_t umask;
    uint32_t reserved;
    uint32_t max_fds;
    uint32_t max_address_space_pages;
    uint64_t max_cpu_ticks;
} norx_session_spec_t;

typedef struct norx_stat {
    uint32_t kind;
    uint32_t mode;
    uint64_t size;
    uint32_t links;
    uint32_t inode;
} norx_stat_t;

typedef struct norx_dir_entry {
    uint32_t kind;
    uint32_t mode;
    uint64_t size;
    uint32_t links;
    uint32_t name_length;
    uint8_t name[32];
} norx_dir_entry_t;

#define NORX_STAT_REGULAR 1u
#define NORX_STAT_DIRECTORY 2u
#define NORX_MAX_DIR_ENTRIES 16u

static inline int norx_is_error(norx_word_t value)
{
    return value >= UINT64_MAX - NORX_ERROR_MAX;
}

static inline int norx_errno_value(norx_word_t value)
{
    return (int)(0u - value);
}

#if defined(__x86_64__)

static inline norx_word_t norx_syscall6(
    norx_word_t number,
    norx_word_t a0,
    norx_word_t a1,
    norx_word_t a2,
    norx_word_t a3,
    norx_word_t a4,
    norx_word_t a5)
{
    norx_word_t result;
    register norx_word_t arg3 __asm__("r10") = a3;
    register norx_word_t arg4 __asm__("r8") = a4;
    register norx_word_t arg5 __asm__("r9") = a5;

    __asm__ volatile (
        "syscall"
        : "=a"(result)
        : "a"(number), "D"(a0), "S"(a1), "d"(a2),
          "r"(arg3), "r"(arg4), "r"(arg5)
        : "rcx", "r11", "memory");
    return result;
}

#elif defined(__aarch64__)

static inline norx_word_t norx_syscall6(
    norx_word_t number,
    norx_word_t a0,
    norx_word_t a1,
    norx_word_t a2,
    norx_word_t a3,
    norx_word_t a4,
    norx_word_t a5)
{
    register norx_word_t x0 __asm__("x0") = a0;
    register norx_word_t x1 __asm__("x1") = a1;
    register norx_word_t x2 __asm__("x2") = a2;
    register norx_word_t x3 __asm__("x3") = a3;
    register norx_word_t x4 __asm__("x4") = a4;
    register norx_word_t x5 __asm__("x5") = a5;
    register norx_word_t x8 __asm__("x8") = number;

    __asm__ volatile (
        "svc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
        : "memory");
    return x0;
}

#else
#error "Norx syscall ABI supports only x86_64 and aarch64"
#endif

static inline norx_word_t norx_syscall0(norx_word_t number)
{
    return norx_syscall6(number, 0, 0, 0, 0, 0, 0);
}

static inline norx_word_t norx_syscall1(norx_word_t number, norx_word_t a0)
{
    return norx_syscall6(number, a0, 0, 0, 0, 0, 0);
}

static inline norx_word_t norx_syscall2(norx_word_t number, norx_word_t a0, norx_word_t a1)
{
    return norx_syscall6(number, a0, a1, 0, 0, 0, 0);
}

static inline norx_word_t norx_syscall3(
    norx_word_t number, norx_word_t a0, norx_word_t a1, norx_word_t a2)
{
    return norx_syscall6(number, a0, a1, a2, 0, 0, 0);
}

static inline norx_word_t norx_syscall4(
    norx_word_t number, norx_word_t a0, norx_word_t a1, norx_word_t a2, norx_word_t a3)
{
    return norx_syscall6(number, a0, a1, a2, a3, 0, 0);
}

static inline norx_word_t norx_syscall5(
    norx_word_t number,
    norx_word_t a0,
    norx_word_t a1,
    norx_word_t a2,
    norx_word_t a3,
    norx_word_t a4)
{
    return norx_syscall6(number, a0, a1, a2, a3, a4, 0);
}

static inline norx_word_t norx_getpid(void)
{
    return norx_syscall0(NORX_SYS_GETPID);
}

static inline norx_word_t norx_gettid(void)
{
    return norx_syscall0(NORX_SYS_GETTID);
}

static inline norx_word_t norx_wait(norx_word_t child)
{
    return norx_syscall1(NORX_SYS_WAIT, child);
}

static inline norx_word_t norx_yield(void)
{
    return norx_syscall0(NORX_SYS_YIELD);
}

static inline norx_word_t norx_sleep(norx_word_t duration)
{
    return norx_syscall1(NORX_SYS_SLEEP, duration);
}

static inline norx_word_t norx_close(norx_word_t fd)
{
    return norx_syscall1(NORX_SYS_CLOSE, fd);
}

static inline norx_word_t norx_open(
    const char *path, norx_word_t length, norx_word_t flags, norx_word_t mode)
{
    return norx_syscall4(
        NORX_SYS_OPEN, (norx_pointer_t)(uintptr_t)path, length, flags, mode);
}

static inline norx_word_t norx_mkdir(const char *path, norx_word_t length, norx_word_t mode)
{
    return norx_syscall3(
        NORX_SYS_MKDIR, (norx_pointer_t)(uintptr_t)path, length, mode);
}

static inline norx_word_t norx_rmdir(const char *path, norx_word_t length)
{
    return norx_syscall2(NORX_SYS_RMDIR, (norx_pointer_t)(uintptr_t)path, length);
}

static inline norx_word_t norx_unlink(const char *path, norx_word_t length)
{
    return norx_syscall2(NORX_SYS_UNLINK, (norx_pointer_t)(uintptr_t)path, length);
}

static inline norx_word_t norx_rename(
    const char *old_path, norx_word_t old_length, const char *new_path, norx_word_t new_length)
{
    return norx_syscall4(
        NORX_SYS_RENAME,
        (norx_pointer_t)(uintptr_t)old_path,
        old_length,
        (norx_pointer_t)(uintptr_t)new_path,
        new_length);
}

static inline norx_word_t norx_link(
    const char *old_path, norx_word_t old_length, const char *new_path, norx_word_t new_length)
{
    return norx_syscall4(
        NORX_SYS_LINK,
        (norx_pointer_t)(uintptr_t)old_path,
        old_length,
        (norx_pointer_t)(uintptr_t)new_path,
        new_length);
}

static inline norx_word_t norx_stat(
    const char *path, norx_word_t length, norx_stat_t *output)
{
    return norx_syscall3(
        NORX_SYS_STAT, (norx_pointer_t)(uintptr_t)path, length,
        (norx_pointer_t)(uintptr_t)output);
}

static inline norx_word_t norx_read_dir(
    const char *path, norx_word_t length, norx_dir_entry_t *output, norx_word_t capacity)
{
    return norx_syscall4(
        NORX_SYS_READ_DIR, (norx_pointer_t)(uintptr_t)path, length,
        (norx_pointer_t)(uintptr_t)output, capacity);
}

static inline norx_word_t norx_pipe(norx_pipe_fds_t *fds, norx_word_t flags)
{
    return norx_syscall2(NORX_SYS_PIPE, (norx_pointer_t)(uintptr_t)fds, flags);
}

static inline norx_word_t norx_dup2(norx_word_t old_fd, norx_word_t new_fd)
{
    return norx_syscall2(NORX_SYS_DUP2, old_fd, new_fd);
}

static inline norx_word_t norx_wait_status(
    norx_word_t child, norx_wait_status_t *status, norx_word_t options)
{
    return norx_syscall3(
        NORX_SYS_WAIT_STATUS, child, (norx_pointer_t)(uintptr_t)status, options);
}

static inline norx_word_t norx_spawn2(const norx_spawn_spec_t *spec)
{
    return norx_syscall1(NORX_SYS_SPAWN2, (norx_pointer_t)(uintptr_t)spec);
}

static inline norx_word_t norx_set_session(const norx_session_spec_t *spec)
{
    return norx_syscall1(NORX_SYS_SET_SESSION, (norx_pointer_t)(uintptr_t)spec);
}

static inline norx_word_t norx_get_credentials(norx_credentials_t *output)
{
    return norx_syscall1(NORX_SYS_GET_CREDENTIALS, (norx_pointer_t)(uintptr_t)output);
}

static inline norx_word_t norx_setpgid(norx_word_t process, norx_word_t group)
{
    return norx_syscall2(NORX_SYS_SETPGID, process, group);
}

static inline norx_word_t norx_getpgid(norx_word_t process)
{
    return norx_syscall1(NORX_SYS_GETPGID, process);
}

static inline norx_word_t norx_killpg(norx_word_t group, norx_word_t signal)
{
    return norx_syscall2(NORX_SYS_KILLPG, group, signal);
}

static inline norx_word_t norx_tty_get_foreground(norx_word_t fd)
{
    return norx_syscall1(NORX_SYS_TTY_GET_FOREGROUND, fd);
}

static inline norx_word_t norx_tty_set_foreground(norx_word_t fd, norx_word_t group)
{
    return norx_syscall2(NORX_SYS_TTY_SET_FOREGROUND, fd, group);
}

static inline norx_word_t norx_tty_get_info(norx_word_t fd, norx_tty_info_t *info)
{
    return norx_syscall3(
        NORX_SYS_TTY_GET_INFO, fd, (norx_pointer_t)(uintptr_t)info, sizeof(*info));
}

static inline norx_word_t norx_tty_set_window(
    norx_word_t fd, uint16_t columns, uint16_t rows)
{
    return norx_syscall3(NORX_SYS_TTY_SET_WINDOW, fd, columns, rows);
}

static inline norx_word_t norx_write(
    norx_word_t fd, const void *buffer, norx_word_t length)
{
    return norx_syscall3(NORX_SYS_WRITE, fd, (norx_pointer_t)(uintptr_t)buffer, length);
}

static inline norx_word_t norx_read(
    norx_word_t fd, void *buffer, norx_word_t length)
{
    return norx_syscall3(NORX_SYS_READ, fd, (norx_pointer_t)(uintptr_t)buffer, length);
}

static inline void norx_exit(int status)
{
    (void)norx_syscall1(NORX_SYS_EXIT, (norx_word_t)(int64_t)status);
}

#endif
