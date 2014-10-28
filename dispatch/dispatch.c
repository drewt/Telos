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

const int NR_SYSCALLS = SYSCALL_MAX;

#define next() (list_dequeue(&ready_queue, struct pcb, chain))

struct sysaction {
	isr_t func;
	int nargs;
};

isr_t systab[SYSCALL_MAX] = {
	[SYS_CREATE]		= (isr_t) sys_create,
	[SYS_FCREATE]		= (isr_t) sys_fcreate,
	[SYS_EXECVE]		= (isr_t) sys_execve,
	[SYS_FORK]		= (isr_t) sys_fork,
	[SYS_YIELD]		= (isr_t) sys_yield,
	[SYS_STOP]		= (isr_t) sys_exit,
	[SYS_GETPID]		= (isr_t) sys_getpid,
	[SYS_SLEEP]		= (isr_t) sys_sleep,
	[SYS_SIGRETURN]		= (isr_t) sig_restore,
	[SYS_KILL]		= (isr_t) sys_kill,
	[SYS_SIGQUEUE]		= (isr_t) sys_sigqueue,
	[SYS_SIGWAIT]		= (isr_t) sys_sigwait,
	[SYS_SIGACTION]		= (isr_t) sys_sigaction,
	[SYS_SIGMASK]		= (isr_t) sys_sigprocmask,
	[SYS_OPEN]		= (isr_t) sys_open,
	[SYS_CLOSE]		= (isr_t) sys_close,
	[SYS_READ]		= (isr_t) sys_read,
	[SYS_WRITE]		= (isr_t) sys_write,
	[SYS_LSEEK]		= (isr_t) sys_lseek,
	[SYS_READDIR]		= (isr_t) sys_readdir,
	[SYS_IOCTL]		= (isr_t) sys_ioctl,
	[SYS_MKNOD]		= (isr_t) sys_mknod,
	[SYS_MKDIR]		= (isr_t) sys_mkdir,
	[SYS_RMDIR]		= (isr_t) sys_rmdir,
	[SYS_CHDIR]		= (isr_t) sys_chdir,
	[SYS_LINK]		= (isr_t) sys_link,
	[SYS_UNLINK]		= (isr_t) sys_unlink,
	[SYS_RENAME]		= (isr_t) sys_rename,
	[SYS_MOUNT]		= (isr_t) sys_mount,
	[SYS_UMOUNT]		= (isr_t) sys_umount,
	[SYS_STAT]		= (isr_t) sys_stat,
	[SYS_ALARM]		= (isr_t) sys_alarm,
	[SYS_SEND]		= (isr_t) sys_send,
	[SYS_RECV]		= (isr_t) sys_recv,
	[SYS_REPLY]		= (isr_t) sys_reply,
	[SYS_TIMER_CREATE]	= (isr_t) sys_timer_create,
	[SYS_TIMER_DELETE]	= (isr_t) sys_timer_delete,
	[SYS_TIMER_GETTIME]	= (isr_t) sys_timer_gettime,
	[SYS_TIMER_SETTIME]	= (isr_t) sys_timer_settime,
	[SYS_TIME]		= (isr_t) sys_time,
	[SYS_CLOCK_GETRES]	= (isr_t) sys_clock_getres,
	[SYS_CLOCK_GETTIME]	= (isr_t) sys_clock_gettime,
	[SYS_CLOCK_SETTIME]	= (isr_t) sys_clock_settime,
	[SYS_SBRK]		= (isr_t) sys_sbrk,
};

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
