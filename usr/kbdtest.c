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

#include <telos/process.h>

static int read_proc(void *arg)
{
	int fd;
	char in[5];

	if ((fd = open("/dev/cons0", 0)) == -1) {
		puts("Error opening console");
		return -1;
	}

	puts("Reading: ");
	if (read(fd, in, 4) == -1) {
		puts("Error reading from console");
		return -1;
	}
	in[4] = '\0';
	puts(in);
	return 0;
}

int main(void)
{
	int sig;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, NULL);

	syscreate(read_proc, NULL);
	syscreate(read_proc, NULL);

	while (sigwait(&set, &sig));
	while (sigwait(&set, &sig));

	printf("Enter any key to exit: ");
	getchar(); puts("");
	return 0;
}
