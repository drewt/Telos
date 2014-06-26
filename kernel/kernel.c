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

#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <kernel/i386.h>
#include <kernel/mmap.h>
#include <kernel/dispatch.h>
#include <kernel/time.h>
#include <kernel/drivers/console.h>

#include <string.h>

pid_t idle_pid;
pid_t root_pid;

// TODO: find a more appropriate place for this
struct pcb proctab[PT_SIZE];

struct multiboot_info *mb_info;

static void proctab_init (void)
{
	for (int i = 0; i < PT_SIZE; i++) {
		proctab[i].pid   = i - PT_SIZE;
		proctab[i].state = STATE_STOPPED;
	}
	proctab[0].pid = 0; // 0 is a reserved pid
}
EXPORT_KINIT(process, SUB_PROCESS, proctab_init);

static void init_subsystems(void)
{
	/* (bubble) sort kinit_set */
	for (unsigned i = 1; i < kinit_set_length; i++) {
		if (kinit_set[i]->subsystem < kinit_set[i-1]->subsystem) {
			struct kinit_struct *tmp = kinit_set[i];
			kinit_set[i] = kinit_set[i-1];
			kinit_set[i-1] = tmp;
			i = 0;
		}
	}

	for (unsigned i = 0; i < kinit_set_length; i++) {
		kinit_set[i]->func();
	}
}

/*-----------------------------------------------------------------------------
 * Kernel entry point, where it all begins... */
//-----------------------------------------------------------------------------
void kmain(struct multiboot_info *info, unsigned long magic)
{
	#define bprintf(fmt, ...) kprintf_clr(0xA, fmt, ## __VA_ARGS__)

	mb_info = info;

	/* initialize console so we can print boot status */
	console_early_init();
	clear_console();

	/* check multiboot magic number */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		kprintf("Invalid Multiboot magic number\n");
		return;
	}

	bprintf("32 bit Telos 0.2\n");

	bprintf("Initializing machine state...\n");
	idt_install();
	isr_init();
	gdt_install();
	pic_init(0x20, 0x28);	// map IRQs after exceptions/reserved vectors
	pit_init(100);		// 10ms timer
	clock_init();

	bprintf("Initializing kernel subsystems...\n");
	init_subsystems();

	bprintf("\n----------- MEMORY -----------\n");
	bprintf("Kernel:    %p - %p\n", &_kstart, &_kend);
	bprintf("Userspace: %p - %p\n", &_ustart, &_uend);
	bprintf("Total:     %d bytes\n", MULTIBOOT_MEM_MAX(mb_info));

	bprintf("Starting Telos...\n\n");
	idle_pid = create_kernel_process(idle_proc, 0, NULL, 0);
	root_pid = create_kernel_process(root_proc, 0, NULL, 0);
	dispatch();

	#undef bprintf
}
