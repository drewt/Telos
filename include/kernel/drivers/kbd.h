/* kbd.h : keyboard driver
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

#ifndef __KBD_H_
#define __KBD_H_

#include <telos/kbd.h>

int kbd_init (void);
void kbd_interrupt (void);
int kbd_open (dev_t devno);
int kbd_close (dev_t devno);
int kbd_read (int fd, void *buf, int nbytes);
int kbd_read_echo (int fd, void *buf, int nbytes);
int kbd_ioctl (int fd, unsigned long command, va_list vargs);

#endif // __KBD_H_
