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

pid_t idle_pid;
extern void init(void*);
extern void create_init(void(*func)(void*));

struct pcb *current = NULL;
static LIST_HEAD(ready_queue);
static LIST_HEAD(zombies);

#define next() (list_dequeue(&ready_queue, struct pcb, chain))

static _Noreturn void idle_proc(void *unused)
{
	asm volatile("_halt: hlt\njmp _halt");
	__builtin_unreachable();
}

_Noreturn void sched_start(void)
{
	idle_pid = create_kernel_process(idle_proc, NULL, 0);
	create_init(init);
	current = next();
	switch_to(current);
}

void ready(struct pcb *p)
{
	p->state = PROC_READY;
	list_add_tail(&p->chain, &ready_queue);
}

void reap(struct pcb *p)
{
	p->state = PROC_DEAD;
	list_del(&p->chain);
}

void wake(struct pcb *p, long rc)
{
	p->rc = rc;
	ready(p);
}

void zombie(struct pcb *p)
{
	p->state = PROC_ZOMBIE;
	list_add_tail(&p->chain, &zombies);
}

static struct pcb *next_process(void)
{
	struct pcb *p = next();

	// skip idle process if possible
	if (p->pid == idle_pid && !list_empty(&ready_queue)) {
		ready(p);
		p = next();
	}
	return p;
}

/*
 * Choose a new process to run.  This function is called from schedule()
 * in entry.S.
 */
struct pcb *_schedule(void)
{
	return (current = next_process());
}