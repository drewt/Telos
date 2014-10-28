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

/*-----------------------------------------------------------------------------
 * Registers an action for a given signal */
//-----------------------------------------------------------------------------
int sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	return syscall3(SYS_SIGACTION, (void*) sig, act, oact);
}

/*-----------------------------------------------------------------------------
 * Sets the signal handler for a given signal */
//-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * Alters the signal mask, either by blocking the given signals, unblocking
 * them, or setting the signal mask to the given set */
//-----------------------------------------------------------------------------
int sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	return syscall3(SYS_SIGMASK, (void*) how, set, oset);
}

/*-----------------------------------------------------------------------------
 * Sends a signal to a process */
//-----------------------------------------------------------------------------
int kill(pid_t pid, int signal_number)
{
	return syscall2(SYS_KILL, (void*) pid, (void*) signal_number);
}

/*-----------------------------------------------------------------------------
 * Sends a signal to a process, with an associated value */
//-----------------------------------------------------------------------------
int sigqueue(pid_t pid, int sig, const union sigval value)
{
	return syscall3(SYS_SIGQUEUE, (void*) pid, (void*) sig,
			value.sigval_ptr);
}

/*-----------------------------------------------------------------------------
 * Suspends execution of the calling process until it receives a signal */
//-----------------------------------------------------------------------------
int sigwait(void)
{
	return syscall0(SYS_SIGWAIT);
}
