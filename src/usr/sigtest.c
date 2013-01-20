/* sigtest : signalling tests
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
#include <signal.h>
#include <kernel/common.h>
#include <telos/process.h>
#include <telos/print.h>

static pid_t sigtest_pid;

static void void_handler (int signo) {}

static void sigusr2_action (int signo, siginfo_t *info, void *data) {
    sysputs ("asdf\n");
    if (info->si_signo != SIGUSR2)
        sysputs ("FAIL: info.si_signo incorrect\n");
    if (info->si_pid != sigtest_pid)
        sysputs ("FAIL: info.si_pid incorrect\n");
}

static void sigusr2_handler (int signo) {
    sysputs ("b");
}

static void sigusr1_handler (int signo) {
    int sig;
    sysputs ("a");
    sig = sigwait ();
    if (sig != SIGUSR2)
        sysreport ("SIGUSR1 handler: bad signal\n");
    else
        sysputs ("c\n");
}

static void sig_proc (void *arg) {
    syssleep (500);
    kill (sigtest_pid, SIGUSR1);
    syssleep (500);
    kill (sigtest_pid, SIGUSR2);
}

static void kill_test (void) {
    sysputs ("Testing kill... b ?= ");
    signal (SIGUSR2, sigusr2_handler);
    kill (sigtest_pid, SIGUSR2);
    sysputs ("\n");
}

static void sigprocmask_test (void) {
    sigset_t mask0, mask1;

    sysputs ("Testing sigprocmask...\n");
    signal (SIGUSR2, void_handler);
    sigprocmask (0, NULL, &mask0);
    kill (sigtest_pid, SIGUSR2);
    sigprocmask (0, NULL, &mask1);
    if (mask0 != mask1)
        sysputs ("FAIL: mask altered\n");
    // TODO: finish
}

static void priority_test (void) {
    int sig;
    sysputs ("Testing signal priority... abc ?= ");
    signal (SIGUSR1, sigusr1_handler);
    signal (SIGUSR2, sigusr2_handler);
    syscreate (sig_proc, NULL);
    sig = sigwait ();
    if (sig != SIGUSR1)
        sysputs ("sig_test: bad signal\n");
}

static void sigaction_test (void) {
    sigset_t mask;
    sigprocmask (0, NULL, &mask);

    sysputs ("Testing sigaction... ");
    struct sigaction act = {
        sigusr2_handler,
        sigusr2_action,
        mask,
        SA_SIGINFO
    };
    struct sigaction oact;
    sigaction (SIGUSR2, &act, NULL);

    // verify that sa_sigaction is called (and not sa_handler)
    sysputs ("asdf ?= ");
    kill (sigtest_pid, SIGUSR2);

    // verify that old sigactions can be retrieved
    sigaction (SIGUSR2, &act, &oact);
    if (oact.sa_handler != act.sa_handler)
        sysputs ("FAIL: sa_handler not equal\n");
    if (oact.sa_sigaction != act.sa_sigaction)
        sysputs ("FAIL: sa_sigaction not equal\n");
    if (oact.sa_mask != act.sa_mask)
        sysputs ("FAIL: sa_mask not equal\n");
    if (oact.sa_flags != act.sa_flags)
        sysputs ("FAIL: sa_flags not equal\n");

    // verify that error returned for invalid input
    if (!sigaction (-1, NULL, NULL) || !sigaction (32, NULL, NULL))
        sysputs ("FAIL: accepted invalid signo\n");
    if (sigaction (0, NULL, NULL) || sigaction (31, NULL, NULL))
        sysputs ("FAIL: rejected valid signo\n");
}

void sig_test (void *arg) {
    sigtest_pid = getpid ();

    kill_test ();
    sigprocmask_test ();
    priority_test ();
    sigaction_test ();
}


