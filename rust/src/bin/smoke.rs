#![feature(alloc_error_handler)]
#![no_std]
#![no_main]

extern crate alloc;

use alloc::vec::Vec;
use core::alloc::Layout;
use core::arch::asm;
use core::panic::PanicInfo;
use userspace::{startup_from_stack, syscall};

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    syscall::exit(101)
}

#[alloc_error_handler]
fn allocation_error(_layout: Layout) -> ! {
    syscall::exit(12)
}

#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    let stack: *const usize;
    #[cfg(target_arch = "x86_64")]
    unsafe {
        asm!("mov {}, rsp", out(reg) stack, options(nomem, nostack, preserves_flags));
    }
    #[cfg(target_arch = "aarch64")]
    unsafe {
        asm!("mov {}, sp", out(reg) stack, options(nomem, nostack, preserves_flags));
    }
    let startup = unsafe { startup_from_stack(stack) };
    let mut values = Vec::new();
    values.extend_from_slice(&[startup.argc as u64, 2, 3]);
    let status = if values == [startup.argc as u64, 2, 3]
        && syscall::getpid().is_ok()
        && syscall::gettid().is_ok()
        && syscall::yield_now().is_ok()
        && syscall::write(
            1,
            b"[   OK   ] userspace: rust-write\n".as_ptr(),
            b"[   OK   ] userspace: rust-write\n".len(),
        )
        .is_ok()
    {
        0
    } else {
        1
    };
    syscall::exit(status)
}
