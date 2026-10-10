#include <limits.h>
#include <stdint.h>
#include <norx/runtime.h>
#include <stdlib.h>

extern int main(int argc, char **argv, char **environment);

static int startup_range(
    uintptr_t base,
    uint64_t offset,
    uint64_t length,
    uintptr_t block_end,
    uintptr_t *start,
    uintptr_t *end)
{
    if (offset > UINTPTR_MAX || length > UINTPTR_MAX) {
        return 0;
    }
    uintptr_t range_start = (uintptr_t)offset;
    if (range_start > UINTPTR_MAX - base) {
        return 0;
    }
    range_start += base;
    if ((uintptr_t)length > UINTPTR_MAX - range_start) {
        return 0;
    }
    uintptr_t range_end = range_start + (uintptr_t)length;
    if (range_end > block_end) {
        return 0;
    }
    *start = range_start;
    *end = range_end;
    return 1;
}

static uintptr_t startup_word(uintptr_t base, uint64_t index)
{
    return *(const uintptr_t *)(base + (uintptr_t)(index * sizeof(uintptr_t)));
}

static int startup_string(uintptr_t pointer, uintptr_t strings_start, uintptr_t strings_end)
{
    if (pointer < strings_start || pointer >= strings_end) {
        return 0;
    }
    while (pointer < strings_end) {
        if (*(const uint8_t *)pointer == 0) {
            return 1;
        }
        ++pointer;
    }
    return 0;
}

int norx_start(const norx_startup_block_t *initial_stack)
{
    if (initial_stack == 0) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    uintptr_t base = (uintptr_t)initial_stack;
    if ((base & (sizeof(uint64_t) - 1)) != 0) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    const norx_startup_block_t *block = initial_stack;
    if (block->magic != NORX_STARTUP_MAGIC || block->version != NORX_STARTUP_VERSION ||
        block->header_size != NORX_STARTUP_HEADER_SIZE ||
        block->block_size < NORX_STARTUP_HEADER_SIZE ||
        block->block_size > NORX_STARTUP_MAX_BYTES ||
        block->argc > NORX_STARTUP_MAX_ARGUMENTS ||
        block->envc > NORX_STARTUP_MAX_ENVIRONMENT ||
        block->argc > (uint64_t)INT_MAX ||
        block->auxv_count != NORX_STARTUP_AUXV_ENTRIES) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    if (block->block_size > UINTPTR_MAX - base) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    uintptr_t block_end = base + (uintptr_t)block->block_size;
    uint64_t argv_bytes = (block->argc + 1) * sizeof(uintptr_t);
    uint64_t envp_bytes = (block->envc + 1) * sizeof(uintptr_t);
    uint64_t auxv_bytes = NORX_STARTUP_AUXV_ENTRIES * 2 * sizeof(uint64_t);
    uint64_t envp_offset = NORX_STARTUP_HEADER_SIZE + argv_bytes;
    uint64_t auxv_offset = envp_offset + envp_bytes;
    if (block->argv_offset != NORX_STARTUP_HEADER_SIZE ||
        block->envp_offset != envp_offset || block->auxv_offset != auxv_offset ||
        block->strings_offset < auxv_offset + auxv_bytes ||
        block->strings_offset > block->block_size ||
        block->strings_size != block->block_size - block->strings_offset) {
        return NORX_STARTUP_REJECT_STATUS;
    }

    uintptr_t argv_start;
    uintptr_t argv_end;
    uintptr_t envp_start;
    uintptr_t envp_end;
    uintptr_t auxv_start;
    uintptr_t auxv_end;
    uintptr_t strings_start;
    uintptr_t strings_end;
    if (!startup_range(base, block->argv_offset, argv_bytes, block_end, &argv_start, &argv_end) ||
        !startup_range(base, block->envp_offset, envp_bytes, block_end, &envp_start, &envp_end) ||
        !startup_range(base, block->auxv_offset, auxv_bytes, block_end, &auxv_start, &auxv_end) ||
        !startup_range(
            base,
            block->strings_offset,
            block->strings_size,
            block_end,
            &strings_start,
            &strings_end) ||
        argv_end > envp_start || envp_end > auxv_start || auxv_end > strings_start ||
        strings_end != block_end) {
        return NORX_STARTUP_REJECT_STATUS;
    }

    const uint64_t *auxv = (const uint64_t *)auxv_start;
    if (auxv[0] != NORX_STARTUP_AT_PAGESZ || auxv[1] != 4096 ||
        auxv[2] != NORX_STARTUP_AT_ENTRY || auxv[3] == 0 || auxv[4] != 0 ||
        auxv[5] != 0) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    for (uint64_t index = 0; index < block->argc; ++index) {
        if (!startup_string(startup_word(argv_start, index), strings_start, strings_end)) {
            return NORX_STARTUP_REJECT_STATUS;
        }
    }
    if (startup_word(argv_start, block->argc) != 0) {
        return NORX_STARTUP_REJECT_STATUS;
    }
    for (uint64_t index = 0; index < block->envc; ++index) {
        if (!startup_string(startup_word(envp_start, index), strings_start, strings_end)) {
            return NORX_STARTUP_REJECT_STATUS;
        }
    }
    if (startup_word(envp_start, block->envc) != 0) {
        return NORX_STARTUP_REJECT_STATUS;
    }

    int main_argc = (int)block->argc;
    int status = main(main_argc, (char **)argv_start, (char **)envp_start);
    exit(status);
}
