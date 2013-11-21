/* gdt.c : global descriptor table
 */

/*  Copyright 2013 Drew T
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

#include <kernel/common.h>
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
	[KCODE_SEGN]	= SEG(0, 0xFFFFFFFF, 0x9A),
	[KDATA_SEGN]	= SEG(0, 0xFFFFFFFF, 0x92),
	[UCODE_SEGN]	= SEG(0, 0xFFFFFFFF, 0xFA),
	[UDATA_SEGN]	= SEG(0, 0xFFFFFFFF, 0xF2)
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

/*-----------------------------------------------------------------------------
 * Routine to create and load the kernel's global descriptor table */
//-----------------------------------------------------------------------------
void gdt_install(void)
{
	unsigned long tss_base  = (unsigned long) &tss;
	unsigned long tss_limit = tss_base + sizeof tss;
	gdt[TSS_SEGN] = (struct gdt_entry) SEG(tss_base, tss_limit, 0xE9);

	load_gdt(&gdt, sizeof gdt); // -1?
	load_tss(SEG_TSS | 3); // load tss with RPL 3
}
