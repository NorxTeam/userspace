#include <norx/syscall.h>

/*
 * This translation unit is also the C-side ABI tripwire.  These assertions
 * intentionally duplicate the frozen kernel boundary: a changed number,
 * flag, errno, layout, or wrapper signature must fail at compile time.
 */
_Static_assert(sizeof(norx_word_t) == 8, "norx_word_t must be 64-bit");
_Static_assert(sizeof(norx_pointer_t) == 8, "norx_pointer_t must be 64-bit");

_Static_assert(NORX_SYS_READ == 0, "read syscall number drifted");
_Static_assert(NORX_SYS_WRITE == 1, "write syscall number drifted");
_Static_assert(NORX_SYS_OPEN == 2, "open syscall number drifted");
_Static_assert(NORX_SYS_MKDIR == 420, "mkdir syscall number drifted");
_Static_assert(NORX_SYS_RMDIR == 421, "rmdir syscall number drifted");
_Static_assert(NORX_SYS_UNLINK == 422, "unlink syscall number drifted");
_Static_assert(NORX_SYS_RENAME == 423, "rename syscall number drifted");
_Static_assert(NORX_SYS_STAT == 425, "stat syscall number drifted");
_Static_assert(NORX_SYS_READ_DIR == 426, "read_dir syscall number drifted");
_Static_assert(NORX_SYS_FSYNC == 427, "fsync syscall number drifted");
_Static_assert(NORX_SYS_SYNC_PATH == 428, "sync_path syscall number drifted");
_Static_assert(NORX_SYS_LSEEK == 429, "lseek syscall number drifted");
_Static_assert(NORX_SYS_FSTAT == 430, "fstat syscall number drifted");
_Static_assert(NORX_SYS_FCHMOD == 431, "fchmod syscall number drifted");
_Static_assert(NORX_SYS_FCNTL == 432, "fcntl syscall number drifted");

_Static_assert(NORX_OPEN_READ == ((norx_word_t)1 << 0), "OPEN_READ drifted");
_Static_assert(NORX_OPEN_WRITE == ((norx_word_t)1 << 1), "OPEN_WRITE drifted");
_Static_assert(NORX_OPEN_CREATE == ((norx_word_t)1 << 2), "OPEN_CREATE drifted");
_Static_assert(NORX_OPEN_TRUNCATE == ((norx_word_t)1 << 3), "OPEN_TRUNCATE drifted");
_Static_assert(NORX_OPEN_APPEND == ((norx_word_t)1 << 4), "OPEN_APPEND drifted");
_Static_assert(NORX_OPEN_EXCLUSIVE == ((norx_word_t)1 << 5), "OPEN_EXCLUSIVE drifted");
_Static_assert(
    (NORX_OPEN_READ | NORX_OPEN_WRITE | NORX_OPEN_CREATE |
     NORX_OPEN_TRUNCATE | NORX_OPEN_APPEND | NORX_OPEN_EXCLUSIVE) == 0x3full,
    "open flag mask drifted");
_Static_assert((NORX_OPEN_READ & NORX_OPEN_WRITE) == 0, "open access flags overlap");
_Static_assert(NORX_SEEK_SET == 0u, "SEEK_SET drifted");
_Static_assert(NORX_SEEK_CUR == 1u, "SEEK_CUR drifted");
_Static_assert(NORX_SEEK_END == 2u, "SEEK_END drifted");
_Static_assert(NORX_F_GETFD == 1u, "F_GETFD drifted");
_Static_assert(NORX_F_SETFD == 2u, "F_SETFD drifted");
_Static_assert(NORX_F_GETFD != NORX_F_SETFD, "fcntl commands must differ");
_Static_assert(NORX_FD_CLOEXEC == ((norx_word_t)1 << 0), "FD_CLOEXEC drifted");
_Static_assert((NORX_FD_CLOEXEC & ~((norx_word_t)1)) == 0, "FD flag mask drifted");
_Static_assert(NORX_STAT_REGULAR == 1u, "regular stat kind drifted");
_Static_assert(NORX_STAT_DIRECTORY == 2u, "directory stat kind drifted");
_Static_assert(NORX_STAT_REGULAR != NORX_STAT_DIRECTORY, "stat kinds must differ");
_Static_assert(NORX_MAX_DIR_ENTRIES == 16u, "directory capacity drifted");

