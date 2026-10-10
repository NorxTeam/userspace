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
  layout on native AArch64;
- `memcpy`, `memmove`, `memset`, `memcmp`, string functions, syscall-backed
  process wrappers, generation-tagged per-thread errno storage, and an
  explicit no-allocator `malloc`
  failure path;
- a no-exception/no-RTTI C++ ABI subset (`__cxa_*`, guard operations, and
  non-throwing allocation symbols) plus a freestanding placement-new/
  `cstddef` header subset.

Normal `exit` drains a bounded, LIFO callback registry shared by C `atexit`
and C++ `__cxa_atexit`, with up to `NORX_ATEXIT_MAX_CALLBACKS` active
callbacks. `_Exit` and `abort` intentionally bypass cleanup.

C++ local-static guards keep the ABI-visible first byte clear while
initialization is pending, record the owning generational TID in the remaining
guard bytes, yield while another thread initializes, and reset on abort. A
recursive initializer or unavailable `gettid` fails closed with exit status
127; exceptions remain disabled.

The current kernel ABI has no thread-create syscall, so `thrd_create` and
`thrd_join` return `thrd_error`/`ENOSYS`; the mutex subset is a short
single-address-space spinlock. This is an intentional supported boundary,
not a claim that POSIX threads are ready.

Errno storage is keyed by the complete generation-bearing TID and bounded to
the kernel's 64-thread process-table limit. It does not use compiler TLS yet:
the static TLS register hand-off and context-switch ownership remain deferred
until the thread ABI lands. A valid kernel TID therefore has isolated errno;
the bootstrap slot is only for calls made before a thread identity exists.

`nanosleep` validates `tv_sec >= 0` and `0 <= tv_nsec < 1,000,000,000`, then
uses the kernel nanosleep boundary. The kernel rounds a positive request up to
its timer tick, writes a zero remainder on success, and preserves the full
request for a pre-sleep error. Signal interruption and partial elapsed
remainders remain unavailable until asynchronous signal delivery is enabled.

`libm`, floating-point formatted I/O, C++ exceptions, RTTI, and the hosted
C++ library are compile-time deferred. Norx currently forbids generated FP
instructions and does not context-switch FP state, while the exception and
dynamic-library contracts are not implemented. `math.h` fails closed with a
target-specific diagnostic until those kernel/ABI gates are opened.

The callback registry is process-local and single-threaded because thread
creation and DSO unloading are not part of the current ABI. Registration after
the capacity bound or during finalization fails with `-1` and sets `errno`.
Callbacks are removed before invocation, so repeated or nested
`__cxa_finalize` calls do not invoke the same callback twice.

Run `python toolchain/scripts/build-runtime.py` to compile the runtime archive
and C/C++ link fixtures for both supported architectures. Outputs and hashes
are copied into `test-rootfs/usr/lib` and `test-rootfs/tests/runtime`.
