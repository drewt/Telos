" Put this file in ~/.vim/after/syntax/
" for Telos syntax highlighting

" integer types
syn keyword telosType s8 s16 s32 s64
syn keyword telosType u8 u16 u32 u64
syn keyword telosType uchar ushort uint ulong

" Telos '*_t' types
syn keyword telosType pid_t port_t dev_t isr_t pte_t pmap_t
syn keyword telosType list_t list_head_t list_chain_t list_entry_t

hi def link telosType Type

" keyword-like macros
syn keyword telosMacro dequeue_iterate list_for_each list_for_each_prev list_for_each_safe list_for_each_prev_safe list_for_each_entry list_for_each_entry_reverse list_for_each_entry_continue list_for_each_entry_continue_reverse list_for_each_entry_from list_for_each_entry_safe list_for_each_entry_safe_continue list_for_each_entry_safe_from list_for_each_entry_safe_reverse hlist_for_each hlist_for_each_safe hlist_for_each_entry hlist_for_each_entry_continue hlist_for_each_entry_from hlist_for_each_entry_safe hash_for_each hash_for_each_safe hash_for_each_possible hash_for_each_possible_safe for_each_set_bit for_each_set_bit_from for_each_clear_bit for_each_clear_bit_from
hi def link telosMacro Statement

" symbolic constants
syn keyword telosConst PROC_DEAD PROC_NASCENT PROC_READY PROC_RUNNING PROC_STOPPED PROC_ZOMBIE PROC_SIGWAIT PROC_SIGSUSPEND PROC_SLEEPING PROC_WAITING
syn keyword telosConst PFLAG_SUPER
syn keyword telosConst SYS_YIELD SYS_EXIT SYS_WAITID SYS_GETPID SYS_GETPPID SYS_SLEEP SYS_SIGRETURN SYS_KILL SYS_SIGQUEUE SYS_SIGWAIT SYS_SIGSUSPEND SYS_SIGACTION SYS_SIGMASK SYS_OPEN SYS_CLOSE SYS_READ SYS_PREAD SYS_WRITE SYS_LSEEK SYS_IOCTL SYS_ALARM SYS_TIMER_CREATE SYS_TIMER_DELETE SYS_TIMER_GETTIME SYS_TIMER_SETTIME SYS_TIME SYS_CLOCK_GETRES SYS_CLOCK_GETTIME SYS_CLOCK_SETTIME SYS_SBRK SYS_MMAP SYS_MOUNT SYS_UMOUNT SYS_MKDIR SYS_READDIR SYS_MKNOD SYS_RMDIR SYS_UNLINK SYS_LINK SYS_RENAME SYS_CHDIR SYS_STAT SYS_TRUNCATE SYS_FCNTL SYS_FORK SYS_EXECVE
syn keyword telosConst EXN_DBZ EXN_ILLOP EXN_GP EXN_PF INTR_TIMER INTR_KBD EXN_FPE EXN_ILL INTR_SYSCALL
syn keyword telosConst SEGNR_NULL SEGNR_KCODE SEGNR_KDATA SEGNR_UCODE SEGNR_UDATA SEGNR_TSS
syn keyword telosConst SEG_KCODE SEG_KDATA SEG_UCODE SEG_UDATA SEG_TSS
syn keyword telosConst VM_EXEC VM_WRITE VM_READ VM_ZERO VM_ALLOC VM_KEEP
hi def link telosConst Constant

" these constants are missing from the default
" C syntax file for whatever reason
syn keyword POSIXConst EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EALREADY ECONNABORTED ECONNREFUSED ECONNRESET EDESTADDRREQ EDQUOT EHOSTUNREACH EIDRM EISCONN ELOOP EMULTIHOP ENETDOWN ENETRESET ENETUNREACH ENOBUFS ENODATA ENOLINK ENOMSG ENOPROTOOPT ENOSR ENOSTR ENOTCONN ENOTSOCK EOPNOTSUPP EOVERFLOW EPROTO EPROTONOSUPPORT EPROTOTYPE ESTALE ETIME ETXTBSY EWOULDBLOCK
syn keyword POSIXConst SIGBUS SIGPOLL SIGPROF SIGSYS SIGURG SIGVTALRM SIGXCPU SIGHOLD
syn keyword POSIXConst SIG_HOLD SIG_BLOCK SIG_SETMASK SIG_UNBLOCK
syn keyword POSIXConst SA_NOCLDSTOP SA_ONSTACK SA_RESETHAND SA_RESTART SA_SIGINFO SA_NOCLDWAIT SA_NODEFER
syn keyword POSIXConst SI_USER SI_QUEUE SI_TIMER SI_ASYNCIO SI_MESGQ
syn keyword POSIXConst SIGEV_NONE SIGEV_SIGNAL SIGEV_THREAD
syn keyword POSIXConst CLOCK_REALTIME CLOCK_MONOTONIC
syn keyword POSIXConst F_DUPFD F_DUPFD_CLOEXEC F_GETFD F_SETFD F_GETFL F_SETFL F_GETLK F_SETLK F_SETLKW F_GETOWN F_SETOWN FD_CLOEXEC F_RDLCK F_UNLCK F_WRLCK
syn keyword POSIXConst O_RDONLY O_WRONLY O_RDWR O_EXEC O_SEARCH O_APPEND O_CLOEXEC O_CREAT O_DIRECTORY O_DSYNC O_EXCL O_NOCTTY O_NOFOLLOW O_NONBLOCK O_RSYNC O_SYNC O_TRUNC O_TTY_INIT
syn keyword POSIXConst PROT_NONE PROT_EXEC PROT_WRITE PROT_READ MAP_PRIVATE MAP_SHARED MAP_FIXED MAP_FAILED
syn keyword POSIXConst WCONTINUED WEXITED WSIGNALED WSTOPPED WNOHANG WUNTRACED WNOWAIT
syn keyword POSIXConst STDIN_FILENO STDOUT_FILENO STDERR_FILENO
hi def link POSIXConst Constant

syn keyword POSIXType pid_t clockid_t timer_t dev_t ino_t mode_t nlink_t uid_t gid_t blksize_t blkcnt_t
hi def link POSIXType Type
