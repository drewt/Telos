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

#define __need_size_t
#include <stddef.h>
#include <sys/type_defs.h>

#ifndef _PTHREAD_T_DEFINED
#define _PTHREAD_T_DEFINED
/*typedef _PTHREAD_T_TYPE pthread_t;*/
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

#define SIGABRT       1
#define SIGALRM       2
#define SIGBUS        3
#define SIGCHLD       4
#define SIGCONT       5
#define SIGFPE        6
#define SIGHUP        7
#define SIGILL        8
#define SIGINT        9
#define SIGPIPE       10
#define SIGQUIT       11
#define SIGSEGV       12
#define SIGTERM       13
#define SIGTSTP       14
#define SIGTTIN       15
#define SIGTTOU       16
#define SIGUSR1       17
#define SIGUSR2       18
#define SIGPOLL       19
#define SIGPROF       20
#define SIGSYS        21
#define SIGTRAP       22
#define SIGURG        23
#define SIGVTALRM     24
#define SIGXCPU       25
#define SIGXFSZ       26
#define SIGSTOP       27
#define SIGKILL       28
#define _TELOS_SIGMAX 29

#define SA_NOCLDSTOP  1
#define SA_ONSTACK    2
#define SA_RESETHAND  4
#define SA_RESTART    8
#define SA_SIGINFO    16
#define SA_NOCLDWAIT  32
#define SA_NODEFER    64

#define ILL_ILLOPC    1
#define ILL_ILLOPN    2
#define ILL_ILLADR    3
#define ILL_ILLTRP    4
#define ILL_PRVOPC    5
#define ILL_PRVREG    6
#define ILL_COPROC    7
#define ILL_BADSTK    8
#define FPE_INTDIV    9
#define FPE_INTOVF    10
#define FPE_FLTDIV    11
#define FPE_FLTOVF    12
#define FPE_FLTUND    13
#define FPE_FLTRES    14
#define FPE_FLTINV    15
#define FPE_FLTSUB    16
#define SEGV_MAPERR   17
#define SEGV_ACCERR   18
#define BUS_ADRALN    19
#define BUS_ADRERR    20
#define BUS_OBJERR    21
#define TRAP_BRKPT    22
#define TRAP_TRACE    23
#define CLD_EXITED    24
#define CLD_KILLED    25
#define CLD_DUMPED    26
#define CLD_TRAPED    27
#define CLD_STOPPED   28
#define CLD_CONTINUED 29
#define POLL_IN       30
#define POLL_OUT      31
#define POLL_MSG      32
#define POLL_ERR      33
#define POLL_PRI      34
#define POLL_HUP      35

#define SI_USER       1
#define SI_QUEUE      2
#define SI_TIMER      3
#define SI_ASYNCIO    4
#define SI_MESGQ      5

#define SIGEV_NONE   0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

#define SIG_BLOCK   0
#define SIG_SETMASK 1
#define SIG_UNBLOCK 2

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
