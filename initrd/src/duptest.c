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

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
	int fd;

	printf("Testing dup... ");
	fd = dup(STDOUT_FILENO);
	write(fd, "OK\n", 3);

	printf("Testing dup2... ");
	fd = dup2(STDOUT_FILENO, STDIN_FILENO);
	if (fd != STDIN_FILENO)
		printf("error: got %d instead of %d\n", fd, STDIN_FILENO);
	write(fd, "OK\n", 3);

	return 0;
}
