/*  Copyright 2013-2015 Drew Thoreson
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

#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <syscall.h>
#include <string.h>
#include <sys/exec.h>

pid_t fork(void)
{
	int rc;
	if ((rc = syscall0(SYS_FORK)) < 0)
		return -1;
	return rc;
}

static int _execve(const char *pathname, char *const argv[], char *const envp[],
		size_t argc, size_t envc)
{
	struct _Telos_string s_argv[argc];
	struct _Telos_string s_envp[envc];
	struct exec_args e_args = {
		.pathname = {
			.str = pathname,
			.len = strlen(pathname),
		},
		.argv = s_argv,
		.envp = s_envp,
		.argc = argc,
		.envc = envc,
	};

	for (size_t i = 0; i < argc; i++) {
		s_argv[i].str = argv[i];
		s_argv[i].len = strlen(argv[i]);
	}

	for (size_t i = 0; i < envc; i++) {
		s_envp[i].str = envp[i];
		s_envp[i].len = strlen(envp[i]);
	}

	return syscall1(SYS_EXECVE, &e_args);
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
	size_t argc, envc;
	for (argc = 0; argv[argc]; argc++);
	for (envc = 0; envp[envc]; envc++);
	return _execve(pathname, argv, envp, argc, envc);
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
	rv = syscall4(SYS_OPEN, pathname, strlen(pathname), flags, mode);
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

ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset)
{
	if (offset < 0)
		return -1;
	int rv = syscall4(SYS_PREAD, fd, buf, nbyte, offset);
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

ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset)
{
	if (offset < 0)
		return -1;
	int rv = syscall4(SYS_PWRITE, fd, buf, nbyte, offset);
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

int truncate(const char *path, off_t length)
{
	int rv;
	if (length < 0)
		return -1;
	rv = syscall3(SYS_TRUNCATE, path, strlen(path), length);
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

int fcntl(int fd, int cmd, ...)
{
	int rv;
	va_list ap;
	int arg;
	va_start(ap, cmd);
	arg = va_arg(ap, int);
	va_end(ap);
	rv = syscall3(SYS_FCNTL, fd, cmd, arg);
	if (rv < 0)
		return -1;
	return rv;
}

int dup(int fd)
{
	int rv = syscall3(SYS_FCNTL, fd, F_DUPFD, 0);
	if (rv < 0)
		return -1;
	return rv;
}

int dup2(int oldfd, int newfd)
{
	int rv = syscall3(SYS_FCNTL, oldfd, F_DUP2FD, newfd);
	if (rv < 0)
		return -1;
	return rv;
}
