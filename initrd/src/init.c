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
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TTY_MODE (S_IFCHR | S_IRUSR | S_IWUSR | S_IWGRP)
#define MOD_MODE (S_IFBLK | S_IRUSR | S_IWUSR)

#define die(...) do { \
	printf(__VA_ARGS__); \
	exit(1); \
} while (0);

static void do_mount(const char *source, const char *target,
		const char *type, unsigned long flags, const void *data)
{
	if (mount(source, target, type, flags, data))
		die("init: failed to mount %s on %s\n", type, target);
}

static void fs_init(void)
{
	// mount filesystems
	do_mount(NULL, "/mod", "modfs", 0, NULL);
	do_mount(NULL, "/tmp", "ramfs", 0, NULL);
}

int main(void)
{
	int status;
	sigset_t set;

	open("/dev/cons0", O_RDONLY);
	open("/dev/cons0", O_WRONLY);
	open("/dev/cons0", O_WRONLY);

	fs_init();

	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	if (!fork()) {
		char *argv[] = { "/bin/tsh", NULL };
		char *envp[] = { NULL };
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		execve("/bin/tsh", argv, envp);
	}

	for (;;)
		wait(&status);
}
