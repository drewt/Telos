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

#ifndef _KERNEL_COMMON_H_
#define _KERNEL_COMMON_H_

#define __KERNEL__

#include <kernel/types.h>
#include <errnodefs.h>

#define SYSERR (-1)

#define STACK_SIZE (1024*16)

#define wprintf(fmt, ...) kprintf_clr(0xC, "WARNING: "fmt"\n", __VA_ARGS__)
#define wprints(str) kprintf_clr(0xC, "WARNING: "str"\n")

/* linux */
#define likely(x) x
#define unlikely(x) x
#define WARN_ON_ONCE(x) x

#define ULLONG_MAX (~0ULL)
#define INT_MAX ((int)(~0U>>1))
#define USHRT_MAX ((u16)(~0U))
#define SHRT_MAX ((s16)(USHRT_MAX>>1))

extern int kprintf(const char *fmt, ...) __attribute__((format(printf,1,2)));
extern int kprintf_clr(unsigned char clr, const char *fmt, ...);
extern void clear_console(void);

#endif
