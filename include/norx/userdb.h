#ifndef NORX_USERDB_H
#define NORX_USERDB_H

#include <stdint.h>

#define NORX_USERDB_ABI_VERSION 1u
#define NORX_USERDB_NAME_MAX 31u
#define NORX_USERDB_PATH_MAX 255u
#define NORX_USERDB_FLAG_DISABLED (1u << 0)
#define NORX_USERDB_FLAG_LOCKED (1u << 1)
#define NORX_USERDB_FLAG_EXPIRED (1u << 2)
#define NORX_USERDB_CAP_ACCOUNT_ADMIN (1ull << 8)
#define NORX_USERDB_CAP_SESSION_ADMIN (1ull << 9)

/* The lookup ABI deliberately contains no password hash or secret material. */
typedef struct {
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t name_length;
    uint32_t home_length;
    uint32_t shell_length;
    uint64_t capabilities;
    char name[NORX_USERDB_NAME_MAX + 1u];
    char home[NORX_USERDB_PATH_MAX + 1u];
    char shell[NORX_USERDB_PATH_MAX + 1u];
} norx_userdb_user_t;

typedef struct {
    uint32_t real_uid;
    uint32_t effective_uid;
    uint32_t saved_uid;
    uint32_t real_gid;
    uint32_t effective_gid;
    uint32_t saved_gid;
    uint64_t capabilities;
} norx_userdb_credentials_t;

enum norx_userdb_mutation {
    NORX_USERDB_CHANGE_OWN_PASSWORD = 1u,
    NORX_USERDB_MODIFY_ACCOUNT = 2u,
    NORX_USERDB_MODIFY_GROUP = 3u,
};

#endif
