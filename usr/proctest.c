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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <telos/process.h>

static char *a = "a";
static char *s = "s";
static char *d = "d";
static char *f = "f";

static int print_proc(void *arg)
{
	printf("%s", arg);
	return 0;
}

static int stop_proc()
{
	exit(0);
	printf("FAIL");
	return 0;
}

int main(void)
{
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	printf("Testing syscreate... asdf ?= ");
	syscreate(print_proc, a);
	sleep(1);
	syscreate(print_proc, s);
	sleep(1);
	syscreate(print_proc, d);
	sleep(1);
	syscreate(print_proc, f);
	sleep(1);
	puts("");

	printf("Testing sysstop...");
	syscreate(stop_proc, NULL);
	sleep(1);
	puts("");
	return 0;
}