_Static_assert(NORX_EBADF == 9, "EBADF drifted");
_Static_assert(NORX_EFAULT == 14, "EFAULT drifted");
_Static_assert(NORX_EINVAL == 22, "EINVAL drifted");
_Static_assert(NORX_ENOENT == 2, "ENOENT drifted");
_Static_assert(NORX_EEXIST == 17, "EEXIST drifted");
_Static_assert(NORX_ENOTDIR == 20, "ENOTDIR drifted");
_Static_assert(NORX_EISDIR == 21, "EISDIR drifted");
_Static_assert(NORX_ENOSPC == 28, "ENOSPC drifted");
_Static_assert(NORX_ENOTEMPTY == 39, "ENOTEMPTY drifted");
_Static_assert(NORX_ENAMETOOLONG == 36, "ENAMETOOLONG drifted");
_Static_assert(NORX_EXDEV == 18, "EXDEV drifted");
_Static_assert(NORX_EROFS == 30, "EROFS drifted");
_Static_assert(NORX_ENOTSUP == 95, "ENOTSUP drifted");
_Static_assert(NORX_EOVERFLOW == 75, "EOVERFLOW drifted");
_Static_assert(NORX_ERROR_MAX == 4095u, "error range drifted");
_Static_assert(
    ((norx_word_t)0 - (norx_word_t)NORX_EBADF) == UINT64_MAX - 8ull,
    "negative errno encoding drifted");
_Static_assert(
    ((norx_word_t)0 - (norx_word_t)NORX_EINVAL) == UINT64_MAX - 21ull,
    "negative errno encoding drifted");
_Static_assert(
    (norx_word_t)0 < UINT64_MAX - NORX_ERROR_MAX,
    "success value overlaps negative errno range");

_Static_assert(sizeof(norx_stat_t) == 24, "norx_stat_t size drifted");
_Static_assert(__builtin_offsetof(norx_stat_t, kind) == 0, "stat.kind offset drifted");
_Static_assert(__builtin_offsetof(norx_stat_t, mode) == 4, "stat.mode offset drifted");
_Static_assert(__builtin_offsetof(norx_stat_t, size) == 8, "stat.size offset drifted");
_Static_assert(__builtin_offsetof(norx_stat_t, links) == 16, "stat.links offset drifted");
_Static_assert(__builtin_offsetof(norx_stat_t, inode) == 20, "stat.inode offset drifted");
_Static_assert(sizeof(norx_dir_entry_t) == 56, "norx_dir_entry_t size drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, kind) == 0, "dir.kind offset drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, mode) == 4, "dir.mode offset drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, size) == 8, "dir.size offset drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, links) == 16, "dir.links offset drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, name_length) == 20, "dir.name_length offset drifted");
_Static_assert(__builtin_offsetof(norx_dir_entry_t, name) == 24, "dir.name offset drifted");

