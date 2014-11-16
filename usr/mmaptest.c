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
	fprintf(stderr, msg);
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
		die("mmaptest: failed to open /test\n");
	if (write(fd, "test\n", 5) < 0)
		die("mmaptest: write failed\n");
	addr = mmap(NULL, 5, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		die("mmaptest: mmap returned MAP_FAILED\n");
	// fork a child to check that mmap works across a fork()
	if ((child = fork())) {
		wait(NULL);
		return 0;
	}
	if (memcmp(addr, "test\n", 5))
		die("mmaptest: unexpected contents in mapped memory\n");
	// make sure writeback works
	memcpy(addr, "mmap\n", 5);
	if (munmap(addr, 5))
		die("mmaptest: munmap failed\n");
	if (pread(fd, buf, 5, 0) < 0)
		die("mmaptest: pread failed\n");
	if (memcmp(buf, "mmap\n", 5))
		die("mmaptest: unexpected contents in file\n");

	// FIXME: this really ought to be allowed; only kernel memory (incl.
	//        kernel stack) should be un-unmappable.  For now, this serves
	//        to test the VM_KEEP restriction.
	if (!munmap(page_align(buf), 1))
		die("mmaptest: succeeded in unmapping stack?\n");

	// test multi-page mapping
	addr = mmap(NULL, 4096 * 3, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		die("mmaptest: mmap returned MAP_FAILED\n");
	memset(addr, 'a', 4096*3);
	// unmap middle page to induce VMA split
	if (munmap((void*)((unsigned long)addr + 4096), 1))
		die("mmaptest: munmap failed\n");
	memset(addr, 'b', 4096);
	memset((void*)((unsigned long)addr + 4096*2), 'c', 4096);
	if (munmap(addr, 4096*3))
		die("mmaptest: munmap failed\n");
	if (pread(fd, buf, 5, 0) < 0)
		die("mmaptest: pread failed\n");
	if (memcmp(buf, "bbbbb", 5))
		die("not b's!\n");
	if (pread(fd, buf, 5, 4096) < 0)
		die("mmaptest: pread failed\n");
	if (memcmp(buf, "aaaaa", 5))
		die("not a's!\n");
	if (pread(fd, buf, 5, 4096*2) < 0)
		die("mmaptest: pread failed\n");
	if (memcmp(buf, "ccccc", 5))
		die("not c's!\n");
	printf("OK\n");
	return 0;
}
