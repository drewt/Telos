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

#include <kernel/i386.h>
#include <string.h>

#define GDT_N_ENTRIES 6

struct gdt_entry {
	unsigned int limit_low : 16;
	unsigned int base_low : 16;
	unsigned int base_mid : 8;
	unsigned int type : 4;
	unsigned int sys : 1;
	unsigned int dpl : 2;
	unsigned int present : 1;
	unsigned int limit_high : 4;
	unsigned int avl : 1;
	unsigned int l : 1;
	unsigned int db : 1;
	unsigned int gran : 1;
	unsigned int base_high : 8;
};

#define SEG_NULL { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

#define SEG(base,limit,attr)				\
{							\
	.limit_low	= (limit) & 0xFFFF,		\
	.base_low	= (base) & 0xFFFF,		\
	.base_mid	= ((base) >> 16) & 0xFF,	\
	.type		= (attr) & 0xF,			\
	.sys		= ((attr) >> 4) & 0x1,		\
	.dpl		= ((attr) >> 5) & 0x3,		\
	.present	= ((attr) >> 7) & 0x1,		\
	.limit_high	= ((limit) >> 16) & 0xF,	\
	.avl		= 0,				\
	.l		= 0,				\
	.db		= 1,				\
	.gran		= 1,				\
	.base_high	= ((base) >> 24) & 0xFF		\
}

struct gdt_entry gdt[GDT_N_ENTRIES] = {
	[0]		= SEG_NULL,
	[SEGNR_KCODE]	= SEG(0, 0xFFFFFFFF, 0x9A),
	[SEGNR_KDATA]	= SEG(0, 0xFFFFFFFF, 0x92),
	[SEGNR_UCODE]	= SEG(0, 0xFFFFFFFF, 0xFA),
	[SEGNR_UDATA]	= SEG(0, 0xFFFFFFFF, 0xF2)
};

struct tss_entry tss = {
	.ss0	= SEG_KDATA,
	.esp0	= 0,
	.cs	= SEG_KCODE | 3,
	.ss	= SEG_KDATA | 3,
	.ds	= SEG_KDATA | 3,
	.es	= SEG_KDATA | 3,
	.fs	= SEG_KDATA | 3,
	.gs	= SEG_KDATA | 3
};

/* from osdev.org wiki inline asm examples (modified to update selectors) */
static inline void load_gdt(void *base, unsigned short size)
{
	volatile struct {
		uint16_t length;
		uint32_t base;
	} __packed gdtr = { size, (uint32_t) base };
	asm volatile(
	"lgdt (%0)		\n"
	"ljmp %1,   $setcs	\n"
"setcs:				\n"
	"movw %2,   %%ax	\n"
	"movw %%ax, %%ds	\n"
	"movw %%ax, %%es	\n"
	"movw %%ax, %%ss	\n"
	:
	: "g" (&gdtr), "i" (SEG_KCODE), "i" (SEG_KDATA)
	: "%eax"
	);
}

static inline void load_tss(unsigned short val)
{
	asm volatile(
	"movw %0, %%ax	\n"
	"ltr  %%ax	\n"
	: : "g" (val) : "%eax"
	);
}

/*-----------------------------------------------------------------------------
 * Routine to create and load the kernel's global descriptor table */
//-----------------------------------------------------------------------------
void gdt_install(void)
{
	unsigned long tss_base  = (unsigned long) &tss;
	unsigned long tss_limit = tss_base + sizeof tss;
	gdt[SEGNR_TSS] = (struct gdt_entry) SEG(tss_base, tss_limit, 0xE9);

	load_gdt(&gdt, sizeof gdt); // -1?
	load_tss(SEG_TSS | 3); // load tss with RPL 3
}
