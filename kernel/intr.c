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

#include <kernel/common.h>
#include <kernel/i386.h>
#include <string.h>

#define INTR_GATE 0xE
#define TRAP_GATE 0xF

/* table of ISRs for error reporting */
extern void(*int_errs[48])(void);

struct idt_entry {
	unsigned int off_low : 16;
	unsigned int selector : 16;
	unsigned int reserved : 5;
	unsigned int zero : 3;
	unsigned int type : 5;
	unsigned int dpl : 2;
	unsigned int present : 1;
	unsigned int off_high : 16;
};

struct idt_entry idt[256];

#define IDT_GATE(num,handler,selector)		\
{						\
	.off_low	= handler,		\
	.selector	= selector,		\
	.zero		= 0,			\
	.type		= INTR_GATE,		\
	.dpl		= 3,			\
	.present	= 1,			\
	.off_high	= handler >> 16		\
}

/*-----------------------------------------------------------------------------
 * Installs a trap gate at vector [num] with the given handler */
//-----------------------------------------------------------------------------
void set_gate(unsigned int num, unsigned long handler,
		unsigned short selector)
{
	idt[num] = (struct idt_entry) IDT_GATE(num, handler, selector);
}

/*-----------------------------------------------------------------------------
 * Installs an interrupt descriptor table in which all exceptions and hardware
 * interrupts signal an error and halt the kernel */
//-----------------------------------------------------------------------------
void idt_install(void)
{
	memset(idt, 0, sizeof(struct idt_entry) * 256);
	for (int i = 0; i < 48; i++)
		set_gate(i, (unsigned long) int_errs[i], SEG_KCODE);
	load_idt(&idt, sizeof(struct idt_entry) * 256);
}
