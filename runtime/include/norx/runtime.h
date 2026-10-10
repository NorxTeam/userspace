#ifndef NORX_RUNTIME_H
#define NORX_RUNTIME_H

#include <stdint.h>

#define NORX_STARTUP_MAGIC 0x4e4f525853544152ULL
#define NORX_STARTUP_VERSION 1ULL
#define NORX_STARTUP_HEADER_WORDS 12ULL
#define NORX_STARTUP_HEADER_SIZE (NORX_STARTUP_HEADER_WORDS * sizeof(uint64_t))
#define NORX_STARTUP_MAX_BYTES 4096ULL
#define NORX_STARTUP_MAX_ARGUMENTS 16ULL
#define NORX_STARTUP_MAX_ENVIRONMENT 16ULL
#define NORX_STARTUP_AUXV_ENTRIES 3ULL
#define NORX_STARTUP_REJECT_STATUS 127
#define NORX_STARTUP_AT_PAGESZ 6ULL
#define NORX_STARTUP_AT_ENTRY 9ULL

typedef struct norx_startup_block {
    uint64_t magic;
    uint64_t version;
    uint64_t header_size;
    uint64_t block_size;
    uint64_t argc;
    uint64_t envc;
    uint64_t argv_offset;
    uint64_t envp_offset;
    uint64_t auxv_offset;
    uint64_t auxv_count;
    uint64_t strings_offset;
    uint64_t strings_size;
} norx_startup_block_t;

int norx_start(const norx_startup_block_t *initial_stack);

#endif
