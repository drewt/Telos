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
	return syscall3(SYS_EXECVE, (void*)pathname, (void*)argv, (void*)envp);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
unsigned int alarm(unsigned int seconds)
{
	return syscall1(SYS_ALARM, (void*) (seconds * __TICKS_PER_SEC))
		* __TICKS_PER_SEC;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
unsigned int sleep(unsigned int seconds)
{
	return syscall1(SYS_SLEEP, (void*) (seconds * __TICKS_PER_SEC))
		* __TICKS_PER_SEC;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
pid_t getpid(void)
{
	return syscall0(SYS_GETPID);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int open(const char *pathname, int flags, ...)
{
	int rv = syscall2(SYS_OPEN, (void*) pathname, (void*) flags);
	if (rv < 0)
		return -1;
	return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int close(int fd)
{
	int rv = syscall1(SYS_CLOSE, (void*) fd);
	if (rv < 0)
		return -1;
	return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t read(int fd, void *buf, size_t nbyte)
{
	int rv = syscall3(SYS_READ, (void*) fd, buf, (void*) nbyte);
	if (rv < 0)
		return -1;
	return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t write(int fd, const void *buf, size_t nbyte)
{
	int rv = syscall3(SYS_WRITE, (void*) fd, (void*) buf, (void*) nbyte);
	if (rv < 0)
		return -1;
	return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
off_t lseek(int fd, off_t offset, int whence)
{
	int rv = syscall3(SYS_LSEEK, (void*) fd, (void*) offset, (void*) whence);
	if (rv < 0)
		return -1;
	return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int ioctl(int fd, int command, ...)
{
	int rv;
	va_list ap;
	va_start(ap, command);
	rv = syscall3(SYS_IOCTL, (void*) fd, (void*) command, ap);
	va_end(ap);
	return (rv < 0) ? -1 : rv;
}
