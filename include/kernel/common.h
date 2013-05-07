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

#ifndef __COMMON_H_
#define __COMMON_H_

#include <kernel/types.h>

#define SYSERR (-1)

#define STACK_SIZE (1024*16)

extern int kprintf (const char *fmt, ...);
extern int kprintf_clr (unsigned char clr, const char *fmt, ...);
extern void clear_console (void);

#endif // __COMMON_H_
