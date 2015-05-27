/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYS_SIGNAL_H_
#define _SYS_SIGNAL_H_

#include <sys/type_defs.h>

#ifndef _PTHREAD_T_DEFINED
#define _PTHREAD_T_DEFINED
/*typedef _PTHREAD_T_TYPE pthread_t;*/
#endif
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef _SIZE_T_TYPE size_t;
#endif
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef _UID_T_TYPE uid_t;
#endif
#ifndef _SIG_ATOMIC_T_DEFINED
#define _SIG_ATOMIC_T_DEFINED
typedef _SIG_ATOMIC_T_TYPE sig_atomic_t;
#endif
#ifndef _SIGSET_T_DEFINED
#define _SIGSET_T_DEFINED
typedef _SIGSET_T_TYPE sigset_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef _TIME_T_TYPE time_t;
#endif
#ifndef _STRUCT_TIMESPEC_DEFINED
#define _STRUCT_TIMESPEC_DEFINED
_STRUCT_TIMESPEC_DEFN
#endif
#ifndef _UNION_SIGVAL_DEFINED
#define _UNION_SIGVAL_DEFINED
_UNION_SIGVAL_DEFN
#endif
#ifndef _SIGINFO_T_DEFINED
#define _SIGINFO_T_DEFINED
_SIGINFO_T_DEFN
#endif

typedef struct sigstack {
	int	ss_onstack; /* non-zero when signal stack is in use */
	void	*ss_sp;     /* stack base or pointer */
	size_t	ss_size;    /* stack size */
	int	ss_flags;   /* flags */
} stack_t;

struct sigevent {
	int		sigev_notify;
	int		sigev_signo;
	union sigval	sigev_value;
	void(*sigev_notify_function)(union sigval);
};

struct sigaction {
	union {
		void(*sa_handler)(int);
		void(*sa_sigaction)(int,siginfo_t*,void*);
	};
	sigset_t	sa_mask;
	int		sa_flags;
};

#define SIG_DFL  ((void(*)(int)) 1)
#define SIG_ERR  ((void(*)(int)) 2)
#define SIG_HOLD ((void(*)(int)) 3)
#define SIG_IGN  ((void(*)(int)) 4)

enum sig_nums {
	SIGABRT   = 1,
	SIGALRM   = 2,
	SIGBUS    = 3,
	SIGCHLD   = 4,
	SIGCONT   = 5,
	SIGFPE    = 6,
	SIGHUP    = 7,
	SIGILL    = 8,
	SIGINT    = 9,
	SIGPIPE   = 10,
	SIGQUIT   = 11,
	SIGSEGV   = 12,
	SIGTERM   = 13,
	SIGTSTP   = 14,
	SIGTTIN   = 15,
	SIGTTOU   = 16,
	SIGUSR1   = 17,
	SIGUSR2   = 18,
	SIGPOLL   = 19,
	SIGPROF   = 20,
	SIGSYS    = 21,
	SIGTRAP   = 22,
	SIGURG    = 23,
	SIGVTALRM = 24,
	SIGXCPU   = 25,
	SIGXFSZ   = 26,
	SIGSTOP   = 27,
	SIGKILL   = 28,
	_TELOS_SIGMAX
};

enum {
	SA_NOCLDSTOP = 1,
	SA_ONSTACK   = 1 << 1,
	SA_RESETHAND = 1 << 2,
	SA_RESTART   = 1 << 3,
	SA_SIGINFO   = 1 << 4,
	SA_NOCLDWAIT = 1 << 5,
	SA_NODEFER   = 1 << 6,
};

enum {
	ILL_ILLOPC,
	ILL_ILLOPN,
	ILL_ILLADR,
	ILL_ILLTRP,
	ILL_PRVOPC,
	ILL_PRVREG,
	ILL_COPROC,
	ILL_BADSTK,
	FPE_INTDIV,
	FPE_INTOVF,
	FPE_FLTDIV,
	FPE_FLTOVF,
	FPE_FLTUND,
	FPE_FLTRES,
	FPE_FLTINV,
	FPE_FLTSUB,
	SEGV_MAPERR,
	SEGV_ACCERR,
	BUS_ADRALN,
	BUS_ADRERR,
	BUS_OBJERR,
	TRAP_BRKPT,
	TRAP_TRACE,
	CLD_EXITED,
	CLD_KILLED,
	CLD_DUMPED,
	CLD_TRAPED,
	CLD_STOPPED,
	CLD_CONTINUED,
	POLL_IN,
	POLL_OUT,
	POLL_MSG,
	POLL_ERR,
	POLL_PRI,
	POLL_HUP,
	SI_USER,
	SI_QUEUE,
	SI_TIMER,
	SI_ASYNCIO,
	SI_MESGQ,
};

enum {
	SIGEV_NONE,
	SIGEV_SIGNAL,
	SIGEV_THREAD,
};

enum {
	SIG_BLOCK,
	SIG_SETMASK,
	SIG_UNBLOCK,
};

static inline int __signo_valid(int signo)
{
	return signo > 0 && signo < _TELOS_SIGMAX;
}

static inline int sigfillset(sigset_t *set)
{
	*set = ~0;
	return 0;
}

static inline int sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}

static inline int sigaddset(sigset_t *set, int signum)
{
#ifndef __KERNEL__
	if (!__signo_valid(signum))
		return -1;
#endif
	*set |= 1 << (signum-1);
	return 0;
}

static inline int sigdelset(sigset_t *set, int signum)
{
#ifndef __KERNEL__
	if (!__signo_valid(signum))
		return -1;
#endif
	*set &= ~(1 << (signum-1));
	return 0;
}

static inline int sigismember(const sigset_t *set, int signum)
{
#ifndef __KERNEL__
	if (!__signo_valid(signum))
		return -1;
#endif
	return *set & (1 << (signum-1));
}

#endif
