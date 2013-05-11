/* kernel.c : kernel (C) entry point
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

#include <kernel/kernel.h>
#include <kernel/common.h>
#include <kernel/multiboot.h>
#include <kernel/i386.h>
#include <kernel/mem.h>
#include <kernel/dispatch.h>
#include <kernel/drivers/console.h>

#include <string.h>

#define BOOT_CLR 0xA

pid_t idle_pid;
pid_t root_pid;

// TODO: find a more appropriate place for this
struct pcb proctab[PT_SIZE];

static void proctab_init (void) {
    for (int i = 0; i < PT_SIZE; i++) {
        proctab[i].pid   = i - PT_SIZE;
        proctab[i].state = STATE_STOPPED;
    }
    proctab[0].pid = 0; // 0 is a reserved pid
}

extern int console_init (void);

/*-----------------------------------------------------------------------------
 * Kernel entry point, where it all begins... */
//-----------------------------------------------------------------------------
void kmain (struct multiboot_info *info, unsigned long magic)
{
    unsigned long memtotal;

    // initialize console so we can print boot status
    console_init ();
    clear_console ();

    // check multiboot magic number
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf ("Invalid Multiboot magic number\n");
        return;
    }

    kprintf_clr (BOOT_CLR, "32 bit Telos 0.2\n");

    kprintf_clr (BOOT_CLR, "Initializing machine state...\n");
    idt_install ();
    gdt_install ();
    pic_init (0x20, 0x28); // map IRQs after reserved interrupts
    pit_init (100);        // 10ms timer

    kprintf_clr (BOOT_CLR, "Initializing kernel subsystems...\n");
    memtotal = mem_init (info);
    isr_init ();
    proctab_init ();
    dev_init ();
    dispatch_init ();

    kprintf_clr (BOOT_CLR, "\n----------- MEMORY -----------\n");
    kprintf_clr (BOOT_CLR, "Kernel:    %x - %x\n", &kstart, &kend);
    kprintf_clr (BOOT_CLR, "Userspace: %x - %x\n", &ustart, &uend);
    kprintf_clr (BOOT_CLR, "Total:     %d bytes\n\n", memtotal);

    kprintf_clr (BOOT_CLR, "Starting Telos...\n\n");

    idle_pid = sys_create (idle_proc, 0, NULL);
    root_pid = sys_create (root_proc, 0, NULL);
    dispatch ();
}
