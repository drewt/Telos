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

int do_pread(int fd, size_t nbyte, off_t off)
{
	ssize_t bytes;
	char buf[nbyte + 1];

	bytes = pread(fd, buf, nbyte, off);
	if (bytes < 0)
		return bytes;
	for (ssize_t i = 0; i < bytes; i++)
		putchar(buf[i]);
	return 0;
}

int main(int argc, char *argv[])
{
	int n, off, fd;

	if (argc != 4) {
		fprintf(stderr, "pread: wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	n = atoi(argv[2]);
	off = atoi(argv[3]);
	if (n < 0 || off < 0) {
		fprintf(stderr, "pread: nbytes and offset must be positive\n");
		exit(EXIT_FAILURE);
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "pread: couldn't open file: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (do_pread(fd, n, off) < 0) {
		fprintf(stderr, "pread: pread() call failed\n");
		exit(EXIT_FAILURE);
	}

	close(fd);
	return 0;
}
