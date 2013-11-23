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

#include <stddef.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <kernel/common.h>
#include <telos/process.h>

/* half-assed "test" of sigqueue--verifies that it's substitutable for kill */
#if 0
#define kill(a,b) sigqueue(a,b,(union sigval)0)
#endif

static pid_t sigtest_pid;

static void void_handler(int signo) {}

static void sigusr2_action(int signo, siginfo_t *info, void *data)
{
	puts("asdf");
	if (info->si_signo != SIGUSR2)
		puts("FAIL: info.si_signo incorrect");
	if (info->si_pid != sigtest_pid)
		puts("FAIL: info.si_pid incorrect");
}

static void sigusr2_handler(int signo)
{
	printf("b");
}

static void sigusr1_handler(int signo)
{
	int sig;
	printf("a");
	sig = sigwait();
	if (sig != SIGUSR2)
		puts("SIGUSR1 handler: bad signal");
	else
		puts("c");
}

static void sig_proc()
{
	sleep(1);
	kill(sigtest_pid, SIGUSR1);
	sleep(1);
	kill(sigtest_pid, SIGUSR2);
}

static void kill_test(void)
{
	printf("Testing kill... b ?= ");
	signal(SIGUSR2, sigusr2_handler);
	kill(sigtest_pid, SIGUSR2);
	puts("");
}

static void sigprocmask_test(void)
{
	sigset_t mask0, mask1;

	puts("Testing sigprocmask...");
	signal(SIGUSR2, void_handler);
	sigprocmask(0, NULL, &mask0);
	kill(sigtest_pid, SIGUSR2);
	sigprocmask(0, NULL, &mask1);
	if (mask0 != mask1)
		puts("FAIL: mask altered");
	// TODO: finish
}

static void priority_test(void)
{
	int sig;
	printf("Testing signal priority... abc ?= ");
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGUSR2, sigusr2_handler);
	syscreate(sig_proc, 0, NULL);
	sig = sigwait();
	if (sig != SIGUSR1)
		puts("sig_test: bad signal");
}

static void sigaction_test(void)
{
	sigset_t mask;
	sigprocmask(0, NULL, &mask);

	printf("Testing sigaction... ");
	struct sigaction act = {
		.sa_sigaction = sigusr2_action,
		mask,
		SA_SIGINFO
	};
	struct sigaction oact;
	sigaction(SIGUSR2, &act, NULL);

	/* verify that signal handler is called */
	printf("asdf ?= ");
	kill(sigtest_pid, SIGUSR2);

	/* verify that old sigactions can be retrieved */
	sigaction(SIGUSR2, &act, &oact);
	if (oact.sa_handler != act.sa_handler)
		puts("FAIL: sa_handler not equal");
	if (oact.sa_sigaction != act.sa_sigaction)
		puts("FAIL: sa_sigaction not equal");
	if (oact.sa_mask != act.sa_mask)
		puts("FAIL: sa_mask not equal");
	if (oact.sa_flags != act.sa_flags)
		puts("FAIL: sa_flags not equal");

	/* verify that error returned for invalid input */
	if (!sigaction(-1, NULL, NULL) || !sigaction(32, NULL, NULL))
		puts("FAIL: accepted invalid signo");
	if (sigaction(1, NULL, NULL) || sigaction(SIGXCPU, NULL, NULL))
		puts("FAIL: rejected valid signo");
}

void sigtest(void *arg)
{
	sigtest_pid = getpid();

	kill_test();
	sigprocmask_test();
	priority_test();
	sigaction_test();
}
