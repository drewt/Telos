" Put this file in ~/.vim/after/syntax/
" for Telos syntax highlighting

" integer types
syn keyword telosType s8 s16 s32 s64
syn keyword telosType u8 u16 u32 u63
syn keyword telosType uchar ushort ulong

" Telos '*_t' types
syn keyword telosType pid_t port_t dev_t isr_t
syn keyword telosType list_t list_head_t list_chain_t list_entry_t

hi def link telosType Type

" keyword-like macros
syn keyword telosMacro list_iterate dequeue_iterate
hi def link telosMacro Statement

" symbolic constants
syn keyword telosConst STATE_STOPPED STATE_RUNNING STATE_READY STATE_BLOCKED STATE_SIGWAIT STATE_SIGSUSPEND STATE_SLEEPING
syn keyword telosConst PFLAG_SUPER
syn keyword telosConst SYS_CREATE SYS_YIELD SYS_STOP SYS_GETPID SYS_SLEEP SYS_SIGRETURN SYS_KILL SYS_SIGWAIT SYS_SIGACTION SYS_SIGNAL SYS_SIGMASK SYS_SEND SYS_RECV SYS_REPLY SYS_OPEN SYS_CLOSE SYS_READ SYS_WRITE SYS_IOCTL SYS_ALARM SYS_MALLOC SYS_FREE SYS_PALLOC
syn keyword telosConst DBZ_EXN ILLOP_EXN GP_EXN PF_EXN TIMER_INTR KBD_INTR FPE_EXN ILL_EXN SYSCALL_INTR
syn keyword telosConst NULL_SEGN KCODE_SEGN KDATA_SEGN UCODE_SEGN UDATA_SEGN TSS_SEGN
syn keyword telosConst SEG_KCODE SEG_KDATA SEG_UCODE SEG_UDATA SEG_TSS

hi def link telosConst Constant

" these constants are missing from the default
" C syntax file for whatever reason
syn keyword POSIXConst EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EALREADY ECONNABORTED ECONNREFUSED ECONNRESET EDESTADDRREQ EDQUOT EHOSTUNREACH EIDRM EISCONN ELOOP EMULTIHOP ENETDOWN ENETRESET ENETUNREACH ENOBUFS ENODATA ENOLINK ENOMSG ENOPROTOOPT ENOSR ENOSTR ENOTCONN ENOTSOCK EOPNOTSUPP EOVERFLOW EPROTO EPROTONOSUPPORT EPROTOTYPE ESTALE ETIME ETXTBSY EWOULDBLOCK
syn keyword POSIXConst SIGBUS SIGPOLL SIGPROF SIGSYS SIGURG SIGVTALRM SIGXCPU SIGHOLD SA_NOCLDSTOP SA_ONSTACK SA_RESETHAND SA_RESTART SA_SIGINFO SA_NOCLDWAIT SA_NODEFER

hi def link POSIXConst Constant
