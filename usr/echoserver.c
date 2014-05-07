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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <telos/msg.h>
#include <telos/process.h>

#define BUF_LEN 100

static void die(const char *reason)
{
	printf("echoserver: %s\n", reason);
	exit(-1);
}

int main(int argc, char *argv[])
{
	printf("%d\n", getpid());

	for (;;) {
		int rv;
		pid_t pid = 0;
		char buf[BUF_LEN];

		if ((rv = recv(&pid, buf, BUF_LEN)) == -1)
			die("recv error");
		if (write(STDOUT_FILENO, buf, rv) == -1)
			die("write error");
		if (reply(pid, NULL, 0) == -1)
			die("reply error");
	}
}
