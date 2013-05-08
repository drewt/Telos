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

struct gdt_entry gdt[GDT_N_ENTRIES]; /* global descriptor table */
struct tss_entry tss;                /* task state segment      */

/*-----------------------------------------------------------------------------
 * Sets a descriptor in the GDT with the given values.  attr is bits 40 -> 47;
 * bits 48->63 are set to default values. */
//-----------------------------------------------------------------------------
static void set_descriptor (int num, unsigned long base, unsigned long limit,
        unsigned char attr)
{
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].type = attr & 0xF;
    gdt[num].sys = (attr >> 4) & 0x1;
    gdt[num].dpl = (attr >> 5) & 0x3;
    gdt[num].present = (attr >> 7) & 0x1;
    gdt[num].limit_high = (limit >> 16) & 0xF;
    gdt[num].avl = 0;
    gdt[num].l = 0;
    gdt[num].db = 1;
    gdt[num].gran = 1;
    gdt[num].base_high = (base >> 24) & 0xFF;
}

/*-----------------------------------------------------------------------------
 * Initializes the task state segment */
//-----------------------------------------------------------------------------
static void init_tss (void)
{
    memset (&tss, 0, sizeof (struct tss_entry));
    tss.ss0  = SEG_KDATA;
    tss.esp0 = 0;
    tss.cs = SEG_KCODE | 3;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = SEG_KDATA | 3;
}

/*-----------------------------------------------------------------------------
 * Routine to create and load the kernel's global descriptor table */
//-----------------------------------------------------------------------------
void gdt_install (void)
{
    unsigned long tss_base = (unsigned long) &tss;

    init_tss ();

    // set up a basic, flat address space
    set_descriptor (0,          0, 0x00000000, 0x00); // null descriptor
    set_descriptor (KCODE_SEGN, 0, 0xFFFFFFFF, 0x9A); // kernel code
    set_descriptor (KDATA_SEGN, 0, 0xFFFFFFFF, 0x92); // kernel data
    set_descriptor (UCODE_SEGN, 0, 0xFFFFFFFF, 0xFA); // user code
    set_descriptor (UDATA_SEGN, 0, 0xFFFFFFFF, 0xF2); // user data
    set_descriptor (TSS_SEGN, tss_base, tss_base + sizeof tss, 0xE9);

    load_gdt (&gdt, sizeof gdt); // -1?
    load_tss (SEG_TSS | 3); // load tss with RPL 3
}
