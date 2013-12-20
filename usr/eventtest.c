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

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <telos/process.h>

static void sleep_proc(int argc, char *argv[])
{
	sleep(argc);
	printf("%d ", argc);
}

static void nanosleep_proc(int argc, char *argvp[])
{
	struct timespec spec = {
		.tv_sec = argc,
		.tv_nsec = 0
	};
	nanosleep(&spec, NULL);
	printf("%d ", argc);
}

static void sleep_test(void)
{
	printf("Testing sleep...\nVerify ascending: ");
	syscreate(sleep_proc, 1, NULL);
	syscreate(sleep_proc, 2, NULL);
	syscreate(sleep_proc, 3, NULL);
	sleep(4);

	printf("\nVefify ascending: ");
	syscreate(sleep_proc, 3, NULL);
	syscreate(sleep_proc, 2, NULL);
	syscreate(sleep_proc, 1, NULL);
	sleep(4);

	printf("\nVerify ascending: ");
	syscreate(nanosleep_proc, 1, NULL);
	syscreate(nanosleep_proc, 2, NULL);
	syscreate(nanosleep_proc, 3, NULL);
	sleep(4);
	puts("");
}

static void alarm_handler(int signo)
{
	puts("rm");
}

static void alarm_test(void)
{
	int sig, rv;
	signal(SIGALRM, alarm_handler);
	printf("Testing alarm... alrm ?= ");
	alarm(2);
	if ((rv = alarm(2)) <= 0)
		printf("error");
	printf("al");
	for (sig = sigwait(); sig != SIGALRM; sig = sigwait());
	printf("Testing alarm(0)... ");
	alarm(2);
	if (alarm(0) > 0)
		puts("okay");
	else
		puts("error");
}

static void timer_action(int signo, siginfo_t *info, void *context)
{
	printf("TIMER!\n");
}

static void timer_test(void)
{
	timer_t tid;
	sigset_t mask;
	struct sigaction act;
	struct itimerspec time = {
		.it_interval = {0},
		.it_value = {
			.tv_sec = 1,
			.tv_nsec = 0,
		},
	};

	sigprocmask(SIG_BLOCK, NULL, &mask);
	act.sa_mask = mask;
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = timer_action;

	if (sigaction(SIGALRM, &act, NULL) == -1) {
		printf("sigaction() failed\n");
	}

	if (timer_create(CLOCK_MONOTONIC, NULL, &tid) < 0) {
		printf("timer_create() failed\n");
	}

	if (timer_settime(tid, 0, &time, NULL) < 0) {
		printf("timer_settime() failed\n");
	}
	sleep(2);
}

void eventtest(int argc, char *argv[])
{
	sleep_test();
	alarm_test();
	timer_test();
}
