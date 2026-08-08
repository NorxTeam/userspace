#include <stdint.h>
#include <norx/syscall.h>

extern "C" [[noreturn]] void __cxa_pure_virtual()
{
    norx_exit(127);
    for (;;) {
        __asm__ volatile ("" ::: "memory");
    }
}

extern "C" int __cxa_atexit(void (*function)(void *), void *argument, void *dso)
{
    (void)function;
    (void)argument;
    (void)dso;
    return 0;
}

extern "C" void __cxa_finalize(void *dso)
{
    (void)dso;
}

extern "C" int __cxa_guard_acquire(uint64_t *guard)
{
    uint64_t expected = 0;
    return __atomic_compare_exchange_n(
        guard, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}

extern "C" void __cxa_guard_release(uint64_t *guard)
{
    __atomic_store_n(guard, 2, __ATOMIC_RELEASE);
}

extern "C" void __cxa_guard_abort(uint64_t *guard)
{
    __atomic_store_n(guard, 0, __ATOMIC_RELEASE);
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
