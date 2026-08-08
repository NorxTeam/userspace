# Norx userspace/toolchain boundary

This directory publishes the frozen first syscall ABI surface. The header is
freestanding and provides the x86_64 `syscall` and AArch64 `svc` register
shims, syscall numbers, six-word argument wrappers, and negative-errno
decoding.

The package recipe is [`gamma.toml`](gamma.toml). It is a source recipe only;
the installed package manifest and resolved lock file belong to Gamma state.

The smoke example uses only calls implemented by the current runtime:
`getpid`, `gettid`, `yield`, and `exit`. `read` and `write` are declared for
ABI completeness but currently return `-ENOSYS`, as documented by the kernel.

The target-owned freestanding C/C++ runtime is under
[`runtime/`](runtime/README.md). It supplies the startup objects, libc memory
and syscall wrappers, atomics/threading subset, no-exception C++ ABI shim, and
the small placement-new header subset. Build it with
`python toolchain/scripts/build-runtime.py`; the resulting archives and ELF
fixtures are staged in the separate test rootfs under
`test-rootfs/usr/lib` and `test-rootfs/tests/runtime`.

Package installation targets `/cfg` for configuration/appdata and
`/users` for user data. `/etc` and `/home` are compatibility paths during the
porting period.

Compile the example for the matching target with a freestanding C compiler:

```sh
cc -ffreestanding -fno-builtin -nostdlib \
  -I include -c examples/abi_smoke.c -o abi_smoke.o
```

Linking and QEMU execution remain follow-up work until user-mode process
creation, executable instruction dispatch, and the pointer-accepting syscall
paths are active.
