# Norx userspace sources

This repository is the source of truth for code that is linked into Norx
userspace programs. It is not the compiler, target specification, linker, or
generated sysroot.

## Ownership

- `include/` — public freestanding C ABI headers, including the syscall ABI.
- `runtime/` — target-owned C/C++ startup code, libc subset, and C++ ABI
  subset.
- `rust/` — the `no_std` Rust userspace API, startup, allocator, and smoke
  binary.
- `examples/` — source fixtures consumed by the toolchain smoke builds.

The corresponding build and release project is [`../toolchain/`](../toolchain/).
It owns target JSON files, linker scripts, compiler-version pins, build
orchestration, SDK manifests, and generated output. Its sysroot is staged at
`../toolchain/build/sysroot/` and must never be copied back into this
repository or committed as a second header tree.

This directory publishes the frozen first syscall ABI surface, including the
reserved process-I/O extension calls. The header is
freestanding and provides the x86_64 `syscall` and AArch64 `svc` register
shims, syscall numbers, six-word argument wrappers, and negative-errno
decoding.

The runtime and Rust package recipes live next to their sources in
[`runtime/gamma.toml`](runtime/gamma.toml) and
[`rust/gamma.toml`](rust/gamma.toml). SDK packaging belongs to
[`../toolchain/gamma.toml`](../toolchain/gamma.toml); there is intentionally no
second root-level SDK recipe here.

The smoke example uses the implemented runtime calls `getpid`, `gettid`,
`yield`, `read`, `write`, and `exit`. `read` is currently a bounded,
nonblocking serial-console poll on fd 0; fd 1/2 write to the kernel console.

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

The static Rust userspace fixtures, including the target `nsh` ELF, are linked
and staged by `toolchain/scripts/build-rust-userspace.py`. The interactive
serial smoke remains an explicit QEMU gate. Bounded process-local
filesystem-backed descriptors (`open`, `pipe`, `dup2`, and `close`) plus the
serial TTY ownership calls are implemented; `spawn2` now has a bounded static
image preparation path with cooperative user continuation, while arbitrary
argv/environment transfer, blocking streams, and full process/job-control
execution remain staged work.
