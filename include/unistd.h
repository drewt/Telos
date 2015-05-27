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

#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <sys/type_defs.h>

#ifndef NULL
#define NULL _NULL_DEFN
#endif
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef _SIZE_T_TYPE size_t;
#endif
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef _SSIZE_T_TYPE ssize_t;
#endif
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef _UID_T_TYPE uid_t;
#endif
#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED
typedef _GID_T_TYPE gid_t;
#endif
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef _OFF_T_TYPE off_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _INTPTR_T_DEFINED
#define _INTPTR_T_DEFINED
//typedef _INTPTR_T_TYPE intptr_t; // FIXME: conflicts with gcc stdint.h
#endif

enum {
	STDIN_FILENO,
	STDOUT_FILENO,
	STDERR_FILENO,
};

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

pid_t fork(void);
int execve(const char *pathname, char *const argv[], char *const env[]);
unsigned int alarm(unsigned int seconds);
pid_t getpid(void);
unsigned int sleep(unsigned int seconds);
int open(const char *pathname, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset);
ssize_t write(int fd, const void *buf, size_t nbyte);
ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset);
off_t lseek(int fd, off_t offset, int whence);
int truncate(const char *path, off_t length);
int ioctl(int fd, int command, ...);
int rmdir(const char *path);
int chdir(const char *path);
int unlink(const char *path);
int link(const char *old, const char *new);
void *sbrk(long increment);
int dup(int fd);
int dup2(int oldfd, int newfd);

#endif
