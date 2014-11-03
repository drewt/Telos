/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <syscall.h>

pid_t fork(void)
{
	int rc;
	if ((rc = syscall0(SYS_FORK)) < 0)
		return -1;
	return rc;
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	return syscall3(SYS_EXECVE, pathname, argv, envp);
}

unsigned int alarm(unsigned int seconds)
{
	return syscall1(SYS_ALARM, (seconds * __TICKS_PER_SEC))
		* __TICKS_PER_SEC;
}

unsigned int sleep(unsigned int seconds)
{
	return syscall1(SYS_SLEEP, (seconds * __TICKS_PER_SEC))
		* __TICKS_PER_SEC;
}

pid_t getpid(void)
{
	return syscall0(SYS_GETPID);
}

int open(const char *pathname, int flags, ...)
{
	int rv;
	va_list ap;
	unsigned long mode;
	va_start(ap, flags);
	mode = va_arg(ap, unsigned long);
	va_end(ap);
	rv = syscall3(SYS_OPEN, pathname, flags, mode);
	if (rv < 0)
		return -1;
	return rv;
}

int close(int fd)
{
	int rv = syscall1(SYS_CLOSE, fd);
	if (rv < 0)
		return -1;
	return rv;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
	int rv = syscall3(SYS_READ, fd, buf, nbyte);
	if (rv < 0)
		return -1;
	return rv;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
	int rv = syscall3(SYS_WRITE, fd, buf, nbyte);
	if (rv < 0)
		return -1;
	return rv;
}

off_t lseek(int fd, off_t offset, int whence)
{
	int rv = syscall3(SYS_LSEEK, fd, offset, whence);
	if (rv < 0)
		return -1;
	return rv;
}

int ioctl(int fd, int command, ...)
{
	int rv;
	va_list ap;
	unsigned long arg;
	va_start(ap, command);
	arg = va_arg(ap, unsigned long);
	va_end(ap);
	rv = syscall3(SYS_IOCTL, fd, command, arg);
	if (rv < 0)
		return -1;
	return rv;
}
