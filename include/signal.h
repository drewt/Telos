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

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <sigdefs.h>

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
	SIGSTOP,
	SIGKILL,
	_TELOS_SIGMAX
};

enum {
	SA_NOCLDSTOP	= 1,
	SA_ONSTACK	= 1 << 1,
	SA_RESETHAND	= 1 << 2,
	SA_RESTART	= 1 << 3,
	SA_SIGINFO	= 1 << 4,
	SA_NOCLDWAIT	= 1 << 5,
	SA_NODEFER	= 1 << 6
};

enum {
	SI_USER,
	SI_QUEUE,
	SI_TIMER,
	SI_ASYNCIO,
	SI_MESGQ
};

enum sigprocmask_flags { SIG_BLOCK, SIG_SETMASK, SIG_UNBLOCK };

int kill(pid_t pid, int signal_number);
int sigqueue(pid_t pid, int signal_number, const union sigval value);
int sigaction(int sig, struct sigaction *act, struct sigaction *oact);
void(*signal(int sig, void(*func)(int)))(int);
int sigprocmask(int how, sigset_t *set, sigset_t *oset);
int sigwait(void);

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
	if (signum >= _TELOS_SIGMAX)
		return -1;

	*set |= (1 << signum);
	return 0;
}

static inline int sigdelset(sigset_t *set, int signum)
{
	if (signum >= _TELOS_SIGMAX)
		return -1;

	*set &= ~(1 << signum);
	return 0;
}

static inline int sigismember(const sigset_t *set, int signum)
{
	if (signum >= _TELOS_SIGMAX)
		return -1;

	return *set & (1 << signum);
}

#endif
