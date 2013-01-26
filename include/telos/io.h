/* io.h : I/O system calls
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
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

#ifndef __TELOS_IO_H_
#define __TELOS_IO_H_

#include <telos/devices.h>

#define EOF 4

int open (int devno);
int close (int fd);
int read (int fd, void *buf, int nbyte);
int write (int fd, void *buf, int nbyte);

int getchar (void);
char *gets (char *s, int size);

#endif // __TELOS_IO_H_
