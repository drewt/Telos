/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
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
int sigaction (int sig, struct sigaction *act, struct sigaction *oact) {
    return syscall3 (SYS_SIGACTION, (void*) sig, act, oact);
}

/*-----------------------------------------------------------------------------
 * Sets the signal handler for a given signal */
//-----------------------------------------------------------------------------
void (*signal(int sig, void (*func)(int)))(int) {
    return (void(*)(int)) syscall2 (SYS_SIGNAL, (void*) sig, func);
}

/*-----------------------------------------------------------------------------
 * Alters the signal mask, either by blocking the given signals, unblocking
 * them, or setting the signal mask to the given set */
//-----------------------------------------------------------------------------
int sigprocmask (int how, sigset_t *set, sigset_t *oset) {
    return syscall3 (SYS_SIGMASK, (void*) how, set, oset);
}

/*-----------------------------------------------------------------------------
 * Sends a signal to a process */
//-----------------------------------------------------------------------------
int kill (pid_t pid, int signal_number) {
    return syscall2 (SYS_KILL, (void*) pid, (void*) signal_number);
}

/*-----------------------------------------------------------------------------
 * Suspends execution of the calling process until it receives a signal */
//-----------------------------------------------------------------------------
int sigwait (void) {
    return syscall0 (SYS_SIGWAIT);
}


