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

#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

#define F_DUPFD         1
#define F_DUP2FD        2
#define F_DUPFD_CLOEXEC 3
#define F_GETFD         4
#define F_SETFD         5
#define F_GETFL         6
#define F_SETFL         7
#define F_GETLK         8
#define F_SETLK         9
#define F_SETLKW        10
#define F_GETOWN        11
#define F_SETOWN        12

#define FD_CLOEXEC 1

#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

#define O_RDONLY    0x1
#define O_WRONLY    0x2
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_EXEC      0x4
#define O_SEARCH    0x8
#define O_APPEND    0x10
#define O_CLOEXEC   0x20
#define O_CREAT     0x40
#define O_DIRECTORY 0x80
#define O_DSYNC     0x100
#define O_EXCL      0x200
#define O_NOCTTY    0x400
#define O_NOFOLLOW  0x800
#define O_NONBLOCK  0x1000
#define O_RSYNC     0x2000
#define O_SYNC      0x4000
#define O_TRUNC     (0x8000 | O_WRONLY)
#define O_TTY_INIT  0x10000

#ifdef __KERNEL__
#define O_READ  O_RDONLY
#define O_WRITE O_WRONLY
#endif

#endif
