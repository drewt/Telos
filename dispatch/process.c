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
#include <kernel/elf.h>
#include <kernel/dispatch.h>
#include <kernel/time.h>
#include <kernel/mmap.h>
#include <kernel/list.h>
#include <kernel/fs.h>
#include <kernel/signal.h>
#include <kernel/stat.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/vma.h>
#include <kernel/drivers/console.h>
#include <sys/wait.h>

extern void exit(int status);

struct pcb proctab[PT_SIZE];

struct proc_status {
	struct list_head chain;
	pid_t pid;
	int status;
};
DEFINE_SLAB_CACHE(proc_status_cachep, sizeof(struct proc_status));

static inline int status_to_si_code(int status)
{
	switch (WHOW(status)) {
	case _WCONTINUED: return CLD_CONTINUED;
	case _WEXITED:    return CLD_EXITED;
	case _WSIGNALED:  return CLD_KILLED;
	case _WSTOPPED:   return CLD_STOPPED;
	}
	return 0;
}

static void assert_status(struct pcb *parent, struct proc_status *status)
{
	list_add_tail(&status->chain, &parent->child_stats);
	__kill(parent, SIGCHLD, status_to_si_code(status->status));
}

void report_status(struct pcb *p, int how, int val)
{
	struct pcb *parent;
	struct proc_status *status;

	if (!(parent = get_pcb(p->parent_pid)))
		panic("Process %d has no parent", p->pid);
	if (!(status = slab_alloc(proc_status_cachep)))
		return; // FIXME: do something
	status->pid = p->pid;
	status->status = make_status(how, val);
	assert_status(parent, status);
}

static void proctab_init (void)
{
	for (int i = 0; i < PT_SIZE; i++) {
		proctab[i].pid   = i - PT_SIZE;
		proctab[i].state = PROC_DEAD;
	}
	proctab[0].pid = 0; // 0 is a reserved pid
}
EXPORT_KINIT(process, SUB_PROCESS, proctab_init);

static struct pcb *pcb_common_init(struct pcb *p)
{
	p->t_alarm.flags = 0;
	p->t_sleep.flags = 0;
	p->timestamp = tick_count;
	p->state = PROC_NASCENT;
	INIT_LIST_HEAD(&p->mm.map);
	INIT_LIST_HEAD(&p->mm.kheap);
	INIT_LIST_HEAD(&p->children);
	INIT_LIST_HEAD(&p->child_stats);
	INIT_LIST_HEAD(&p->posix_timers);
	return p;
}

static struct pcb *get_free_pcb(void)
{
	for (int i = 0; i < PT_SIZE; i++)
		if (proctab[i].state == PROC_DEAD)
			return pcb_common_init(&proctab[i]);
	return NULL;
}

static struct pcb *pcb_clone(struct pcb *src)
{
	struct pcb *p;

	if (!(p = get_free_pcb()))
		return NULL;

	p->esp = src->esp;
	p->ifp = src->ifp;

	sig_clone(&p->sig, &src->sig);

	for (int i = 0; i < NR_FILES; i++)
		if ((p->filp[i] = src->filp[i]))
			p->filp[i]->f_count++;

	p->pid += PT_SIZE;
	p->parent_pid = src->pid;
	p->flags = src->flags;

	p->root = src->root;
	p->pwd = src->pwd;

	list_add_tail(&p->child_chain, &src->children);
	return p;
}

int create_kernel_process(void(*func)(void*), void *arg, ulong flags)
{
	int error;
	struct kcontext *frame;
	struct pcb *p;

	if (!(p = get_free_pcb()))
		return -EAGAIN;
	sig_init(&p->sig);
	for (int i = 0; i < NR_FILES; i++)
		p->filp[i] = NULL;
	p->pid += PT_SIZE;
	p->parent_pid = 1;
	p->flags = flags | PFLAG_SUPER;
	p->root = root_inode;
	p->pwd = root_inode;

	if ((error = mm_kinit(&p->mm)) < 0)
		return error;

	p->esp = ((char*)p->mm.kernel_stack->end - sizeof(struct kcontext) - 16);

	frame = kmap_tmp_range(p->mm.pgdir, (ulong)p->esp, sizeof(struct kcontext) + 16);
	put_iret_kframe(frame, (ulong)func);
	frame->stack[0] = (ulong) exit;
	frame->stack[1] = (ulong) arg;
	kunmap_tmp_range(frame, sizeof(struct kcontext));

	ready(p);
	return p->pid;
}

void create_init(void(*func)(void*))
{
	int error;
	long esp;
	ulong *args;
	void *frame;
	struct pcb *p;

	p = pcb_common_init(&proctab[1]);
	sig_init(&p->sig);
	for (int i = 0; i < NR_FILES; i++)
		p->filp[i] = NULL;
	p->pid = 1;
	p->parent_pid = 1;
	p->flags = 0;
	p->root = root_inode;
	p->pwd = root_inode;

	if ((error = mm_init(&p->mm)) < 0)
		panic("mm_init failed with error %d", error);

	p->ifp = (char*)p->mm.kernel_stack->end - 16;
	p->esp = (char*)p->ifp - sizeof(struct ucontext);

	frame = kmap_tmp_range(p->mm.pgdir, (ulong)p->esp, sizeof(struct ucontext));
	esp = p->mm.stack->end - 16;
	put_iret_uframe(frame, (ulong)func, esp);
	kunmap_tmp_range(frame, sizeof(struct ucontext));

	args = kmap_tmp_range(p->mm.pgdir, esp, sizeof(ulong));
	args[0] = (ulong) exit;
	kunmap_tmp_range(args, sizeof(ulong));

	ready(p);

}

