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

#define TIMESPEC_INIT(s, ns)	\
	{			\
		.tv_sec  = s,	\
		.tv_nsec = ns	\
	}

#define TIMESPEC(name, s, ns) \
	struct timespec name = TIMESPEC_INIT(s, ns)

#define ITIMERSPEC(name, s, ns)				\
	struct itimerspec name = {			\
		.it_interval = {0},			\
		.it_value = TIMESPEC_INIT(s, ns)	\
	}

#define HALFSEC_NSEC 500000000

static const int one = 1;
static const int two = 2;
static const TIMESPEC(less_than_one, 0, HALFSEC_NSEC);
static const TIMESPEC(less_than_two, 1, HALFSEC_NSEC);
static const TIMESPEC(less_than_three, 2, HALFSEC_NSEC);

static void sleep_proc(int argc, char *argv[])
{
	int sec = argc == 2 ? 1 : 2;
	sleep(sec);
	printf("%d ", argc);
}

static void nanosleep_proc(int argc, char *argv[])
{
	const struct timespec *spec = argc == 1 ? &less_than_one : &less_than_two;
	nanosleep(spec, NULL);
	printf("%d ", argc);
}

static void sleep_test(void)
{
	printf("Testing sleep...\nVerify ascending: ");
	syscreate(nanosleep_proc, 1, NULL);
	syscreate(sleep_proc, 2, NULL);
	syscreate(sleep_proc, 4, NULL);
	syscreate(nanosleep_proc, 3, NULL);
	nanosleep(&less_than_three, NULL);
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
	alarm(1);
	if ((rv = alarm(1)) <= 0)
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
	printf("1 ");
}

static void timer_test(void)
{
	timer_t tid;
	sigset_t mask;
	struct sigaction act;
	ITIMERSPEC(time, 0, HALFSEC_NSEC);

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

	printf("Testing timers...\nVerify ascending: ");
	if (timer_settime(tid, 0, &time, NULL) < 0) {
		printf("timer_settime() failed\n");
	}
	sleep(1);
	puts("2");
}

void eventtest(int argc, char *argv[])
{
	sleep_test();
	alarm_test();
	timer_test();
}
