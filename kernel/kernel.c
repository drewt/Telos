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

#include <kernel/dispatch.h>
#include <kernel/elf.h>
#include <kernel/fs.h>
#include <kernel/i386.h>
#include <kernel/kernel.h>
#include <kernel/mmap.h>
#include <kernel/multiboot.h>
#include <kernel/time.h>
#include <kernel/drivers/console.h>

#include <string.h>

struct multiboot_info *mb_info;
extern _Noreturn void sched_start(void);

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
		kprintf("Initializing %s... ", kinit_set[i]->name);
		kinit_set[i]->func();
		kprintf("done.\n");
	}
}

/*-----------------------------------------------------------------------------
 * Kernel entry point, where it all begins... */
//-----------------------------------------------------------------------------
void kmain(struct multiboot_info *info, unsigned long magic)
{
	#define bprintf(fmt, ...) _kprintf(0xA, fmt, ## __VA_ARGS__)

	mb_info = info;

	// initialize console so we can print boot status
	console_clear(0);

	// check multiboot magic number
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		kprintf("Invalid Multiboot magic number\n");
		return;
	}

	bprintf("32 bit Telos " VERSION "\n");

	bprintf("Initializing machine state...\n");
	idt_install();
	gdt_install();
	pic_init(0x20, 0x28);	// map IRQs after exceptions/reserved vectors
	pit_init(100);		// 10ms timer
	clock_init();

	bprintf("Initializing kernel subsystems...\n");
	init_subsystems();

	bprintf("\n----------- MEMORY -----------\n");
	bprintf("Kernel:    %p - %p\n", &_kstart, &_kend);
	bprintf("Userspace: %p - %p\n", &_ustart, &_uend);
	bprintf("Total:     %lx bytes\n", MULTIBOOT_MEM_MAX(mb_info));

	mount_root();

	bprintf("Starting Telos...\n\n");
	sched_start();

	#undef bprintf
}
