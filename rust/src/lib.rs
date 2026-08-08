#![no_std]

extern crate alloc;

use core::alloc::{GlobalAlloc, Layout};
use core::cell::UnsafeCell;
use core::sync::atomic::{AtomicUsize, Ordering};

pub mod syscall {
    use core::arch::asm;

    pub type Word = u64;

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    #[repr(u64)]
    pub enum Number {
        Read = 0,
        Write = 1,
        Close = 3,
        Wait = 61,
        Exit = 60,
        GetPid = 39,
        GetTid = 186,
        Yield = 24,
        Sleep = 35,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct Error(pub u64);

    impl Error {
        pub const fn errno(self) -> u64 {
            0u64.wrapping_sub(self.0)
        }
    }

    pub const fn is_error(value: Word) -> bool {
        value >= Word::MAX - 4095
    }

    fn result(value: Word) -> Result<Word, Error> {
        if is_error(value) {
            Err(Error(value))
        } else {
            Ok(value)
        }
    }

    #[cfg(target_arch = "x86_64")]
    unsafe fn raw6(number: Word, args: [Word; 6]) -> Word {
        let mut result = number;
        asm!(
            "syscall",
            inlateout("rax") result,
            in("rdi") args[0],
            in("rsi") args[1],
            in("rdx") args[2],
            in("r10") args[3],
            in("r8") args[4],
            in("r9") args[5],
            lateout("rcx") _,
            lateout("r11") _,
            options(nostack)
        );
        result
    }

    #[cfg(target_arch = "aarch64")]
    unsafe fn raw6(number: Word, args: [Word; 6]) -> Word {
        let mut result = args[0];
        asm!(
            "svc #0",
            inlateout("x0") result,
            in("x1") args[1],
            in("x2") args[2],
            in("x3") args[3],
            in("x4") args[4],
            in("x5") args[5],
            in("x8") number,
            options(nostack)
        );
        result
    }

    pub fn getpid() -> Result<Word, Error> {
        result(unsafe { raw6(Number::GetPid as Word, [0; 6]) })
    }

    pub fn gettid() -> Result<Word, Error> {
        result(unsafe { raw6(Number::GetTid as Word, [0; 6]) })
    }

    pub fn yield_now() -> Result<(), Error> {
        result(unsafe { raw6(Number::Yield as Word, [0; 6]) }).map(|_| ())
    }

    pub fn sleep(duration: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::Sleep as Word, [duration, 0, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn write(fd: Word, buffer: *const u8, length: usize) -> Result<usize, Error> {
        result(unsafe {
            raw6(
                Number::Write as Word,
                [fd, buffer as Word, length as Word, 0, 0, 0],
            )
        })
        .map(|value| value as usize)
    }

    pub fn read(fd: Word, buffer: *mut u8, length: usize) -> Result<usize, Error> {
        result(unsafe {
            raw6(
                Number::Read as Word,
                [fd, buffer as Word, length as Word, 0, 0, 0],
            )
        })
        .map(|value| value as usize)
    }

    pub fn close(fd: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::Close as Word, [fd, 0, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn exit(status: i32) -> ! {
        let _ = unsafe { raw6(Number::Exit as Word, [status as u64, 0, 0, 0, 0, 0]) };
        loop {
            core::hint::spin_loop();
        }
    }
}

#[derive(Clone, Copy)]
pub struct Startup {
    pub argc: usize,
    pub argv: *const *const u8,
    pub envp: *const *const u8,
}

pub unsafe fn startup_from_stack(stack: *const usize) -> Startup {
    let argc = *stack;
    let argv = stack.add(1) as *const *const u8;
    let envp = argv.add(argc + 1);
    Startup { argc, argv, envp }
}

struct Heap {
    bytes: UnsafeCell<[u8; 64 * 1024]>,
    cursor: AtomicUsize,
}

unsafe impl Sync for Heap {}

struct BumpAllocator;

static HEAP: Heap = Heap {
    bytes: UnsafeCell::new([0; 64 * 1024]),
    cursor: AtomicUsize::new(0),
};

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let base = HEAP.bytes.get() as *mut u8 as usize;
        let start = base + HEAP.cursor.load(Ordering::Relaxed);
        let aligned = (start + layout.align() - 1) & !(layout.align() - 1);
        let offset = aligned - base;
        let end = match offset.checked_add(layout.size()) {
            Some(end) if end <= 64 * 1024 => end,
            _ => return core::ptr::null_mut(),
        };
        if HEAP
            .cursor
            .compare_exchange(offset, end, Ordering::AcqRel, Ordering::Relaxed)
            .is_ok()
        {
            aligned as *mut u8
        } else {
            self.alloc(layout)
        }
    }

    unsafe fn dealloc(&self, _pointer: *mut u8, _layout: Layout) {}
}

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator;