typedef norx_word_t (*norx_open_shape_t)(const char *, norx_word_t, norx_word_t, norx_word_t);
typedef norx_word_t (*norx_read_shape_t)(norx_word_t, void *, norx_word_t);
typedef norx_word_t (*norx_write_shape_t)(norx_word_t, const void *, norx_word_t);
typedef norx_word_t (*norx_lseek_shape_t)(norx_word_t, int64_t, norx_word_t);
typedef norx_word_t (*norx_fstat_shape_t)(norx_word_t, norx_stat_t *);
typedef norx_word_t (*norx_fchmod_shape_t)(norx_word_t, norx_word_t);
typedef norx_word_t (*norx_fcntl_shape_t)(norx_word_t, norx_word_t, norx_word_t);
typedef norx_word_t (*norx_fsync_shape_t)(norx_word_t);
typedef norx_word_t (*norx_sync_path_shape_t)(const char *, norx_word_t);
typedef norx_word_t (*norx_mkdir_shape_t)(const char *, norx_word_t, norx_word_t);
typedef norx_word_t (*norx_rmdir_shape_t)(const char *, norx_word_t);
typedef norx_word_t (*norx_unlink_shape_t)(const char *, norx_word_t);
typedef norx_word_t (*norx_rename_shape_t)(const char *, norx_word_t, const char *, norx_word_t);
typedef norx_word_t (*norx_stat_shape_t)(const char *, norx_word_t, norx_stat_t *);
typedef norx_word_t (*norx_read_dir_shape_t)(
    const char *, norx_word_t, norx_dir_entry_t *, norx_word_t);

#define NORX_ASSERT_WRAPPER_SHAPE(name, type) \
    _Static_assert(_Generic(&(name), type: 1, default: 0), #name " wrapper shape drifted")

NORX_ASSERT_WRAPPER_SHAPE(norx_open, norx_open_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_read, norx_read_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_write, norx_write_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_lseek, norx_lseek_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_fstat, norx_fstat_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_fchmod, norx_fchmod_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_fcntl, norx_fcntl_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_fsync, norx_fsync_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_sync_path, norx_sync_path_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_mkdir, norx_mkdir_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_rmdir, norx_rmdir_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_unlink, norx_unlink_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_rename, norx_rename_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_stat, norx_stat_shape_t);
NORX_ASSERT_WRAPPER_SHAPE(norx_read_dir, norx_read_dir_shape_t);

/* Unevaluated calls check the argument count/types without issuing syscalls. */
_Static_assert(_Generic(norx_open((const char *)0, 0, 0, 0), norx_word_t: 1, default: 0), "open call shape");
_Static_assert(_Generic(norx_read(0, (void *)0, 0), norx_word_t: 1, default: 0), "read call shape");
_Static_assert(_Generic(norx_write(0, (const void *)0, 0), norx_word_t: 1, default: 0), "write call shape");
_Static_assert(_Generic(norx_lseek(0, (int64_t)0, NORX_SEEK_SET), norx_word_t: 1, default: 0), "lseek call shape");
_Static_assert(_Generic(norx_fstat(0, (norx_stat_t *)0), norx_word_t: 1, default: 0), "fstat call shape");
_Static_assert(_Generic(norx_fchmod(0, 0), norx_word_t: 1, default: 0), "fchmod call shape");
_Static_assert(_Generic(norx_fcntl(0, NORX_F_GETFD, 0), norx_word_t: 1, default: 0), "fcntl call shape");
_Static_assert(_Generic(norx_fsync(0), norx_word_t: 1, default: 0), "fsync call shape");
_Static_assert(_Generic(norx_sync_path((const char *)0, 0), norx_word_t: 1, default: 0), "sync_path call shape");
_Static_assert(_Generic(norx_mkdir((const char *)0, 0, 0), norx_word_t: 1, default: 0), "mkdir call shape");
_Static_assert(_Generic(norx_rmdir((const char *)0, 0), norx_word_t: 1, default: 0), "rmdir call shape");
_Static_assert(_Generic(norx_unlink((const char *)0, 0), norx_word_t: 1, default: 0), "unlink call shape");
_Static_assert(_Generic(norx_rename((const char *)0, 0, (const char *)0, 0), norx_word_t: 1, default: 0), "rename call shape");
_Static_assert(_Generic(norx_stat((const char *)0, 0, (norx_stat_t *)0), norx_word_t: 1, default: 0), "stat call shape");
_Static_assert(_Generic(norx_read_dir((const char *)0, 0, (norx_dir_entry_t *)0, 0), norx_word_t: 1, default: 0), "read_dir call shape");

#undef NORX_ASSERT_WRAPPER_SHAPE

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
