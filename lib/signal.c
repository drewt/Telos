/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <syscall.h>
#include <signal.h>

int sigaction(int sig, struct sigaction *restrict act,
		struct sigaction *restrict oact)
{
	return syscall3(SYS_SIGACTION, (void*) sig, act, oact);
}

void(*signal(int sig, void(*func)(int)))(int)
{
	struct sigaction oact;
	struct sigaction act = {
		.sa_handler = func,
		.sa_flags = 0,
		.sa_mask = 0,
	};
	int rv = sigaction(sig, &act, &oact);
	if (rv < 0)
		return SIG_ERR;
	return oact.sa_handler;
}

int sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	return syscall3(SYS_SIGMASK, (void*) how, set, oset);
}

int kill(pid_t pid, int signal_number)
{
	return syscall2(SYS_KILL, (void*) pid, (void*) signal_number);
}

int sigqueue(pid_t pid, int sig, const union sigval value)
{
	return syscall3(SYS_SIGQUEUE, (void*) pid, (void*) sig,
			value.sigval_ptr);
}

int sigwait(const sigset_t *restrict set, int *restrict sig)
{
	return syscall2(SYS_SIGWAIT, (void*) *set, sig);
}

int sigsuspend(const sigset_t *sigmask)
{
	return syscall1(SYS_SIGSUSPEND, (void*) *sigmask);
}
