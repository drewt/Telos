/*  Copyright 2013 Drew Thoreson
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

#ifndef _KERNEL_SIGNAL_H_
#define _KERNEL_SIGNAL_H_

#include <kernel/sigdefs.h>

#define SIG_DFL  ((void(*)(int)) 1)
#define SIG_ERR  ((void(*)(int)) 2)
#define SIG_HOLD ((void(*)(int)) 3)
#define SIG_IGN  ((void(*)(int)) 4)

enum sig_nums {
	SIGABRT = 1,
	SIGALRM,
	SIGBUS,
	SIGCHLD,
	SIGCONT,
	SIGFPE,
	SIGHUP,
	SIGILL,
	SIGINT,
	SIGPIPE,
	SIGQUIT,
	SIGSEGV,
	SIGTERM,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	SIGUSR1,
	SIGUSR2,
	SIGPOLL,
	SIGPROF,
	SIGSYS,
	SIGTRAP,
	SIGURG,
	SIGVTALRM,
	SIGXCPU,
	SIGXFSZ,
	SIGSTOP,
	SIGKILL,
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
	SI_USER,
	SI_QUEUE,
	SI_TIMER,
	SI_ASYNCIO,
	SI_MESGQ
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
	*set = (1 << (_TELOS_SIGMAX - 1)) - 1;
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
	*set |= (1 << signum);
	return 0;
}

static inline int sigdelset(sigset_t *set, int signum)
{
#ifndef __KERNEL__
	if (!__signo_valid(signum))
		return -1;
#endif
	*set &= ~(1 << signum);
	return 0;
}

static inline int sigismember(const sigset_t *set, int signum)
{
#ifndef __KERNEL__
	if (!__signo_valid(signum))
		return -1;
#endif
	return *set & (1 << signum);
}

#ifdef __KERNEL__

#define signo_valid __signo_valid

struct sig_struct {
	sigset_t pending;
	sigset_t mask;
	sigset_t restore;
	struct sigaction actions[_TELOS_SIGMAX];
	struct siginfo infos[_TELOS_SIGMAX];
};
void sig_init(struct sig_struct *sig);
void sig_clone(struct sig_struct *dst, struct sig_struct *src);
void sig_exec(struct sig_struct *sig);

static inline int signal_ignored(struct sig_struct *sig, int sig_no)
{
	return sig->actions[sig_no].sa_handler == SIG_IGN;
}

static inline int signal_blocked(struct sig_struct *sig, int sig_no)
{
	return !sigismember(&sig->mask, sig_no);
}

static inline int signal_accepted(struct sig_struct *sig, int sig_no)
{
	return !signal_ignored(sig, sig_no) && !signal_blocked(sig, sig_no);
}

#endif /* __KERNEL__ */
#endif