#define ARG_MAX 32
#include <string.h>
static int copy_args(char **argv)
{
	int argc;
	ulong addr = current->mm.heap->start + (ARG_MAX * sizeof(char*));
	char **uargv = (char**) current->mm.heap->start;

	if (!argv)
		return 0;

	for (argc = 0; argc < ARG_MAX && argv[argc]; argc++, addr += 128) {
		uargv[argc] = (char*) addr;
		memcpy((void*)addr, argv[argc], 128);
	}
	return argc;
}

long sys_execve(const char *pathname, char **argv, char **envp)
{
	int error, argc;
	ulong esp;
	ulong *args;
	void *frame;
	struct inode *inode;

	// TODO: verify addresses
	error = namei(pathname, &inode);
	if (error)
		return error;
	if (!S_ISFUN(inode->i_mode))
		return -EACCES;

	argc = copy_args(argv);
	sig_exec(&current->sig);

	current->ifp = (char*)current->mm.kernel_stack->end - 16;
	current->esp = (char*)current->ifp - sizeof(struct ucontext);

	frame = kmap_tmp_range(current->mm.pgdir, (ulong)current->esp, sizeof(struct ucontext));
	esp = current->mm.stack->end - 16;
	put_iret_uframe(frame, (ulong)inode->i_private, esp);
	kunmap_tmp_range(frame, sizeof(struct ucontext));

	args = kmap_tmp_range(current->mm.pgdir, esp, sizeof(ulong) * 3);
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = current->mm.heap->start;
	kunmap_tmp_range(args, sizeof(ulong) * 3);

	return 0;
}

long sys_fork(void)
{
	int rc;
	struct pcb *p;

	// set return value to zero for child
	((struct ucontext*)current->esp)->reg.eax = 0;

	if (!(p = pcb_clone(current)))
		return -EAGAIN;

	if ((rc = mm_clone(&p->mm, &current->mm)) < 0)
		return rc;

	ready(p);
	return p->pid;
}

long sys_yield(void)
{
	ready(current);
	schedule();
	return 0;
}

void do_exit(struct pcb *p, int status)
{
	struct pcb *pit;
	struct proc_status *sit, *n;
	struct mem_header *hit;

	report_status(p, _WEXITED, status);

	// free memory allocated to process
	dequeue_iterate(hit, struct mem_header, chain, &current->mm.kheap)
		kfree(hit->data);

	for (int i = 0; i < NR_FILES; i++)
		if (current->filp[i] != NULL)
			sys_close(i);

	// re-parent orphans to init
	list_for_each_entry(pit, &current->children, child_chain) {
		pit->parent_pid = 1;
	}
	// give status events to new parent
	if (!(pit = get_pcb(1)))
		panic("No init process");
	list_for_each_entry_safe(sit, n, &current->child_stats, chain) {
		list_del(&sit->chain);
		assert_status(pit, sit);
	}

	del_pgdir(current->mm.pgdir);
	mm_fini(&current->mm);
	zombie(p);
}

long sys_exit(int status)
{
	do_exit(current, status);
	schedule();
	return 0;
}

long sys_getpid(void)
{
	return current->pid;
}

long sys_getppid(void)
{
	return current->parent_pid;
}

static struct proc_status *get_status(idtype_t idtype, id_t id, int options)
{
	struct proc_status *s;
	list_for_each_entry(s, &current->child_stats, chain) {
		// TODO: P_PGID
		if (idtype == P_PID && s->pid != (pid_t)id)
			continue;
		switch (WHOW(s->status)) {
		case _WCONTINUED:
			if (options & WCONTINUED)
				return s;
			break;
		case _WEXITED:
			if (options & WEXITED)
				return s;
			break;
		case _WSTOPPED:
			if (options & WSTOPPED)
				return s;
			break;
		default:
			return s;
		}
	}
	return NULL;
}

long sys_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	int signo;
	struct proc_status *s;

	if (vm_verify(&current->mm, infop, sizeof(*infop), VM_WRITE))
		return -EFAULT;
	if (list_empty(&current->children))
		return -ECHILD;
	for (;;) {
		if ((s = get_status(idtype, id, options)))
			break;
		if (options & WNOHANG) {
			infop->si_signo = 0;
			infop->si_pid = 0;
			return 0;
		}

		current->state = PROC_WAITING;
		if ((signo = schedule()) != SIGCHLD)
			return -EINTR;
		if (signal_from_user(current->sig.infos[signo].si_code))
			return -EINTR;
	}
	infop->si_signo = SIGCHLD;
	infop->si_code = status_to_si_code(s->status);
	infop->si_errno = 0;
	infop->si_pid = s->pid;
	infop->si_uid = 0; // TODO
	infop->si_status = WVALUE(s->status);
	infop->si_value.sigval_int = s->status; // for wait[pid] wrapper

	// reap zombie
	if (WIFEXITED(s->status) || WIFSIGNALED(s->status)) {
		struct pcb *p = get_pcb(s->pid);
		if (!p)
			panic("Tried to reap non-existant child");
		list_del(&p->child_chain);
		reap(p);
	}

	list_del(&s->chain);
	slab_free(proc_status_cachep, s);
	return 0;
}
