/* intr.c : interrupt descriptor table
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

#include <kernel/common.h>
#include <kernel/i386.h>
#include <string.h>

#define INTR_GATE 0xE
#define TRAP_GATE 0xF

/* table of ISRs for error reporting */
extern void(*int_errs[48])(void);

struct idt_entry {
    uint16_t off_low;
    uint16_t selector;
    uint32_t reserved : 5;
    uint32_t zero : 3;
    uint32_t type : 5;
    uint32_t dpl : 2;
    uint32_t present : 1;
    uint16_t off_high;
};

struct idt_entry idt[256];

/*-----------------------------------------------------------------------------
 * Installs a trap gate at vector [num] with the given handler */
//-----------------------------------------------------------------------------
void set_gate (uint8_t num, unsigned long handler, int selector) {
    idt[num].off_low  = handler;
    idt[num].selector = selector;
    idt[num].zero     = 0;
    idt[num].type     = INTR_GATE;
    idt[num].dpl      = 3;
    idt[num].present  = 1;
    idt[num].off_high = handler >> 16;
}

/*-----------------------------------------------------------------------------
 * Installs an interrupt descriptor table in which all exceptions and hardware
 * interrupts signal an error and halt the kernel */
//-----------------------------------------------------------------------------
void idt_install (void) {
    memset (idt, 0, sizeof (struct idt_entry) * 256);
    for (int i = 0; i < 48; i++)
        set_gate (i, (unsigned long) int_errs[i], SEG_KCODE);
    load_idt (&idt, sizeof (struct idt_entry) * 256);
}
