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

#ifndef _KERNEL_PROCESS_H_
#define _KERNEL_PROCESS_H_

#define PCB_RC  0x08
#define PCB_ESP 0x0C
#define PCB_KSP 0x10
#define PCB_IFP 0x14
#define PCB_FLG 0x18
#define PCB_PID 0x1C
#define PCB_PGD 0x20

#define PFLAG_SUPER 0x1

#ifndef __ASM__

#include <kernel/list.h>
#include <kernel/fs.h>
#include <kernel/signal.h>
#include <kernel/timer.h>
#include <kernel/mm/vma.h>

#define PT_SIZE  256
#define PID_MASK (PT_SIZE - 1)

#define PT_INDEX(pid) ((pid) & PID_MASK)

#define FDT_SIZE 8
#define FD_NONE  (DT_SIZE + 1)

#define NR_FILES 8

/* process status codes */
enum {
	STATE_STOPPED		= 0,
	STATE_RUNNING		= 1 << 1,
	STATE_READY		= 1 << 2,
	STATE_BLOCKED		= 1 << 3,
	STATE_SIGWAIT		= 1 << 4,
	STATE_SIGSUSPEND	= 1 << 5,
	STATE_SLEEPING		= 1 << 6,
	STATE_NASCENT		= 1 << 7
};

struct pbuf {
	void *buf; // buffer
	int  len;  // length of buffer
	int  id;   // id (context dependent)
};

/* process control block */
struct pcb {
	struct list_head chain;
	long		rc;		// return value for system calls
	void		*esp;		// process stack pointer
	void		*ksp;		// kernel stack pointer
	void		*ifp;		// interrupt frame pointer
	unsigned long	flags;
	int		pid;		// process ID
	struct mm_struct mm;
	/* metadata */
	int		parent_pid;	// parent process's pid
	unsigned int	state;		// state
	/* time */
	unsigned int	timestamp;	// creation time
	struct timer	t_alarm;	// alarm timer
	struct timer	t_sleep;	// sleep timer
	struct list_head posix_timers;
	timer_t		posix_timer_id;
	/* signals */
	struct sigaction sigactions[_TELOS_SIGMAX];	// signal handlers
	struct siginfo   siginfos[_TELOS_SIGMAX];	// signal information
	u32		sig_pending;	// bitmask for pending signals
	u32		sig_accept;	// bitmask for accepted signals
	u32		sig_ignore;	// bitmask for ignored signals
	/* message passing IPC */
	struct pbuf	pbuf;		// saved buffer
	struct pbuf	reply_blk;
	struct list_head send_q;	// processes waiting to send
	struct list_head recv_q;	// processes waiting to receive
	struct list_head repl_q;	// processes waiting for a reply
	/* vfs */
	struct file *filp[FDT_SIZE];
	/* */
	void		*parg;		// pointer to... something
};

#define assert_pcb_offset(member, offset) \
	_Static_assert(offsetof(struct pcb, member) == offset, \
			"Misaligned PCB member: " #member)

assert_pcb_offset(rc,    PCB_RC);
assert_pcb_offset(esp,   PCB_ESP);
assert_pcb_offset(ifp,   PCB_IFP);
assert_pcb_offset(flags, PCB_FLG);
assert_pcb_offset(pid,   PCB_PID);
assert_pcb_offset(mm,    PCB_PGD);
_Static_assert(offsetof(struct mm_struct, pgdir) == 0,
		"Misaligned PCB member: pgdir");

extern struct pcb proctab[];
extern const struct sigaction default_sigactions[_TELOS_SIGMAX];

int create_user_process(void(*func)(int,char*), int argc, char **argv,
		unsigned long flags);
int create_kernel_process(void(*func)(void*), void *arg, ulong flags);

#endif /* __ASM__ */
#endif /* _KERNEL_PROCESS_H_ */
