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

#include <telos/process.h>

static void print_proc(int argc, char *argv[])
{
	printf("%s", (char*) argv[0]);
}

static void stop_proc()
{
	exit(0);
	printf("FAIL");
}

void proctest(void *arg)
{
	char *a = "a", *s = "s", *d = "d", *f = "f";
	printf("Testing syscreate... asdf ?= ");
	syscreate(print_proc, 1, &a);
	sleep(1);
	syscreate(print_proc, 1, &s);
	sleep(1);
	syscreate(print_proc, 1, &d);
	sleep(1);
	syscreate(print_proc, 1, &f);
	sleep(1);
	puts("");

	printf("Testing sysstop...");
	syscreate(stop_proc, 0, NULL);
	sleep(1);
	puts("");
}
