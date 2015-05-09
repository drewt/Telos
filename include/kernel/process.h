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
#define PCB_IFP 0x10
#define PCB_PGD 0x14

#ifndef __ASSEMBLER__

#include <kernel/list.h>
#include <kernel/signal.h>
#include <kernel/timer.h>
#include <kernel/mm/vma.h>

#define PT_SIZE  256
#define PID_MASK (PT_SIZE - 1)

#define NR_FILES 8

/* process status codes */
enum {
	PROC_DEAD,
	PROC_NASCENT,
	PROC_READY,
	PROC_RUNNING,
	PROC_STOPPED,
	PROC_ZOMBIE,
	PROC_SIGWAIT,
	PROC_WAITING,
	PROC_INTERRUPTIBLE,
};

enum {
	PFLAG_SUPER     = 1,
};

struct inode;
struct file;

/* process control block */
struct pcb {
	struct list_head  chain;
	long              rc;
	void              *esp;
	void              *ifp;
	struct mm_struct  mm;
	/* metadata */
	pid_t             pid;
	pid_t             parent_pid;
	unsigned long     flags;
	unsigned int      state;
	struct list_head  child_stats;
	struct list_head  children;
	struct list_head  child_chain;
	struct list_head  wait_chain;
	/* time */
	unsigned int      timestamp;
	struct timer      t_alarm;
	struct timer      t_sleep;
	struct list_head  posix_timers;
	timer_t	          posix_timer_id;
	/* signals */
	struct sig_struct sig;
	/* vfs */
	struct file       *filp[NR_FILES];
	struct inode      *pwd;
	struct inode      *root;
};

#define assert_pcb_offset(member, offset) \
	assert_struct_offset(struct pcb, member, offset)
assert_pcb_offset(rc,    PCB_RC);
assert_pcb_offset(esp,   PCB_ESP);
assert_pcb_offset(ifp,   PCB_IFP);
assert_pcb_offset(mm,    PCB_PGD);
assert_struct_offset(struct mm_struct, pgdir, 0);

extern struct pcb proctab[PT_SIZE];
extern struct pcb *current;

int create_user_process(void(*func)(void*), void *arg, unsigned long flags);
int create_kernel_process(void(*func)(void*), void *arg, ulong flags);

/* scheduling */
_Noreturn void switch_to(struct pcb *p);
void ready(struct pcb *p);
void zombie(struct pcb *p);
void reap(struct pcb *p);
void wake(struct pcb *p, long rc);
long schedule(void);

static inline struct pcb *get_pcb(pid_t pid)
{
	struct pcb *p = &proctab[pid & PID_MASK];
	if (p->state != PROC_DEAD && p->pid == pid)
		return p;
	return NULL;
}

static inline struct file *fd_file(struct pcb *p, int fd)
{
	return (fd >= 0 && fd < NR_FILES && p->filp[fd]) ? p->filp[fd] : NULL;
}

static inline int fd_ok(struct pcb *p, int fd)
{
	return fd < NR_FILES && p->filp[fd];
}

static inline int get_fd(struct pcb *p, int start)
{
	if (start >= NR_FILES)
		return -EINVAL;
	for (; start < NR_FILES; start++)
		if (!p->filp[start])
			return start;
	return -EMFILE;
}

#endif /* __ASSEMBLER__ */
#endif /* _KERNEL_PROCESS_H_ */
