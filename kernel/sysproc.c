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

#ifdef __KERNEL__
#undef __KERNEL__
#endif

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <kernel/major.h>
#include <telos/process.h>

_Noreturn void idle_proc(void)
{
	asm volatile("_halt: hlt\njmp _halt");
	__builtin_unreachable();
}

_Noreturn void halt(void)
{
	printf("Halting.");
	idle_proc();
}

static _Noreturn void reboot(void)
{
	asm volatile(
	"cli			\n"
	"rb_loop:		\n"
		"inb $0x64, %al	\n"
		"and $0x2,  %al	\n"
		"cmp $0x0,  %al	\n"
		"jne  rb_loop	\n"
	"mov $0xFE, %al		\n"
	"out %al, $0x64		\n"
	);
	__builtin_unreachable();
}

static _Noreturn void die(const char *msg)
{
	printf("%s\n", msg);
	halt();
}

#define TTY_MODE (S_IFCHR | S_IRUSR | S_IWUSR | S_IWGRP)

static void fs_init(void)
{
	/* /dev/ */
	if (mkdir("/dev", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mknod("/dev/cons0", TTY_MODE, DEVICE(TTY_MAJOR, 0)))
		die("fs_init: mknod failed");
	if (mknod("/dev/cons1", TTY_MODE, DEVICE(TTY_MAJOR, 1)))
		die("fs_init: mknod failed");

	/* /bin/ */
	if (mkdir("/bin", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mount(NULL, "/bin", "binfs", 0, NULL))
		die("fs_init: mount failed");

	/* /mod/ */
	if (mkdir("/mod", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		die("fs_init: mkdir failed");
	if (mount(NULL, "/mod", "modfs", 0, NULL))
		die("fs_init: mount failed");

	int fd;
	char buf[6];

	if ((fd = open("/file", O_CREAT, S_IRWXU)) < 0)
		die("fs_init: open failed");
	if (close(fd))
		die("fs_init: close failed");
	if ((fd = open("/test", O_CREAT, S_IRWXU)) < 0)
		die("fs_init: open failed");
	if (write(fd, "test\n", 5) != 5)
		die("fs_init: write failed");
	if (close(fd))
		die("fs_init: close failed");
	if ((fd = open("/test", O_RDONLY)) < 0)
		die("fs_init: open(2) failed");
	if (read(fd, buf, 7) != 5)
		die("fs_init: read failed");
	if (close(fd))
		die("fs_init: close failed");
	buf[5] = '\0';

	if (strcmp(buf, "test\n"))
		die("fs_init: read/write test failed\n");
}

void root_proc()
{
	int status;
	sigset_t set;

	fs_init();

	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	fcreate("/bin/tsh");
	for (;;)
		wait(&status);
	halt();
}
