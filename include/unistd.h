/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TELOS_UNISTD_H_
#define _TELOS_UNISTD_H_

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int ssize_t;
#endif

typedef int pid_t;

enum std_fds {
    STDIN_FILENO,
    STDOUT_FILENO,
    STDERR_FILENO,
};

unsigned int alarm (unsigned int seconds);
pid_t getpid (void);
unsigned int sleep (unsigned int seconds);
int open (const char *pathname, int flags, ...);
int close (int fd);
ssize_t read (int fd, void *buf, size_t nbyte);
ssize_t write (int fd, const void *buf, size_t nbyte);
int ioctl (int fd, int command, ...);

#endif
