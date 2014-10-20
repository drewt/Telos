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

int newproc(int argc, char *argv[])
{
	printf("OK\n");
	return 0;
}

int main(void)
{
	char *argv[] = { "/bin/ls", "/bin" };
	if (!fork()) {
		printf("Testing execve... ");
		execve("/bin/ls", argv, NULL);
	}
	sleep(1);
	printf("Exiting in parent\n");
	return 0;
}