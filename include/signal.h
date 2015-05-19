/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
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

#include <kernel/signal.h>

int kill(pid_t pid, int signal_number);
//int killpg(pid_t, int);
//void psiginfo(const siginfo_t *,const char *);
//void psignal(int, const char *);
//int pthread_kill(pthread_t, int);
//int pthread_sigmask(int, const sigset_t *restrict, sigset_t *restrict);
//int raise(int);
int sigaction(int sig, struct sigaction *restrict act,
		struct sigaction *restrict oact);
//int sigaltstack(const stack_t *restruct, stack_t *restrict);
//int sighold(int);
//int sigignore(int);
//int siginterrupt(int, int);
void(*signal(int sig, void(*func)(int)))(int);
//int sigpause(int);
//int sigpending(sigset_t *);
int sigprocmask(int how, sigset_t *set, sigset_t *oset);
int sigqueue(pid_t pid, int signal_number, const union sigval value);
//int sigrelse(int);
//void (*sigset(int, void (*)(int)))(int);
int sigsuspend(const sigset_t *sigmask);
//int sigtimedwait(const sigset_t *restrict, siginfo_t *restrict,
//		const struct timespec *restrict);
int sigwait(const sigset_t *restrict set, int *restrict sig);
//int sigwaitinfo(const sigset_t *restrict siginfo_t *restrict);

#endif
