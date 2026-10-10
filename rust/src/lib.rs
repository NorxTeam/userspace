#![no_std]

extern crate alloc;
#[cfg(test)]
extern crate std;

use core::alloc::{GlobalAlloc, Layout};
use core::cell::UnsafeCell;
use core::sync::atomic::{AtomicUsize, Ordering};

pub mod syscall {
    use core::arch::asm;

    pub type Word = u64;

    include!(concat!(env!("OUT_DIR"), "/syscall_number.rs"));

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
        pub abi_version: u16,
        pub size: u16,
        pub reserved0: u32,
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
        pub reserved: u32,
        pub reserved1: u32,
    }

    #[repr(C)]
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct DelegatedSpawnSpec {
        pub abi_version: u16,
        pub size: u16,
        pub reserved0: u32,
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
        pub target_uid: u32,
        pub target_gid: u32,
        pub reserved: u32,
        pub reserved1: u32,
        pub capabilities: u64,
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
        pub abi_version: u16,
        pub size: u16,
        pub reserved0: u32,
        pub credentials: Credentials,
        pub cwd: Word,
        pub cwd_length: Word,
        pub umask: u32,
        pub reserved: u32,
        pub max_fds: u32,
        pub max_address_space_pages: u32,
        pub max_cpu_ticks: u64,
    }

    pub const ABI_VERSION: u16 = 2;
    pub const SPAWN_SPEC_SIZE: u16 = core::mem::size_of::<SpawnSpec>() as u16;
    pub const DELEGATED_SPAWN_SPEC_SIZE: u16 =
        core::mem::size_of::<DelegatedSpawnSpec>() as u16;
    pub const SESSION_SPEC_SIZE: u16 = core::mem::size_of::<SessionSpec>() as u16;

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
    pub const SPAWN_INHERIT_OPEN_FDS: Word = 1 << 3;
    pub const SPAWN2_SUPPORTED_FLAGS: Word = SPAWN_NEW_PROCESS_GROUP
        | SPAWN_FOREGROUND
        | SPAWN_INHERIT_CREDENTIALS
        | SPAWN_INHERIT_OPEN_FDS;
    pub const CAP_PRIVILEGE_DELEGATION: u64 = 1 << 7;
    pub const WAIT_NONBLOCK: Word = 1 << 0;
    pub const WAIT_EXITED: u32 = 1;
    pub const WAIT_SIGNALED: u32 = 2;
    pub const WAIT_STOPPED: u32 = 3;
    pub const WAIT_CONTINUED: u32 = 4;
    pub const TTY_FLAG_AVAILABLE: u32 = 1 << 0;
    pub const TTY_FLAG_SERIAL: u32 = 1 << 1;
    pub const SEEK_SET: Word = 0;
    pub const SEEK_CUR: Word = 1;
    pub const SEEK_END: Word = 2;
    pub const F_GETFD: Word = 1;
    pub const F_SETFD: Word = 2;
    pub const FD_CLOEXEC: Word = 1 << 0;

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    pub struct Error(pub u64);

    impl Error {
        pub const fn errno(self) -> u64 {
            0u64.wrapping_sub(self.0)
        }
    }

    /// A kernel-issued session authority. The raw handle is intentionally not
    /// constructible or inspectable outside this module; every use is
    /// revalidated by the kernel against its live session generation.
    #[derive(Clone, Copy, PartialEq, Eq)]
    pub struct SessionCapability(Word);

    impl SessionCapability {
        pub fn acquire() -> Result<Self, Error> {
            result(unsafe { raw6(Number::GetSessionCapability as Word, [0; 6]) })
                .map(Self)
        }

        pub fn revalidate(
            self,
            required_capabilities: u64,
            target_uid: u32,
        ) -> Result<(), Error> {
            result(unsafe {
                raw6(
                    Number::ValidateSessionCapability as Word,
                    [self.0, required_capabilities, target_uid as Word, 0, 0, 0],
                )
            })
            .map(|_| ())
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

    pub fn lseek(fd: Word, offset: i64, whence: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::Lseek as Word, [fd, offset as Word, whence, 0, 0, 0]) })
    }

    pub fn fstat(fd: Word, output: *mut Stat) -> Result<(), Error> {
        result(unsafe { raw6(Number::Fstat as Word, [fd, output as Word, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn fchmod(fd: Word, mode: Word) -> Result<(), Error> {
        result(unsafe { raw6(Number::Fchmod as Word, [fd, mode, 0, 0, 0, 0]) }).map(|_| ())
    }

    pub fn fcntl(fd: Word, command: Word, argument: Word) -> Result<Word, Error> {
        result(unsafe { raw6(Number::Fcntl as Word, [fd, command, argument, 0, 0, 0]) })
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

    pub fn spawn_delegated(spec: *const DelegatedSpawnSpec) -> Result<Word, Error> {
        result(unsafe {
            raw6(
                Number::SpawnDelegated as Word,
                [spec as Word, 0, 0, 0, 0, 0],
            )
        })
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

    pub fn get_session_capability() -> Result<SessionCapability, Error> {
        SessionCapability::acquire()
    }

    pub fn validate_session_capability(
        capability: SessionCapability,
        required_capabilities: u64,
        target_uid: u32,
    ) -> Result<(), Error> {
        capability.revalidate(required_capabilities, target_uid)
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

const STARTUP_MAGIC: u64 = 0x4e4f525853544152;
pub const STARTUP_VERSION: u64 = 1;
const STARTUP_HEADER_WORDS: usize = 12;
const STARTUP_HEADER_BYTES: usize = STARTUP_HEADER_WORDS * core::mem::size_of::<u64>();
pub const STARTUP_MAX_BYTES: usize = 4096;
pub const STARTUP_MAX_ARGUMENTS: usize = 16;
pub const STARTUP_MAX_ENVIRONMENT: usize = 16;
const STARTUP_AUXV_ENTRIES: usize = 3;
const AT_PAGESZ: u64 = 6;
const AT_ENTRY: u64 = 9;
const PAGE_SIZE: u64 = 4096;

#[repr(C)]
#[derive(Clone, Copy)]
struct StartupBlock {
    magic: u64,
    version: u64,
    header_size: u64,
    block_size: u64,
    argc: u64,
    envc: u64,
    argv_offset: u64,
    envp_offset: u64,
    auxv_offset: u64,
    auxv_count: u64,
    strings_offset: u64,
    strings_size: u64,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum StartupError {
    NullStack,
    MisalignedStack,
    InvalidHeader,
    UnsupportedVersion,
    InvalidSize,
    TooManyArguments,
    InvalidLayout,
    InvalidAuxiliaryVector,
    InvalidPointer,
    UnterminatedString,
}

#[derive(Clone, Copy)]
pub struct Startup {
    pub argc: usize,
    pub envc: usize,
    argv: usize,
    envp: usize,
    strings_end: usize,
}

pub unsafe fn startup_from_stack(stack: *const usize) -> Result<Startup, StartupError> {
    if stack.is_null() {
        return Err(StartupError::NullStack);
    }
    let base = stack as usize;
    if base & (core::mem::align_of::<StartupBlock>() - 1) != 0 {
        return Err(StartupError::MisalignedStack);
    }
    let header = stack.cast::<StartupBlock>().read();
    if header.magic != STARTUP_MAGIC {
        return Err(StartupError::InvalidHeader);
    }
    if header.version != STARTUP_VERSION {
        return Err(StartupError::UnsupportedVersion);
    }
    if header.header_size != STARTUP_HEADER_BYTES as u64 {
        return Err(StartupError::InvalidHeader);
    }
    let block_size = usize_from_startup_word(header.block_size).ok_or(StartupError::InvalidSize)?;
    if !(STARTUP_HEADER_BYTES..=STARTUP_MAX_BYTES).contains(&block_size) {
        return Err(StartupError::InvalidSize);
    }
    let block_end = base
        .checked_add(block_size)
        .ok_or(StartupError::InvalidSize)?;
    let argc = usize_from_startup_word(header.argc).ok_or(StartupError::TooManyArguments)?;
    let envc = usize_from_startup_word(header.envc).ok_or(StartupError::TooManyArguments)?;
    if argc > STARTUP_MAX_ARGUMENTS || envc > STARTUP_MAX_ENVIRONMENT {
        return Err(StartupError::TooManyArguments);
    }

    let argv_bytes = argc
        .checked_add(1)
        .and_then(|count| count.checked_mul(core::mem::size_of::<u64>()))
        .ok_or(StartupError::InvalidLayout)?;
    let envp_bytes = envc
        .checked_add(1)
        .and_then(|count| count.checked_mul(core::mem::size_of::<u64>()))
        .ok_or(StartupError::InvalidLayout)?;
    let auxv_bytes = STARTUP_AUXV_ENTRIES
        .checked_mul(2)
        .and_then(|count| count.checked_mul(core::mem::size_of::<u64>()))
        .ok_or(StartupError::InvalidLayout)?;
    let argv_offset = STARTUP_HEADER_BYTES;
    let envp_offset = argv_offset
        .checked_add(argv_bytes)
        .ok_or(StartupError::InvalidLayout)?;
    let auxv_offset = envp_offset
        .checked_add(envp_bytes)
        .ok_or(StartupError::InvalidLayout)?;
    if header.argv_offset != argv_offset as u64
        || header.envp_offset != envp_offset as u64
        || header.auxv_offset != auxv_offset as u64
        || header.auxv_count != STARTUP_AUXV_ENTRIES as u64
    {
        return Err(StartupError::InvalidLayout);
    }
    let argv = bounded_array(base, header.argv_offset, argv_bytes, block_end)?;
    let envp = bounded_array(base, header.envp_offset, envp_bytes, block_end)?;
    let auxv = bounded_array(base, header.auxv_offset, auxv_bytes, block_end)?;

    let strings_offset = usize_from_startup_word(header.strings_offset)
        .ok_or(StartupError::InvalidLayout)?;
    let strings_size = usize_from_startup_word(header.strings_size)
        .ok_or(StartupError::InvalidLayout)?;
    if strings_offset < auxv_offset + auxv_bytes
        || strings_offset > block_size
        || strings_size != block_size - strings_offset
    {
        return Err(StartupError::InvalidLayout);
    }
    let strings_start = base
        .checked_add(strings_offset)
        .ok_or(StartupError::InvalidLayout)?;
    let strings_end = strings_start
        .checked_add(strings_size)
        .ok_or(StartupError::InvalidLayout)?;
    if strings_end != block_end {
        return Err(StartupError::InvalidLayout);
    }

    if read_word(auxv, 0) != AT_PAGESZ
        || read_word(auxv, 1) != PAGE_SIZE
        || read_word(auxv, 2) != AT_ENTRY
        || read_word(auxv, 3) == 0
        || read_word(auxv, 4) != 0
        || read_word(auxv, 5) != 0
    {
        return Err(StartupError::InvalidAuxiliaryVector);
    }
    for index in 0..argc {
        validate_string(read_word(argv, index), strings_start, strings_end)?;
    }
    if read_word(argv, argc) != 0 {
        return Err(StartupError::InvalidLayout);
    }
    for index in 0..envc {
        validate_string(read_word(envp, index), strings_start, strings_end)?;
    }
    if read_word(envp, envc) != 0 {
        return Err(StartupError::InvalidLayout);
    }
    Ok(Startup {
        argc,
        envc,
        argv,
        envp,
        strings_end,
    })
}

fn usize_from_startup_word(value: u64) -> Option<usize> {
    (value <= usize::MAX as u64).then_some(value as usize)
}

fn bounded_array(
    base: usize,
    offset: u64,
    length: usize,
    block_end: usize,
) -> Result<usize, StartupError> {
    let offset = usize_from_startup_word(offset).ok_or(StartupError::InvalidLayout)?;
    if !offset.is_multiple_of(core::mem::size_of::<u64>()) {
        return Err(StartupError::InvalidLayout);
    }
    let start = base
        .checked_add(offset)
        .ok_or(StartupError::InvalidLayout)?;
    let end = start
        .checked_add(length)
        .ok_or(StartupError::InvalidLayout)?;
    if end > block_end {
        return Err(StartupError::InvalidLayout);
    }
    Ok(start)
}

unsafe fn read_word(address: usize, index: usize) -> u64 {
    let offset = index
        .checked_mul(core::mem::size_of::<u64>())
        .expect("validated startup array index");
    core::ptr::read_unaligned((address + offset) as *const u64)
}

unsafe fn validate_string(
    pointer: u64,
    strings_start: usize,
    strings_end: usize,
) -> Result<(), StartupError> {
    let pointer = usize_from_startup_word(pointer).ok_or(StartupError::InvalidPointer)?;
    if pointer < strings_start || pointer >= strings_end {
        return Err(StartupError::InvalidPointer);
    }
    let mut cursor = pointer;
    while cursor < strings_end {
        if (cursor as *const u8).read() == 0 {
            return Ok(());
        }
        cursor += 1;
    }
    Err(StartupError::UnterminatedString)
}

const HEAP_BYTES: usize = 256 * 1024;
const MAX_ALLOC_RETRIES: usize = 8;

struct Heap {
    bytes: UnsafeCell<[u8; HEAP_BYTES]>,
    cursor: AtomicUsize,
}

unsafe impl Sync for Heap {}

impl Heap {
    const fn new() -> Self {
        Self {
            bytes: UnsafeCell::new([0; HEAP_BYTES]),
            cursor: AtomicUsize::new(0),
        }
    }

    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let base = self.bytes.get() as *mut u8 as usize;
        let alignment_mask = layout.align() - 1;
        for _ in 0..MAX_ALLOC_RETRIES {
            let cursor = self.cursor.load(Ordering::Relaxed);
            let Some(start) = base.checked_add(cursor) else {
                return core::ptr::null_mut();
            };
            let Some(aligned) = start
                .checked_add(alignment_mask)
                .map(|value| value & !alignment_mask)
            else {
                return core::ptr::null_mut();
            };
            let Some(offset) = aligned.checked_sub(base) else {
                return core::ptr::null_mut();
            };
            let Some(end) = offset.checked_add(layout.size()) else {
                return core::ptr::null_mut();
            };
            if end > HEAP_BYTES {
                return core::ptr::null_mut();
            }
            if self
                .cursor
                .compare_exchange(cursor, end, Ordering::AcqRel, Ordering::Relaxed)
                .is_ok()
            {
                return aligned as *mut u8;
            }
            core::hint::spin_loop();
        }
        core::ptr::null_mut()
    }
}

struct BumpAllocator;

static HEAP: Heap = Heap::new();

unsafe impl GlobalAlloc for BumpAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        HEAP.alloc(layout)
    }

    unsafe fn dealloc(&self, _pointer: *mut u8, _layout: Layout) {}
}

#[global_allocator]
static ALLOCATOR: BumpAllocator = BumpAllocator;

#[cfg(test)]
mod tests {
    use super::*;
    use core::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

    #[test]
    fn startup_rejects_malformed_blocks_before_pointer_walk() {
        let mut words = [0u64; 24];
        let base = words.as_ptr() as usize;
        words[..12].copy_from_slice(&[
            STARTUP_MAGIC, STARTUP_VERSION, 96, 170, 1, 0, 96, 112, 120, 3, 168, 2,
        ]);
        words[12] = (base + 168) as u64;
        words[15..21].copy_from_slice(&[AT_PAGESZ, PAGE_SIZE, AT_ENTRY, 1, 0, 0]);
        words[21] = b'a' as u64;
        let valid = words;
        let parse = |block: &[u64; 24]| unsafe {
            startup_from_stack(block.as_ptr().cast()).map(|startup| (startup.argc, startup.envc))
        };
        assert_eq!(parse(&words), Ok((1, 0)));
        for (index, value, expected) in [
            (0, 0, StartupError::InvalidHeader),
            (1, STARTUP_VERSION + 1, StartupError::UnsupportedVersion),
            (3, u64::MAX, StartupError::InvalidSize),
            (4, 17, StartupError::TooManyArguments),
            (6, u64::MAX, StartupError::InvalidLayout),
            (12, (base + 167) as u64, StartupError::InvalidPointer),
            (13, 1, StartupError::InvalidLayout),
            (16, 0, StartupError::InvalidAuxiliaryVector),
            (21, 0x6161, StartupError::UnterminatedString),
        ] {
            words = valid;
            words[index] = value;
            assert_eq!(parse(&words), Err(expected));
        }
        assert!(matches!(unsafe { startup_from_stack(core::ptr::null()) }, Err(StartupError::NullStack)));
        assert!(matches!(unsafe { startup_from_stack((base + 1) as *const usize) }, Err(StartupError::MisalignedStack)));
    }

    #[test]
    fn allocator_claims_disjoint_ranges_under_contention() {
        const THREADS: usize = 8;
        const ALLOCATIONS: usize = 64;
        const ALLOCATION_SIZE: usize = 16;
        const SLOTS: usize = THREADS * ALLOCATIONS;
        static TEST_HEAP: Heap = Heap::new();

        let start = AtomicBool::new(false);
        let ready = AtomicUsize::new(0);
        let pointers = core::array::from_fn::<_, SLOTS, _>(|_| AtomicUsize::new(0));
        let start_ref = &start;
        let ready_ref = &ready;
        let pointers_ref = &pointers;

        std::thread::scope(|scope| {
            for thread_id in 0..THREADS {
                scope.spawn(move || {
                    ready_ref.fetch_add(1, Ordering::Relaxed);
                    while !start_ref.load(Ordering::Acquire) {
                        core::hint::spin_loop();
                    }
                    let layout = Layout::from_size_align(ALLOCATION_SIZE, 8).unwrap();
                    for allocation in 0..ALLOCATIONS {
                        let pointer = unsafe { TEST_HEAP.alloc(layout) };
                        if !pointer.is_null() {
                            pointers_ref[thread_id * ALLOCATIONS + allocation]
                                .store(pointer as usize, Ordering::Relaxed);
                        }
                    }
                });
            }
            while ready.load(Ordering::Acquire) != THREADS {
                core::hint::spin_loop();
            }
            start.store(true, Ordering::Release);
        });

        let mut addresses = [0usize; SLOTS];
        let mut count = 0;
        for pointer in pointers.iter() {
            let pointer = pointer.load(Ordering::Relaxed);
            if pointer != 0 {
                addresses[count] = pointer;
                count += 1;
            }
        }
        assert!(count >= THREADS);
        addresses[..count].sort_unstable();
        assert!(addresses[..count].windows(2).all(|pair| pair[0] != pair[1]));
        let cursor = TEST_HEAP.cursor.load(Ordering::Acquire);
        assert!(cursor >= count * ALLOCATION_SIZE);
        assert!(cursor <= HEAP_BYTES);
    }

    #[test]
    fn allocator_rejects_exhaustion_and_oversized_layouts() {
        static TEST_HEAP: Heap = Heap::new();
        let full = Layout::from_size_align(HEAP_BYTES, 1).unwrap();
        assert!(!unsafe { TEST_HEAP.alloc(full) }.is_null());
        assert!(unsafe { TEST_HEAP.alloc(Layout::from_size_align(1, 1).unwrap()) }.is_null());
        assert!(unsafe {
            TEST_HEAP.alloc(Layout::from_size_align(isize::MAX as usize, 1).unwrap())
        }
        .is_null());
    }
}
