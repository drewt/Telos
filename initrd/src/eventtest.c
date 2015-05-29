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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

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

static _Noreturn void nanosleep_proc(const struct timespec *how_long, char c)
{
	nanosleep(how_long, NULL);
	printf("%c", c);
	exit(0);
}

static void sleep_test(void)
{
	printf("Testing sleep... abc ?= ");
	fflush(stdout);
	if (!fork())
		nanosleep_proc(&less_than_two, 'b');
	if (!fork())
		nanosleep_proc(&less_than_one, 'a');
	nanosleep(&less_than_three, NULL);
	puts("c");
	fflush(stdout);
}

static void alarm_handler(int signo)
{
	puts("rm");
}

static void alarm_test(void)
{
	sigset_t set;
	signal(SIGALRM, alarm_handler);
	printf("Testing alarm... alrm ?= ");
	fflush(stdout);
	alarm(1);
	if (alarm(1) <= 0)
		printf("error");
	printf("al");
	fflush(stdout);
	sigfillset(&set);
	sigdelset(&set, SIGALRM);
	sigsuspend(&set);
	printf("Testing alarm(0)... ");
	fflush(stdout);
	alarm(2);
	if (alarm(0) > 0)
		puts("okay");
	else
		puts("error");
	fflush(stdout);
}

static void timer_action(int signo, siginfo_t *info, void *context)
{
	printf("1 ");
	fflush(stdout);
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
	fflush(stdout);
	if (timer_settime(tid, 0, &time, NULL) < 0) {
		printf("timer_settime() failed\n");
	}
	sleep(200); /* should be interrupted */
	puts("2");
	fflush(stdout);
}

static void clock_test(void)
{
	struct timespec t;

	if (clock_gettime(CLOCK_MONOTONIC, &t) < 0) {
		printf("clock_gettime() failed\n");
		return;
	}

	printf("CLOCK_MONOTONIC: %lu\n", t.tv_sec);

	if (clock_gettime(CLOCK_REALTIME, &t) < 0) {
		printf("clock_gettime() failed\n");
		return;
	}

	printf("CLOCK_REALTIME: %lx\n", t.tv_sec);
}

#include <stdlib.h>
int main(int argc, char *argv[])
{
	sleep_test();
	alarm_test();
	timer_test();
	clock_test();
	return 0;
}
