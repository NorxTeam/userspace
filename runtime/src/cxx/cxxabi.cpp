#include <stdint.h>
#include <norx/syscall.h>

extern "C" int norx_cxa_atexit_register(void (*function)(void *), void *argument, void *dso);
extern "C" void norx_cxa_finalize(void *dso);
extern "C" void *__dso_handle = &__dso_handle;

extern "C" [[noreturn]] void __cxa_pure_virtual()
{
    norx_exit(127);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

extern "C" int __cxa_atexit(void (*function)(void *), void *argument, void *dso)
{
    return norx_cxa_atexit_register(function, argument, dso);
}

extern "C" void __cxa_finalize(void *dso)
{
    norx_cxa_finalize(dso);
}

namespace {

// The compiler tests the first guard byte before calling __cxa_guard_acquire.
// Pending must therefore be invisible in that byte; the remaining bytes carry
// an owner TID so recursive initialization can fail instead of deadlocking.
constexpr uint64_t kGuardInitialized = 1;
constexpr uint64_t kGuardPending = 1ull << 8;
constexpr unsigned kGuardOwnerShift = 16;
constexpr uint64_t kGuardOwnerMask = 0xffffffffull << kGuardOwnerShift;
constexpr uint64_t kGuardKnownMask = kGuardPending | kGuardOwnerMask;

[[noreturn]] static void norx_cpp_guard_failure()
{
    norx_exit(127);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

static uint64_t norx_cpp_current_tid()
{
    const uint64_t tid = norx_gettid();
    if (norx_is_error(tid) || tid == 0 || tid > 0xffffffffull) {
        norx_cpp_guard_failure();
    }
    return tid;
}

static uint64_t norx_cpp_pending_state(uint64_t tid)
{
    return kGuardPending | (tid << kGuardOwnerShift);
}

static bool norx_cpp_is_pending(uint64_t state)
{
    return (state & ~kGuardKnownMask) == 0 && (state & kGuardPending) != 0
        && (state & kGuardOwnerMask) != 0;
}

static uint64_t norx_cpp_pending_owner(uint64_t state)
{
    return (state & kGuardOwnerMask) >> kGuardOwnerShift;
}

static void norx_cpp_finish_guard(uint64_t *guard, uint64_t desired)
{
    const uint64_t expected = norx_cpp_pending_state(norx_cpp_current_tid());
    uint64_t observed = expected;
    if (!__atomic_compare_exchange_n(
            guard, &observed, desired, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE)) {
        norx_cpp_guard_failure();
    }
}

} // namespace

extern "C" int __cxa_guard_acquire(uint64_t *guard)
{
    for (;;) {
        const uint64_t state = __atomic_load_n(guard, __ATOMIC_ACQUIRE);
        if (state == kGuardInitialized) {
            return 0;
        }

        if (state == 0) {
            const uint64_t pending = norx_cpp_pending_state(norx_cpp_current_tid());
            uint64_t expected = 0;
            if (__atomic_compare_exchange_n(
                    guard, &expected, pending, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
                return 1;
            }
            continue;
        }

        if (!norx_cpp_is_pending(state)) {
            norx_cpp_guard_failure();
        }
        if (norx_cpp_pending_owner(state) == norx_cpp_current_tid()) {
            norx_cpp_guard_failure();
        }

        // Norx has no futex/guard wait primitive yet. Yield is the scheduler
        // boundary, and remains safe when there is no other runnable thread.
        (void)norx_yield();
        __asm__ volatile ("" ::: "memory");
    }
}

extern "C" void __cxa_guard_release(uint64_t *guard)
{
    norx_cpp_finish_guard(guard, kGuardInitialized);
}

extern "C" void __cxa_guard_abort(uint64_t *guard)
{
    norx_cpp_finish_guard(guard, 0);
}

[[noreturn]] static void norx_cpp_no_memory()
{
    norx_exit(12);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

void *operator new(unsigned long size)
{
    (void)size;
    norx_cpp_no_memory();
}

void *operator new[](unsigned long size)
{
    (void)size;
    norx_cpp_no_memory();
}

void operator delete(void *pointer) noexcept
{
    (void)pointer;
}

void operator delete[](void *pointer) noexcept
{
    (void)pointer;
}

void operator delete(void *pointer, unsigned long size) noexcept
{
    (void)pointer;
    (void)size;
}

void operator delete[](void *pointer, unsigned long size) noexcept
{
    (void)pointer;
    (void)size;
}
