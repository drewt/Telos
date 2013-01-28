/* syscall.c : system calls
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
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

#include <stddef.h>
#include <syscall.h>
#include <telos/process.h>
#include <telos/msg.h>
#include <telos/io.h>
#include <signal.h>

typedef int pid_t;

/* PROCESS.H */

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
pid_t syscreate (void(*func)(int,char**), int argc, char *argv[]) {
    return syscall3 (SYS_CREATE, func, (void*) argc, argv);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysyield (void) {
    syscall0 (SYS_YIELD);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysstop (void) {
    syscall0 (SYS_STOP);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
pid_t getpid (void) {
    return syscall0 (SYS_GETPID);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int syssleep (int milliseconds) {
    return syscall1 (SYS_SLEEP, (void*) milliseconds);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int alarm (int seconds) {
    return syscall1 (SYS_ALARM, (void*) seconds);
}

/* PRINT.H */

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysreport (char *s) {
    syscall1 (SYS_REPORT, s);
}

/* MSG.H */

/*-----------------------------------------------------------------------------
 * */ 
//-----------------------------------------------------------------------------
int send (pid_t pid, void *obuf, int olen, void *ibuf, int ilen) {
    return syscall5 (SYS_SEND, (void*) pid, obuf, (void*) olen, ibuf,
            (void*) ilen);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int recv (pid_t *pid, void *buffer, int length) {
    return syscall3 (SYS_RECV, (void*) pid, buffer, (void*) length);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int reply (pid_t pid, void *buffer, int length) {
    return syscall3 (SYS_REPLY, (void*) pid, buffer, (void*) length);
}

/* SIGNAL.H */

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

/* IO.H */

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int open (int devno) {
    return syscall1 (SYS_OPEN, (void*) devno);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int close (int fd) {
    return syscall1 (SYS_CLOSE, (void*) fd);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t read (int fd, void *buf, size_t nbyte) {
    return syscall3 (SYS_READ, (void*) fd, buf, (void*) nbyte);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t write (int fd, const void *buf, size_t nbyte) {
    return syscall3 (SYS_WRITE, (void*) fd, (void*) buf, (void*) nbyte);
}
