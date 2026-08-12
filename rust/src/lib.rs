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
        Open = 2,
        Pipe = 22,
        Dup2 = 33,
        Spawn = 400,
        WaitStatus = 402,
        Spawn2 = 401,
        SetProcessGroup = 403,
        GetProcessGroup = 404,
        KillProcessGroup = 405,
        TtyGetForeground = 406,
        TtySetForeground = 407,
        TtyGetInfo = 408,
        TtySetWindow = 409,
        SetSession = 410,
        GetCredentials = 411,
        Mkdir = 420,
        Rmdir = 421,
        Unlink = 422,
        Rename = 423,
        Link = 424,
        Stat = 425,
        ReadDir = 426,
        Fsync = 427,
        SyncPath = 428,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct PipeFds {
        pub read: Word,
        pub write: Word,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct WaitStatus {
        pub kind: u32,
        pub code: i32,
        pub signal: u32,
        pub reserved: u32,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct SpawnSpec {
        pub path: Word,
        pub path_length: Word,
        pub argv: Word,
        pub argc: Word,
        pub environment: Word,
        pub environment_count: Word,
        pub stdin_fd: Word,
        pub stdout_fd: Word,
        pub stderr_fd: Word,
        pub process_group: Word,
        pub flags: Word,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct Credentials {
        pub real_uid: u32,
        pub effective_uid: u32,
        pub saved_uid: u32,
        pub real_gid: u32,
        pub effective_gid: u32,
        pub saved_gid: u32,
        pub capabilities: u64,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct SessionSpec {
        pub credentials: Credentials,
        pub cwd: Word,
        pub cwd_length: Word,
        pub umask: u32,
        pub reserved: u32,
        pub max_fds: u32,
        pub max_address_space_pages: u32,
        pub max_cpu_ticks: u64,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub struct Stat {
        pub kind: u32,
        pub mode: u32,
        pub size: u64,
        pub links: u32,
        pub inode: u32,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
    pub struct TtyInfo {
        pub flags: u32,
        pub controlling_process: u32,
        pub foreground_group: u32,
        pub columns: u16,
        pub rows: u16,
        pub reserved: u32,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct DirEntry {
        pub kind: u32,
        pub mode: u32,
        pub size: u64,
        pub links: u32,
        pub name_length: u32,
        pub name: [u8; 32],
    }

    impl Default for DirEntry {
        fn default() -> Self {
            Self {
                kind: 0,
                mode: 0,
                size: 0,
                links: 0,
                name_length: 0,
                name: [0; 32],
            }
        }
    }

    pub const OPEN_READ: Word = 1 << 0;
    pub const OPEN_WRITE: Word = 1 << 1;
    pub const OPEN_CREATE: Word = 1 << 2;
    pub const OPEN_TRUNCATE: Word = 1 << 3;
    pub const OPEN_APPEND: Word = 1 << 4;
    pub const OPEN_EXCLUSIVE: Word = 1 << 5;
    pub const STAT_REGULAR: u32 = 1;
    pub const STAT_DIRECTORY: u32 = 2;
    pub const NAME_MAX: usize = 31;
    pub const PIPE_NONBLOCK: Word = 1 << 0;
    pub const SPAWN_NEW_PROCESS_GROUP: Word = 1 << 0;
    pub const SPAWN_FOREGROUND: Word = 1 << 1;
    pub const SPAWN_INHERIT_CREDENTIALS: Word = 1 << 2;
    pub const WAIT_NONBLOCK: Word = 1 << 0;
    pub const WAIT_EXITED: u32 = 1;
    pub const WAIT_SIGNALED: u32 = 2;
    pub const WAIT_STOPPED: u32 = 3;
    pub const WAIT_CONTINUED: u32 = 4;
    pub const TTY_FLAG_AVAILABLE: u32 = 1 << 0;
    pub const TTY_FLAG_SERIAL: u32 = 1 << 1;

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
            lateout("x9") _,
            lateout("x10") _,
            lateout("x11") _,
            lateout("x12") _,
            lateout("x13") _,
            lateout("x14") _,
            lateout("x15") _,
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

    pub fn open(path: *const u8, length: usize, flags: Word, mode: Word) -> Result<Word, Error> {
        result(unsafe {
            raw6(
                Number::Open as Word,
                [path as Word, length as Word, flags, mode, 0, 0],
            )
        })
    }

    pub fn mkdir(path: *const u8, length: usize, mode: Word) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Mkdir as Word,
                [path as Word, length as Word, mode, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn rmdir(path: *const u8, length: usize) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Rmdir as Word,
                [path as Word, length as Word, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn unlink(path: *const u8, length: usize) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Unlink as Word,
                [path as Word, length as Word, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn rename(
        old_path: *const u8,
        old_length: usize,
        new_path: *const u8,
        new_length: usize,
    ) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Rename as Word,
                [
                    old_path as Word,
                    old_length as Word,
                    new_path as Word,
                    new_length as Word,
                    0,
                    0,
                ],
            )
        })
        .map(|_| ())
    }

    pub fn link(
        old_path: *const u8,
        old_length: usize,
        new_path: *const u8,
        new_length: usize,
    ) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Link as Word,
                [
                    old_path as Word,
                    old_length as Word,
                    new_path as Word,
                    new_length as Word,
                    0,
                    0,
                ],
            )
        })
        .map(|_| ())
    }

    pub fn stat(path: *const u8, length: usize, output: *mut Stat) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::Stat as Word,
                [path as Word, length as Word, output as Word, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn read_dir(
        path: *const u8,
        length: usize,
        output: *mut DirEntry,
        capacity: usize,
    ) -> Result<usize, Error> {
        result(unsafe {
            raw6(
                Number::ReadDir as Word,
                [
                    path as Word,
                    length as Word,
                    output as Word,
                    capacity as Word,
                    0,
                    0,
                ],
            )
        })
        .map(|value| value as usize)
    }

    pub fn fsync(fd: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::Fsync as Word, [fd, 0, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn sync_path(path: *const u8, length: usize) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::SyncPath as Word,
                [path as Word, length as Word, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn pipe(fds: *mut PipeFds, flags: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::Pipe as Word, [fds as Word, flags, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn dup2(old_fd: Word, new_fd: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::Dup2 as Word, [old_fd, new_fd, 0, 0, 0, 0]) })
    }

    pub fn wait_status(child: Word, status: *mut WaitStatus, options: Word) -> Result<Word, Error> {
        result(unsafe {
            raw6(
                Number::WaitStatus as Word,
                [child, status as Word, options, 0, 0, 0],
            )
        })
    }

    pub fn wait(child: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::Wait as Word, [child, 0, 0, 0, 0, 0]) })
    }

    pub fn spawn2(spec: *const SpawnSpec) -> Result<Word, Error> {
        result(unsafe { raw6(Number::Spawn2 as Word, [spec as Word, 0, 0, 0, 0, 0]) })
    }

    pub fn set_session(spec: *const SessionSpec) -> Result<(), Error> {
        result(unsafe { raw6(Number::SetSession as Word, [spec as Word, 0, 0, 0, 0, 0]) })
            .map(|_| ())
    }

    pub fn get_credentials(output: *mut Credentials) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::GetCredentials as Word,
                [output as Word, 0, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn set_process_group(process: Word, group: Word) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::SetProcessGroup as Word,
                [process, group, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn process_group(process: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::GetProcessGroup as Word, [process, 0, 0, 0, 0, 0]) })
    }

    pub fn kill_process_group(group: Word, signal: Word) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::KillProcessGroup as Word,
                [group, signal, 0, 0, 0, 0],
            )
        })
        .map(|_| ())
    }

    pub fn tty_foreground(fd: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::TtyGetForeground as Word, [fd, 0, 0, 0, 0, 0]) })
    }

    pub fn tty_set_foreground(fd: Word, group: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::TtySetForeground as Word, [fd, group, 0, 0, 0, 0]) })
            .map(|_| ())
    }

    pub fn tty_get_info(fd: Word, output: *mut TtyInfo) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::TtyGetInfo as Word,
                [
                    fd,
                    output as Word,
                    core::mem::size_of::<TtyInfo>() as Word,
                    0,
                    0,
                    0,
                ],
            )
        })
        .map(|_| ())
    }

    pub fn tty_set_window(fd: Word, columns: u16, rows: u16) -> Result<(), Error> {
        result(unsafe {
            raw6(
                Number::TtySetWindow as Word,
                [fd, columns as Word, rows as Word, 0, 0, 0],
            )
        })
        .map(|_| ())
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
    bytes: UnsafeCell<[u8; 256 * 1024]>,
    cursor: AtomicUsize,
}

unsafe impl Sync for Heap {}

struct BumpAllocator;

static HEAP: Heap = Heap {
    bytes: UnsafeCell::new([0; 256 * 1024]),
    cursor: AtomicUsize::new(0),
};

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let base = HEAP.bytes.get() as *mut u8 as usize;
        let cursor = HEAP.cursor.load(Ordering::Relaxed);
        let start = base + cursor;
        let aligned = (start + layout.align() - 1) & !(layout.align() - 1);
        let offset = aligned - base;
        let end = match offset.checked_add(layout.size()) {
            Some(end) if end <= 256 * 1024 => end,
            _ => return core::ptr::null_mut(),
        };
        if HEAP
            .cursor
            .compare_exchange(cursor, end, Ordering::AcqRel, Ordering::Relaxed)
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
