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

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/types.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

enum {
	STDIN_FILENO,
	STDOUT_FILENO,
	STDERR_FILENO,
};

pid_t fork(void);
unsigned int alarm(unsigned int seconds);
pid_t getpid(void);
unsigned int sleep(unsigned int seconds);
int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t write(int fd, const void *buf, size_t nbyte);
int ioctl(int fd, int command, ...);
int rmdir(const char *path);
int chdir(const char *path);
int unlink(const char *path);
int link(const char *old, const char *new);
void *sbrk(long increment);

#endif
