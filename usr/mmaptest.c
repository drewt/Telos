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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

int main(void)
{
	int fd;
	char *addr;
	pid_t child;

	printf("Testing mmap()... ");
	fd = open("/test", O_CREAT | O_TRUNC, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "mmaptest: failed to open /test\n");
		exit(EXIT_FAILURE);
	}

	if (write(fd, "mmap\n", 5) < 0) {
		fprintf(stderr, "mmaptest: write failed\n");
		exit(EXIT_FAILURE);
	}

	addr = mmap(NULL, 5, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "mmaptest: mmap returned MAP_FAILED\n");
		exit(EXIT_FAILURE);
	}

	// fork a child to check that mmap works across a fork()
	if ((child = fork())) {
		wait(NULL);
		return 0;
	}

	if (memcmp(addr, "mmap\n", 5)) {
		fprintf(stderr, "mmaptest: unexpected contents in mapped memory\n");
		exit(EXIT_FAILURE);
	}

	printf("OK\n");
	return 0;
}
