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
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/interrupt.h>
#include <kernel/list.h>
#include <syscall.h>

typedef void(*isr_t)();

/* routines defined in other files */
extern unsigned int context_switch(struct pcb *p);
extern int send_signal(struct pcb *p, int sig_no);
extern void tick(void);

struct pcb *current = NULL;	/* the running process */
static LIST_HEAD(ready_queue); 	/* queue of ready processes */

#define next() (list_dequeue(&ready_queue, struct pcb, chain))

struct sysaction {
	isr_t func;
	int nargs;
};

/* table of actions to be taken for interrupts/system calls */
static struct sysaction sysactions[SYSCALL_MAX] = {
/*	INDEX			ACTION			NR ARGS */
	[EXN_FPE]	= { (isr_t) exn_fpe,		0 },
	[EXN_ILL]	= { (isr_t) exn_ill_instr,	0 },
	[EXN_PF]	= { (isr_t) exn_page_fault,	0 },
	[INTR_TIMER]	= { (isr_t) tick,		0 },
	[SYS_CREATE]	= { (isr_t) sys_create,		3 },
	[SYS_YIELD]	= { (isr_t) sys_yield,		0 },
	[SYS_STOP]	= { (isr_t) sys_exit,		1 },
	[SYS_GETPID]	= { (isr_t) sys_getpid,		0 },
	[SYS_REPORT]	= { (isr_t) sys_report,		1 },
	[SYS_SLEEP]	= { (isr_t) sys_sleep,		1 },
	[SYS_SIGRETURN]	= { (isr_t) sig_restore,	1 },
	[SYS_KILL]	= { (isr_t) sys_kill,		2 },
	[SYS_SIGQUEUE]	= { (isr_t) sys_sigqueue,	3 },
	[SYS_SIGWAIT]	= { (isr_t) sys_sigwait,	0 },
	[SYS_SIGACTION]	= { (isr_t) sys_sigaction,	4 },
	[SYS_SIGNAL]	= { (isr_t) sys_signal,		2 },
	[SYS_SIGMASK]	= { (isr_t) sys_sigprocmask,	3 },
	[SYS_OPEN]	= { (isr_t) sys_open,		3 },
	[SYS_CLOSE]	= { (isr_t) sys_close,		1 },
	[SYS_READ]	= { (isr_t) sys_read,		3 },
	[SYS_WRITE]	= { (isr_t) sys_write,		3 },
	[SYS_IOCTL]	= { (isr_t) sys_ioctl,		3 },
	[SYS_ALARM]	= { (isr_t) sys_alarm,		1 },
	[SYS_SEND]	= { (isr_t) sys_send,		5 },
	[SYS_RECV]	= { (isr_t) sys_recv,		3 },
	[SYS_REPLY]	= { (isr_t) sys_reply,		3 },
	[SYS_MALLOC]	= { (isr_t) sys_malloc,		2 },
	[SYS_FREE]	= { (isr_t) sys_free,		1 },
	[SYS_PALLOC]	= { (isr_t) sys_palloc,		1 }
};

static inline void set_action(unsigned int vector, isr_t f, int nargs)
{
	sysactions[vector] = (struct sysaction) { f, nargs };
}

/*-----------------------------------------------------------------------------
 * Initializes the dispatcher.  Must be called *after* the device table is
 * initialized, if any device ISRs are assigned dynamically */
//-----------------------------------------------------------------------------
void dispatch_init(void)
{
	/* initialize actions that can't be initialized statically */
	set_action(INTR_KBD, (isr_t) devtab[DEV_KBD].dv_op->dviint, 0);
}

/*-----------------------------------------------------------------------------
 * The dispatcher.  Passes control to the appropriate routines to handle 
 * interrupts, system calls, and other events */
//*----------------------------------------------------------------------------
void dispatch(void)
{
	unsigned int req, sig_no;
	struct sys_args args;

	current = next();
	for (;;) {

		if (current->sig_pending & current->sig_ignore) {
			sig_no = 31;
			while (sig_no && !(current->sig_pending >> sig_no))
				sig_no--;
			send_signal(current, sig_no);
		}

		req  = context_switch(current);

		if (req < SYSCALL_MAX && sysactions[req].func != NULL) {

			if (sysactions[req].nargs > 0)
				copy_from_userspace(current->pgdir, &args,
						current->esp, sizeof args);

			switch (sysactions[req].nargs) {
			case 0:
				sysactions[req].func();
				break;
			case 1:
				sysactions[req].func(args.arg0);
				break;
			case 2:
				sysactions[req].func(args.arg0, args.arg1);
				break;
			case 3:
				sysactions[req].func(args.arg0, args.arg1,
						args.arg2);
				break;
			case 4:
				sysactions[req].func(args.arg0, args.arg1,
						args.arg2, args.arg3);
				break;
			case 5:
				sysactions[req].func(args.arg0, args.arg1,
						args.arg2, args.arg3, args.arg4);
				break;
			}
		} else {
			__kill(current, SIGSYS);
		}
	}
}

/*-----------------------------------------------------------------------------
 * Enqueues a process on the ready queue */
//-----------------------------------------------------------------------------
void ready(struct pcb *p)
{
	p->state = STATE_READY;
	list_add_tail(&p->chain, &ready_queue);
}

/*-----------------------------------------------------------------------------
 * Selects a new process to run, choosing the idle process only if there are no
 * other processes ready to run */
//-----------------------------------------------------------------------------
void new_process(void)
{
	current = next();

	/* skip idle process if possible */
	if (current->pid == idle_pid && !list_empty(&ready_queue)) {
		ready(current);
		current = next();
	}
}
