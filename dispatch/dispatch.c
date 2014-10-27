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

#include <kernel/bitops.h>
#include <kernel/dispatch.h>
#include <kernel/interrupt.h>
#include <kernel/list.h>
#include <syscall.h>

typedef long(*isr_t)();

/* routines defined in other files */
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
	[EXN_FPE]		= { (isr_t) exn_fpe,		0 },
	[EXN_ILL]		= { (isr_t) exn_ill_instr,	0 },
	[EXN_PF]		= { (isr_t) exn_page_fault,	0 },
	[INTR_TIMER]		= { (isr_t) tick,		0 },
	[INTR_KBD]		= { (isr_t) int_keyboard,	0 },
	[SYS_CREATE]		= { (isr_t) sys_create,		2 },
	[SYS_FCREATE]		= { (isr_t) sys_fcreate,	1 },
	[SYS_EXECVE]		= { (isr_t) sys_execve,		3 },
	[SYS_FORK]		= { (isr_t) sys_fork,		0 },
	[SYS_YIELD]		= { (isr_t) sys_yield,		0 },
	[SYS_STOP]		= { (isr_t) sys_exit,		1 },
	[SYS_GETPID]		= { (isr_t) sys_getpid,		0 },
	[SYS_SLEEP]		= { (isr_t) sys_sleep,		1 },
	[SYS_SIGRETURN]		= { (isr_t) sig_restore,	1 },
	[SYS_KILL]		= { (isr_t) sys_kill,		2 },
	[SYS_SIGQUEUE]		= { (isr_t) sys_sigqueue,	3 },
	[SYS_SIGWAIT]		= { (isr_t) sys_sigwait,	0 },
	[SYS_SIGACTION]		= { (isr_t) sys_sigaction,	4 },
	[SYS_SIGNAL]		= { (isr_t) sys_signal,		2 },
	[SYS_SIGMASK]		= { (isr_t) sys_sigprocmask,	3 },
	[SYS_OPEN]		= { (isr_t) sys_open,		3 },
	[SYS_CLOSE]		= { (isr_t) sys_close,		1 },
	[SYS_READ]		= { (isr_t) sys_read,		3 },
	[SYS_WRITE]		= { (isr_t) sys_write,		3 },
	[SYS_LSEEK]		= { (isr_t) sys_lseek,		3 },
	[SYS_READDIR]		= { (isr_t) sys_readdir,	3 },
	[SYS_IOCTL]		= { (isr_t) sys_ioctl,		3 },
	[SYS_MKNOD]		= { (isr_t) sys_mknod,		3 },
	[SYS_MKDIR]		= { (isr_t) sys_mkdir,		2 },
	[SYS_RMDIR]		= { (isr_t) sys_rmdir,		1 },
	[SYS_CHDIR]		= { (isr_t) sys_chdir,		1 },
	[SYS_LINK]		= { (isr_t) sys_link,		2 },
	[SYS_UNLINK]		= { (isr_t) sys_unlink,		1 },
	[SYS_RENAME]		= { (isr_t) sys_rename,		2 },
	[SYS_MOUNT]		= { (isr_t) sys_mount,          5 },
	[SYS_UMOUNT]		= { (isr_t) sys_umount,		1 },
	[SYS_STAT]		= { (isr_t) sys_stat,		2 },
	[SYS_ALARM]		= { (isr_t) sys_alarm,		1 },
	[SYS_SEND]		= { (isr_t) sys_send,		5 },
	[SYS_RECV]		= { (isr_t) sys_recv,		3 },
	[SYS_REPLY]		= { (isr_t) sys_reply,		3 },
	[SYS_TIMER_CREATE]	= { (isr_t) sys_timer_create,	3 },
	[SYS_TIMER_DELETE]	= { (isr_t) sys_timer_delete,	1 },
	[SYS_TIMER_GETTIME]	= { (isr_t) sys_timer_gettime,	2 },
	[SYS_TIMER_SETTIME]	= { (isr_t) sys_timer_settime,	4 },
	[SYS_TIME]		= { (isr_t) sys_time,		1 },
	[SYS_CLOCK_GETRES]	= { (isr_t) sys_clock_getres,	2 },
	[SYS_CLOCK_GETTIME]	= { (isr_t) sys_clock_gettime,	2 },
	[SYS_CLOCK_SETTIME]	= { (isr_t) sys_clock_settime,	2 },
	[SYS_SBRK]		= { (isr_t) sys_sbrk,		2 },
};

/*-----------------------------------------------------------------------------
 * The dispatcher.  Passes control to the appropriate routines to handle 
 * interrupts, system calls, and other events */
//*----------------------------------------------------------------------------
void dispatch(ulong call, struct sys_args *args)
{
	struct pcb *p = current;

	if (call < SYSCALL_MIN && sysactions[call].func != NULL) {
		sysactions[call].func();
	} else if (call < SYSCALL_MAX && sysactions[call].func != NULL) {

		p = current;
		switch (sysactions[call].nargs) {
		case 0:
			p->rc = sysactions[call].func();
			break;
		case 1:
			p->rc = sysactions[call].func(args->arg0);
			break;
		case 2:
			p->rc = sysactions[call].func(args->arg0,
					args->arg1);
			break;
		case 3:
			p->rc = sysactions[call].func(args->arg0,
					args->arg1, args->arg2);
			break;
		case 4:
			p->rc = sysactions[call].func(args->arg0,
					args->arg1, args->arg2,
					args->arg3);
			break;
		case 5:
			p->rc = sysactions[call].func(args->arg0,
					args->arg1, args->arg2,
					args->arg3, args->arg4);
			break;
		}
	} else {
		__kill(current, SIGSYS);
	}
}

_Noreturn void kernel_start(void)
{
	current = next();
	switch_to(current);
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
