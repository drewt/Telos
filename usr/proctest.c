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
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static void fork_test(void)
{
	pid_t child, parent = getpid();
	printf("Testing fork()... ");
	if ((child = fork()) < 0)
		printf("error: fork returned %d\n", child);
	else if (!child) {
		if (getpid() == parent)
			printf("error: getpid() returned parent pid in child\n");
		exit(0);
	}
	putchar('\n');
}

static void wait_test(void)
{
	int status;
	pid_t child, r;
	printf("Testing waitpid()... ");
	if (!(child = fork()))
		exit(2);
	r = waitpid(child, &status, 0);
	if (r != child)
		printf("error: wrong return value from waitpid(): %d\n", r);
	if (!WIFEXITED(status))
		printf("error: wrong reason in status\n");
	if (WEXITSTATUS(status) != 2)
		printf("error: wrong exit status: %d\n", WEXITSTATUS(status));
	putchar('\n');
}

int main(void)
{
	fork_test();
	wait_test();
	return 0;
}
