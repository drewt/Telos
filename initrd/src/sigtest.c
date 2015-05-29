/*  Copyright 2013-2015 Drew Thoreson
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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

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
	if (signo != SIGUSR2)
		printf("sigusr2_handler: wrong signal: %d\n", signo);
	printf("b");
	fflush(stdout);
}

static void sigusr1_handler(int signo)
{
	sigset_t set;
	if (signo != SIGUSR1)
		printf("sigusr1_handler: wrong signal: %d\n", signo);
	printf("a");
	fflush(stdout);
	sigfillset(&set);
	sigdelset(&set, SIGUSR2);
	sigsuspend(&set);
	puts("c");
	fflush(stdout);
}

static _Noreturn void sig_proc(void)
{
	sleep(1);
	kill(sigtest_pid, SIGUSR1);
	sleep(1);
	kill(sigtest_pid, SIGUSR2);
	exit(0);
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
	sigset_t set;
	printf("Testing signal priority... abc ?= ");
	fflush(stdout);
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGUSR2, sigusr2_handler);
	if (!fork())
		sig_proc();
	sigfillset(&set);
	sigdelset(&set, SIGUSR1);
	sigsuspend(&set);
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

static void child_proc(void)
{
	struct timespec t = {
		.tv_sec = 0,
		.tv_nsec = 100000000,
	};
	printf("Kill m");
	for (;;) {
		putchar('e');
		fflush(stdout);
		nanosleep(&t, NULL);
	}
}

static void sigkill_test(void)
{
	pid_t child;
	if (!(child = fork()))
		child_proc();
	sleep(1);
	kill(child, SIGKILL);
	printf("\nKilled child\n");
}

static void change_mask_handler(int sig)
{
	sigset_t mask;
	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);
}

static void suspend_proc(sigset_t oset)
{
	sigset_t nset;
	signal(SIGUSR1, change_mask_handler);

	// suspend with SIGUSR1 unblocked
	sigfillset(&nset);
	sigdelset(&nset, SIGUSR1);
	sigsuspend(&nset);

	// verify that signal mask unchanged
	sigprocmask(0, NULL, &nset);
	if (nset != oset)
		printf("FAIL: signal mask altered by sigsuspend\n");
	exit(0);
}

static void sigsuspend_test(void)
{
	int sig;
	pid_t child;
	sigset_t set;

	// block all signals
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	sigprocmask(0, NULL, &set);

	if (!(child = fork()))
		suspend_proc(set);
	kill(child, SIGUSR1);
	do {
		sigwait(&set, &sig);
	} while (sig != SIGCHLD);
}

int main(void)
{
	sigset_t mask;
	sigtest_pid = getpid();

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	kill_test();
	sigprocmask_test();
	priority_test();
	sigaction_test();
	sigkill_test();
	sigsuspend_test();

	return 0;
}
