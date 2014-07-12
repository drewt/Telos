/*  Copyright 2013 Drew Thoreson
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

#ifndef _KERNEL_COMMON_H_
#define _KERNEL_COMMON_H_

#define __KERNEL__

#include <stdarg.h>
#include <kernel/types.h>
#include <kernel/compiler.h>
#include <errnodefs.h>

#define SYSERR (-1)

/* linux */
#define WARN_ON_ONCE(x) x

#define ULLONG_MAX (~0ULL)
#define INT_MAX ((int)(~0U>>1))
#define USHRT_MAX ((u16)(~0U))
#define SHRT_MAX ((s16)(USHRT_MAX>>1))

void clear_console(void);
int _kvprintf(unsigned char attr, const char *fmt, va_list ap) __printf(2,0);
int _kprintf(unsigned char attr, const char *fmt, ...) __printf(2,3);

#define TXT_CLR   0x7
#define kvprintf(fmt,ap) _kvprintf(TXT_CLR, fmt, ap)
#define kprintf(fmt, ...) _kprintf(TXT_CLR, fmt, ## __VA_ARGS__)
#define warn(fmt, ...) _kprintf(0xC, "WARNING: "fmt"\n", ## __VA_ARGS__)

_Noreturn void panic(const char *fmt, ...) __printf(1,2);

/* initialization order */
enum {
	SUB_MEMORY,
	SUB_VFS,
	SUB_DRIVER,
	SUB_PROCESS,

	SUB_LAST
};

struct kinit_struct {
	ulong subsystem;
	void(*func)(void);
};

static inline void breakpoint(void)
{
	asm("int $3");
}

/* export declarations */

#define EXPORT(set, sym) \
	asm(".section .set." #set ",\"aw\""); \
	asm(".long " #sym); \
	asm(".previous")

#define EXPORT_KINIT(uniq, subsys, fun) \
	static struct kinit_struct uniq ## _kinit __used = { \
		.subsystem = subsys, \
		.func = fun, \
	}; \
	EXPORT(kinit, uniq ## _kinit);

#endif
