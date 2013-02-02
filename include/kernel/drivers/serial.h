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

#ifndef __SERIAL_H_
#define __SERIAL_H_

int serial_init (void);
int serial_read (int fd, void *buf, int buf_len);
int serial_write (int fd, void *buf, int buf_len);
int serial_open (enum dev_id devno);
int serial_close (enum dev_id devno);
int serial_ioctl (int fd, unsigned long command, va_list vargs);
void serial_int (void);

#endif // __SERIAL_H
