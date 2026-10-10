#!/usr/bin/env python3
"""Exercise the C++ guard protocol with real host threads."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src" / "cxx" / "cxxabi.cpp"

SYSCALL_SHIM = r"""
#ifndef NORX_SYSCALL_H
#define NORX_SYSCALL_H

#include <stdint.h>
#include <sched.h>
#include <unistd.h>

typedef uint64_t norx_word_t;

static uint64_t norx_test_next_tid = 1;
static _Thread_local uint64_t norx_test_tid;

static inline int norx_cxa_atexit_register(void (*function)(void *), void *argument, void *dso)
{
    (void)function;
    (void)argument;
    (void)dso;
    return 0;
}

static inline void norx_cxa_finalize(void *dso)
{
    (void)dso;
}

static inline int norx_is_error(norx_word_t value)
{
    return value >= UINT64_MAX - 4095;
}

static inline norx_word_t norx_gettid(void)
{
    if (norx_test_tid == 0) {
        norx_test_tid = __atomic_fetch_add(&norx_test_next_tid, 1, __ATOMIC_RELAXED);
    }
    return norx_test_tid;
}

static inline norx_word_t norx_yield(void)
{
    return (norx_word_t)sched_yield();
}

static inline void norx_exit(int status)
{
    _exit(status);
}

#endif
"""

HARNESS = r"""
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern int __cxa_guard_acquire(uint64_t *guard);
extern void __cxa_guard_release(uint64_t *guard);
extern void __cxa_guard_abort(uint64_t *guard);

enum { WAITER_COUNT = 8 };

static uint64_t guard;
static _Atomic unsigned started;
static _Atomic unsigned completed;
static _Atomic int results[WAITER_COUNT];

static void *waiter(void *argument)
{
    const unsigned index = (unsigned)(uintptr_t)argument;
    atomic_fetch_add_explicit(&started, 1, memory_order_release);
    const int result = __cxa_guard_acquire(&guard);
    atomic_store_explicit(&results[index], result, memory_order_release);
    atomic_fetch_add_explicit(&completed, 1, memory_order_release);
    return 0;
}

static int expect_exit(void (*function)(void))
{
    const pid_t child = fork();
    assert(child >= 0);
    if (child == 0) {
        function();
        _exit(126);
    }

    int status = 0;
    assert(waitpid(child, &status, 0) == child);
    return WIFEXITED(status) && WEXITSTATUS(status) == 127;
}

static void recursive_acquire(void)
{
    uint64_t recursive_guard = 0;
    assert(__cxa_guard_acquire(&recursive_guard) == 1);
    (void)__cxa_guard_acquire(&recursive_guard);
    _exit(125);
}

static void malformed_state(void)
{
    uint64_t malformed_guard = 0x2;
    (void)__cxa_guard_acquire(&malformed_guard);
    _exit(124);
}

int main(void)
{
    assert(__cxa_guard_acquire(&guard) == 1);
    assert(guard != 0);
    assert((guard & 0xffu) == 0);

    pthread_t threads[WAITER_COUNT];
    for (unsigned index = 0; index < WAITER_COUNT; ++index) {
        atomic_store_explicit(&results[index], -1, memory_order_relaxed);
        assert(pthread_create(&threads[index], 0, waiter, (void *)(uintptr_t)index) == 0);
    }
    while (atomic_load_explicit(&started, memory_order_acquire) != WAITER_COUNT) {
        sched_yield();
    }
    for (unsigned spin = 0; spin < 10000; ++spin) {
        assert(atomic_load_explicit(&completed, memory_order_acquire) == 0);
        assert((guard & 0xffu) == 0);
        sched_yield();
    }

    __cxa_guard_release(&guard);
    assert((guard & 0xffu) != 0);
    for (unsigned index = 0; index < WAITER_COUNT; ++index) {
        assert(pthread_join(threads[index], 0) == 0);
        assert(atomic_load_explicit(&results[index], memory_order_acquire) == 0);
    }
    assert(__cxa_guard_acquire(&guard) == 0);

    guard = 0;
    assert(__cxa_guard_acquire(&guard) == 1);
    __cxa_guard_abort(&guard);
    assert((guard & 0xffu) == 0);
    assert(__cxa_guard_acquire(&guard) == 1);
    __cxa_guard_release(&guard);
    assert(__cxa_guard_acquire(&guard) == 0);

    assert(expect_exit(recursive_acquire));
    assert(expect_exit(malformed_state));
    return 0;
}
"""


def run(command: list[str]) -> None:
    subprocess.run(command, check=True)


def main() -> int:
    cxx = os.environ.get("CXX", "clang++")
    cc = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory(prefix="norx-cxxabi-guard-") as directory:
        root = Path(directory)
        include = root / "include" / "norx"
        include.mkdir(parents=True)
        (include / "syscall.h").write_text(SYSCALL_SHIM, encoding="utf-8")
        harness = root / "guard_test.c"
        harness.write_text(HARNESS, encoding="utf-8")
        cxx_object = root / "cxxabi.o"
        binary = root / "guard_test"
        run([
            cxx,
            "-std=c++17",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(root / "include"),
            "-c",
            str(SOURCE),
            "-o",
            str(cxx_object),
        ])
        run([
            cc,
            "-std=c11",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pthread",
            str(harness),
            str(cxx_object),
            "-o",
            str(binary),
        ])
        run([str(binary)])
    print("C++ ABI guard test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
