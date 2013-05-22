/* signal.h : signals
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

#ifndef __SIGNAL_H_
#define __SIGNAL_H_

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

enum sigaction_flags {
    SA_NOCLDSTOP  = 1,
    SA_ONSTACK    = 1 << 1,
    SA_RESETHAND  = 1 << 2,
    SA_RESTART    = 1 << 3,
    SA_SIGINFO    = 1 << 4,
    SA_NOCLDWAIT  = 1 << 5,
    SA_NODEFER    = 1 << 6
};

enum sigprocmask_flags { SIG_BLOCK, SIG_SETMASK, SIG_UNBLOCK };

int kill (pid_t pid, int signal_number);
int sigaction (int sig, struct sigaction *act, struct sigaction *oact);
void(*signal(int sig, void (*func)(int)))(int);
int sigprocmask (int how, sigset_t *set, sigset_t *oset);
int sigwait (void);

#endif // __SIGNAL_H_
