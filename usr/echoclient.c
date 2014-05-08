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

int main(int argc, char *argv[])
{
	char reply_blk;
	pid_t server_pid;

	if (argc < 3) {
		puts("usage: echoclient <server-pid> <string>");
		return -1;
	}

	server_pid = atoi(argv[1]);

	for (int i = 2; argv[i] != NULL; i++) {
		if (send(server_pid, argv[i], strlen(argv[i])+1, &reply_blk, 1) == -1)
			puts("echoclient: send error");
		else
			putchar(' ');
	}
	putchar(' ');
	return 0;
}
