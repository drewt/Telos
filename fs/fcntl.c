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

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <sys/fcntl.h>

static inline int dupfd(int oldfd, int newfd)
{
	current->filp[newfd] = current->filp[oldfd];
	current->filp[newfd]->f_count++;
	return newfd;
}

static int do_dup(int fd, int start)
{
	if (!fd_ok(current, fd))
		return -EBADF;
	start = get_fd(current, start);
	if (start < 0)
		return start;
	return dupfd(fd, start);
}

static int do_dup2(int oldfd, int newfd)
{
	if (!fd_ok(current, oldfd))
		return -EBADF;
	if (newfd == oldfd)
		return newfd;
	if (newfd >= NR_FILES)
		return -EBADF;
	sys_close(newfd);
	return dupfd(oldfd, newfd);
}

long sys_fcntl(int fd, int cmd, int arg)
{
	switch (cmd) {
	case F_DUPFD:
		return do_dup(fd, arg);
	case F_DUP2FD:
		return do_dup2(fd, arg);
	case F_DUPFD_CLOEXEC:
	case F_GETFD:
	case F_SETFD:
	case F_GETFL:
	case F_SETFL:
	case F_GETOWN:
	case F_SETOWN:
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		return -ENOTSUP;
	}
	return -EINVAL;
}
