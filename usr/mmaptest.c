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

static _Noreturn void die(const char *msg)
{
	fprintf(stderr, "mmaptest: %s\n", msg);
	exit(EXIT_FAILURE);
}

static inline void *page_align(void *addr)
{
	return (void*) ((unsigned long)addr & ~0xFFF);
}

int main(void)
{
	int fd;
	char *addr;
	pid_t child;
	char buf[6];

	printf("Testing mmap()... ");
	fd = open("/test", O_CREAT | O_TRUNC, O_RDWR);
	if (fd < 0)
		die("failed to open /test");
	if (write(fd, "test\n", 5) < 0)
		die("write failed");
	addr = mmap(NULL, 5, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		die("mmap returned MAP_FAILED");
	// fork a child to check that mmap works across a fork()
	if ((child = fork())) {
		wait(NULL);
		return 0;
	}
	if (memcmp(addr, "test\n", 5))
		die("unexpected contents in mapped memory");
	// make sure writeback works
	memcpy(addr, "mmap\n", 5);
	if (munmap(addr, 5))
		die("munmap failed");
	if (pread(fd, buf, 5, 0) < 0)
		die("pread failed");
	if (memcmp(buf, "mmap\n", 5))
		die("unexpected contents in file");

	// test VM_KEEP restriction by trying to unmap kernel stack
	if (!munmap((void*)0xFFFFD000, 1))
		die("succeeded in unmapping kernel stack?");

	// test multi-page mapping
	addr = mmap(NULL, 4096 * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		die("mmap returned MAP_FAILED");
	memset(addr, 'a', 4096*3);
	// unmap middle page to induce VMA split
	if (munmap((void*)((unsigned long)addr + 4096), 1))
		die("munmap failed");
	memset(addr, 'b', 4096);
	memset((void*)((unsigned long)addr + 4096*2), 'c', 4096);
	if (munmap(addr, 4096*3))
		die("munmap failed");
	if (pread(fd, buf, 5, 0) < 0)
		die("pread failed");
	if (memcmp(buf, "bbbbb", 5))
		die("not b's!");
	if (pread(fd, buf, 5, 4096) < 0)
		die("pread failed");
	if (memcmp(buf, "aaaaa", 5))
		die("not a's!");
	if (pread(fd, buf, 5, 4096*2) < 0)
		die("pread failed");
	if (memcmp(buf, "ccccc", 5))
		die("not c's!");
	addr = mmap(NULL, 6, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	if (addr == MAP_FAILED)
		die("mmap returned MAP_FAILED");
	if (memcmp(addr, "\0\0\0\0\0\0", 6))
		die("anonymous mapping is not zeroed");
	memset(addr, 1, 6);
	printf("OK\n");
	return 0;
}
