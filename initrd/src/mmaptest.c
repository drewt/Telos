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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

static _Noreturn void _die(int line, const char *msg)
{
	fprintf(stderr, "mmaptest: %d: %s\n", line, msg);
	exit(EXIT_FAILURE);
}

#define die(msg) _die(__LINE__, msg)

static inline void *page_align(void *addr)
{
	return (void*) ((unsigned long)addr & ~0xFFF);
}

#define die_if(cond) do { if (cond) die(#cond); } while (0)

#define FIXED_ADDR ((void*)0x05000000)
#define PROT_RW (PROT_READ | PROT_WRITE)

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

	addr = mmap(NULL, 5, PROT_RW, MAP_SHARED, fd, 0);
	die_if(addr == MAP_FAILED);
	// fork a child to check that mmap works across a fork()
	if ((child = fork())) {
		wait(NULL);
		return 0;
	}
	die_if(memcmp(addr, "test\n", 5));
	// make sure writeback works
	memcpy(addr, "mmap\n", 5);
	die_if(munmap(addr, 5));
	die_if(pread(fd, buf, 5, 0) < 0);
	die_if(memcmp(buf, "mmap\n", 5));

	// test VM_KEEP restriction by trying to unmap kernel stack
	die_if(!munmap((void*)0xFFFFD000, 1));

	// test multi-page mapping
	addr = mmap(NULL, 4096 * 3, PROT_RW, MAP_SHARED, fd, 0);
	die_if(addr == MAP_FAILED);
	memset(addr, 'a', 4096*3);
	// unmap middle page to induce VMA split
	die_if(munmap((void*)((unsigned long)addr + 4096), 1));
	memset(addr, 'b', 4096);
	memset((void*)((unsigned long)addr + 4096*2), 'c', 4096);
	die_if(munmap(addr, 4096*3));
	die_if(pread(fd, buf, 5, 0) < 0);
	die_if(memcmp(buf, "bbbbb", 5));
	die_if(pread(fd, buf, 5, 4096) < 0);
	die_if(memcmp(buf, "aaaaa", 5));
	die_if(pread(fd, buf, 5, 4096*2) < 0);
	die_if(memcmp(buf, "ccccc", 5));

	// MAP_ANONYMOUS
	addr = mmap(NULL, 6, PROT_RW, MAP_ANONYMOUS, -1, 0);
	die_if(addr == MAP_FAILED);
	die_if(memcmp(addr, "\0\0\0\0\0\0", 6));
	memset(addr, 1, 6);

	// MAP_FIXED
	addr = mmap(FIXED_ADDR, 6, PROT_RW, MAP_FIXED | MAP_ANONYMOUS, -1, 0);
	die_if(addr == MAP_FAILED);
	die_if(addr != FIXED_ADDR);
	memset(addr, 1, 6);
	printf("OK\n");
	return 0;
}
