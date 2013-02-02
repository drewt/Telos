/* console.h : console driver
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

#ifndef __CONSOLE_H_
#define __CONSOLE_H_

#include <telos/console.h>

int console_init (void);
int console_write (int fd, void *buf, int buf_len);
int console_open (enum dev_id devno);
int console_close (enum dev_id devno);
int console_ioctl (int fd, unsigned long command, va_list vargs);

#endif // __CONSOLE_H_
