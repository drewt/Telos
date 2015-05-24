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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int off, fd;

	if (argc != 4) {
		fprintf(stderr, "pwrite: wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	off = atoi(argv[3]);

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "pwrite: couldn't open file: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (pwrite(fd, argv[2], strlen(argv[2]), off) < 0) {
		fprintf(stderr, "pwrite: pwrite() call failed\n");
		exit(EXIT_FAILURE);
	}

	close(fd);
	return 0;
}
