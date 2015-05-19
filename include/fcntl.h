/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FNCTL_H_
#define _FNCTL_H_

#include <sys/fcntl.h>

//int creat(const char *, mode_t);
int fcntl(int fd, int cmd, ...);
int open(const char *, int, ...);
//int openat(int, const char *, int, ...);
//int posix_fadvise(int, off_t, off_t, int);
//int posix_fallocate(int, off_t, off_t);

#endif
