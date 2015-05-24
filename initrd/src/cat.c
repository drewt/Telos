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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd;
	ssize_t rv;
	char buf[1024];

	for (int i = 1; i < argc; i++) {
		if ((fd = open(argv[i], O_RDONLY)) < 0) {
			printf("%s: %s: No such file or directory\n", argv[0],
					argv[i]);
			continue;
		}
		while ((rv = read(fd, buf, 1024)) > 0)
			write(STDOUT_FILENO, buf, rv);
			//printf("%.*s", rv, buf);
		if (rv < 0)
			printf("%s: error reading %s\n", argv[0], argv[i]);
	}
	return 0;
}
