# Nordix Rust userspace API

This `no_std` crate is the stable Rust-facing layer above the frozen syscall
ABI. It owns register-correct syscall wrappers, startup stack parsing, and a
small single-address-space bump allocator so `alloc::Vec` can be tested before
the kernel exposes a userspace heap syscall.

The staging package recipe is [`gamma.toml`](gamma.toml). It keeps the
target-aware build description separate from Cargo metadata and from the
installed manifest.

The build gate uses the pinned nightly with `build-std=core,alloc` and
`compiler_builtins` from the installed `rust-src`, then links a Rust ELF for
both custom target specs. The smoke binary checks allocation, startup parsing,
`getpid`, `gettid`, and `yield`; it is a link/API gate until Roadmap 5.1.6
enables actual userspace execution on QEMU.

The allocator is deliberately bounded to 64 KiB and never reclaims memory.
It is a bootstrap fixture, not the final process allocator. `std`, dynamic
linking, unwinding, and FP-dependent APIs remain outside the current ABI.
