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

#include <kernel/device.h>
#include <kernel/list.h>
#include <kernel/signal.h>
#include <kernel/timer.h>

#define PT_SIZE  256
#define PID_MASK (PT_SIZE - 1)

#define PT_INDEX(pid) ((pid) & PID_MASK)

#define FDT_SIZE 8
#define FD_NONE  (DT_SIZE + 1)

/* process status codes */
enum {
	STATE_STOPPED		= 0,
	STATE_RUNNING		= 1 << 1,
	STATE_READY		= 1 << 2,
	STATE_BLOCKED		= 1 << 3,
	STATE_SIGWAIT		= 1 << 4,
	STATE_SIGSUSPEND	= 1 << 5,
	STATE_SLEEPING		= 1 << 6
};

enum {
	PFLAG_SUPER = 1
};

struct pbuf {
	void *buf; // buffer
	int  len;  // length of buffer
	int  id;   // id (context dependent)
};

/* process control block */
struct pcb {
	struct list_head chain;
	/* metadata */
	int		pid;		// process ID
	int		parent_pid;	// parent process's pid
	unsigned int	state;		// state
	long		rc;		// return value for system calls
	unsigned long	flags;
	/* memory */
	void		*stack_mem;	// beginning of stack memory
	void		*int_stack;	// stack for interrupts
	void		*esp;		// stack pointer
	void		*ifp;		// interrupt frame pointer
	unsigned long	*pgdir;		// page directory
	unsigned long   heap_start;
	unsigned long   heap_end;
	unsigned long	brk;
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
	/* */
	void		*parg;		// pointer to... something
	dev_t		fds[FDT_SIZE];	// file descriptors
	struct list_head heap_mem;	// heap allocated memory
	struct list_head page_mem;
};

extern struct pcb proctab[];
extern const struct sigaction default_sigactions[_TELOS_SIGMAX];

int create_user_process(void(*func)(int,char*), int argc, char **argv,
		unsigned long flags);
int create_kernel_process(void(*func)(int,char*), int argc, char **argv,
		unsigned long flags);


#endif
