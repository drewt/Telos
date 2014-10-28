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

#include <kernel/i386.h>
#include <kernel/list.h>
#include <kernel/interrupt.h>
#include <kernel/dispatch.h>

/*
 * This function merely saves the current context on the stack, and then calls
 * _schedule().  This is accomplished with a software interrupt.  See
 * schedule_entry in entry.S for details.
 */
void schedule(void)
{
	void *esp = current->esp;
	kprintf("saved esp=%p for %d\n", esp, current->pid);
	asm volatile("int %[xnum]" : : [xnum] "i" (INTR_SCHEDULE) : );
	kprintf("resetoring esp=%p\n", esp);
	for(;;);
	current->esp = esp;
}

/*
 * Choose a new process to run.
 */
struct pcb *_schedule(void)
{
	kprintf("new esp=%p for %d\n", current->esp, current->pid);
	kprintf("eip=%p\n", (void*)((struct kcontext*)current->esp)->iret_eip);
	new_process();
	kprintf("switching to %d\n", current->pid);
	return current;
}
