" Put this file in ~/.vim/after/syntax/
" for Telos syntax highlighting

" integer types
syn keyword telosType s8 s16 s32 s64
syn keyword telosType u8 u16 u32 u64
syn keyword telosType uchar ushort ulong

" Telos '*_t' types
syn keyword telosType pid_t port_t dev_t isr_t pte_t pmap_t
syn keyword telosType list_t list_head_t list_chain_t list_entry_t

hi def link telosType Type

" keyword-like macros
syn keyword telosMacro dequeue_iterate list_for_each list_for_each_prev list_for_each_safe list_for_each_prev_safe list_for_each_entry list_for_each_entry_reverse list_for_each_entry_continue list_for_each_entry_continue_reverse list_for_each_entry_from list_for_each_entry_safe list_for_each_entry_safe_continue list_for_each_entry_safe_from list_for_each_entry_safe_reverse hlist_for_each hlist_for_each_safe hlist_for_each_entry hlist_for_each_entry_continue hlist_for_each_entry_from hlist_for_each_entry_safe
hi def link telosMacro Statement

" symbolic constants
syn keyword telosConst STATE_STOPPED STATE_RUNNING STATE_READY STATE_BLOCKED STATE_SIGWAIT STATE_SIGSUSPEND STATE_SLEEPING
syn keyword telosConst PFLAG_SUPER
syn keyword telosConst SYS_CREATE SYS_YIELD SYS_STOP SYS_GETPID SYS_SLEEP SYS_SIGRETURN SYS_KILL SYS_SIGQUEUE SYS_SIGWAIT SYS_SIGACTION SYS_SIGNAL SYS_SIGMASK SYS_SEND SYS_RECV SYS_REPLY SYS_OPEN SYS_CLOSE SYS_READ SYS_WRITE SYS_IOCTL SYS_ALARM SYS_MALLOC SYS_FREE SYS_PALLOC
syn keyword telosConst EXN_DBZ EXN_ILLOP EXN_GP EXN_PF INTR_TIMER INTR_KBD EXN_FPE EXN_ILL INTR_SYSCALL
syn keyword telosConst SEGNR_NULL SEGNR_KCODE SEGNR_KDATA SEGNR_UCODE SEGNR_UDATA SEGNR_TSS
syn keyword telosConst SEG_KCODE SEG_KDATA SEG_UCODE SEG_UDATA SEG_TSS

hi def link telosConst Constant

" these constants are missing from the default
" C syntax file for whatever reason
syn keyword POSIXConst EADDRINUSE EADDRNOTAVAIL EAFNOSUPPORT EALREADY ECONNABORTED ECONNREFUSED ECONNRESET EDESTADDRREQ EDQUOT EHOSTUNREACH EIDRM EISCONN ELOOP EMULTIHOP ENETDOWN ENETRESET ENETUNREACH ENOBUFS ENODATA ENOLINK ENOMSG ENOPROTOOPT ENOSR ENOSTR ENOTCONN ENOTSOCK EOPNOTSUPP EOVERFLOW EPROTO EPROTONOSUPPORT EPROTOTYPE ESTALE ETIME ETXTBSY EWOULDBLOCK
syn keyword POSIXConst SIGBUS SIGPOLL SIGPROF SIGSYS SIGURG SIGVTALRM SIGXCPU SIGHOLD
syn keyword POSIXConst SIG_HOLD SIG_BLOCK SIG_SETMASK SIG_UNBLOCK
syn keyword POSIXConst SA_NOCLDSTOP SA_ONSTACK SA_RESETHAND SA_RESTART SA_SIGINFO SA_NOCLDWAIT SA_NODEFER
syn keyword POSIXConst SI_USER SI_QUEUE SI_TIMER SI_ASYNCIO SI_MESGQ

hi def link POSIXConst Constant
