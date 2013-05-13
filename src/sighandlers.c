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

#include <telos/process.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define I SIG_IGN

static void T (int signo) {
    exit (1);
}

static void A (int signo) {
    exit (1);
}

static void S (int signo) {
    // TODO
}

static void C (int signo) {
    // TODO
}

const struct sigaction default_sigactions[_TELOS_SIGMAX] = {
    [SIGABRT]   = { .sa_handler = A, .sa_flags = 0 },
    [SIGALRM]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGBUS]    = { .sa_handler = A, .sa_flags = 0 },
    [SIGCHLD]   = { .sa_handler = I, .sa_flags = 0 },
    [SIGCONT]   = { .sa_handler = C, .sa_flags = 0 },
    [SIGFPE]    = { .sa_handler = A, .sa_flags = 0 },
    [SIGHUP]    = { .sa_handler = T, .sa_flags = 0 },
    [SIGILL]    = { .sa_handler = A, .sa_flags = 0 },
    [SIGINT]    = { .sa_handler = T, .sa_flags = 0 },
    [SIGPIPE]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGQUIT]   = { .sa_handler = A, .sa_flags = 0 },
    [SIGSEGV]   = { .sa_handler = A, .sa_flags = 0 },
    [SIGTERM]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGTSTP]   = { .sa_handler = S, .sa_flags = 0 },
    [SIGTTIN]   = { .sa_handler = S, .sa_flags = 0 },
    [SIGTTOU]   = { .sa_handler = S, .sa_flags = 0 },
    [SIGUSR1]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGUSR2]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGPOLL]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGPROF]   = { .sa_handler = T, .sa_flags = 0 },
    [SIGSYS]    = { .sa_handler = A, .sa_flags = 0 },
    [SIGTRAP]   = { .sa_handler = A, .sa_flags = 0 },
    [SIGURG]    = { .sa_handler = I, .sa_flags = 0 },
    [SIGVTALRM] = { .sa_handler = T, .sa_flags = 0 },
    [SIGXCPU]   = { .sa_handler = A, .sa_flags = 0 },
    [SIGSTOP]   = { .sa_handler = S, .sa_flags = 0 },
    [SIGKILL]   = { .sa_handler = T, .sa_flags = 0 },
};
