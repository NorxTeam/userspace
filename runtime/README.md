# Norx freestanding C/C++ runtime

This is the first target-owned runtime layer. It is built with the Norx
sysroot and never links a host CRT or host libc.

The staging package recipe is [`gamma.toml`](gamma.toml). It describes the
target-aware runtime build and its C/C++ fixture outputs; generated manifests
remain build artifacts under the rootfs.

Implemented in this gate:

- target headers for sizes, integer types, errno, strings, process I/O,
  time, and a small C11 atomic/threading surface;
- `_start` plus stack parsing for the frozen `argc/argv/envp/auxv` startup
  layout on x86_64 and AArch64;
- `memcpy`, `memmove`, `memset`, `memcmp`, string functions, syscall-backed
  process wrappers, errno storage, and an explicit no-allocator `malloc`
  failure path;
- a no-exception/no-RTTI C++ ABI subset (`__cxa_*`, guard operations, and
  non-throwing allocation symbols) plus a freestanding placement-new/
  `cstddef` header subset.

The current kernel ABI has no thread-create syscall, so `thrd_create` and
`thrd_join` return `thrd_error`/`ENOSYS`; the mutex subset is a short
single-address-space spinlock. This is an intentional supported boundary,
not a claim that POSIX threads are ready.

`libm`, floating-point formatted I/O, C++ exceptions, RTTI, and the hosted
C++ library are compile-time deferred. Norx currently forbids generated FP
instructions and does not context-switch FP state, while the exception and
dynamic-library contracts are not implemented. `math.h` fails closed with a
target-specific diagnostic until those kernel/ABI gates are opened.

Run `python toolchain/scripts/build-runtime.py` to compile the runtime archive
and C/C++ link fixtures for both supported architectures. Outputs and hashes
are copied into `test-rootfs/usr/lib` and `test-rootfs/tests/runtime`.
