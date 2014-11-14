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
#include <sys/exec.h>
#include <sys/wait.h>
#include <string.h>

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

#define HEAP_START   0x00100000UL
#define HEAP_SIZE    FRAME_SIZE
#define STACK_START  0x0F000000UL
#define STACK_SIZE   (FRAME_SIZE*8)
#define KSTACK_START (STACK_START+STACK_SIZE)
#define KSTACK_SIZE  (FRAME_SIZE*4)

#define DATA_FLAGS   (VM_READ | VM_WRITE)
#define RODATA_FLAGS VM_READ
#define HEAP_FLAGS   (VM_READ | VM_WRITE | VM_ZERO)
#define USTACK_FLAGS (VM_READ | VM_WRITE | VM_ZERO | VM_EXEC)
#define KSTACK_FLAGS (USTACK_FLAGS | VM_ALLOC | VM_KEEP)

static int address_space_init(struct mm_struct *mm)
{
	struct vma *vma;
	mm->brk = HEAP_START + HEAP_SIZE;
	mm->heap = vma_map(mm, HEAP_START, HEAP_SIZE, HEAP_FLAGS);
	if (!mm->heap)
		goto abort;
	mm->stack = vma_map(mm, STACK_START, STACK_SIZE, USTACK_FLAGS);
	if (!mm->stack)
		goto abort;
	mm->kernel_stack = vma_map(mm, KSTACK_START, KSTACK_SIZE, KSTACK_FLAGS);
	if (!mm->kernel_stack)
		goto abort;
	vma = vma_map(mm, urwstart, urwend - urwstart, DATA_FLAGS | VM_KEEP);
	if (!vma)
		goto abort;
	vma = vma_map(mm, urostart, uroend - urostart, RODATA_FLAGS | VM_KEEP);
	if (!vma)
		goto abort;
	return 0;
abort:
	return -ENOMEM;
}

#define KFRAME_ROOM (sizeof(struct kcontext) + 16)

int create_kernel_process(void(*func)(void*), void *arg, ulong flags)
{
	int error;
	struct pcb *p;
	unsigned char cx_mem[KFRAME_ROOM];
	struct kcontext *frame = (struct kcontext*) cx_mem;

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

	if ((error = mm_init(&p->mm)) < 0)
		return error;;
	if ((error = address_space_init(&p->mm)) < 0)
		goto abort;
	if (!vma_map(&p->mm, krostart, kroend - krostart, RODATA_FLAGS)) {
		error = -ENOMEM;
		goto abort;
	}

	p->esp = ((char*)p->mm.kernel_stack->end - KFRAME_ROOM);

	put_iret_kframe(frame, (ulong)func);
	frame->stack[0] = (ulong) exit;
	frame->stack[1] = (ulong) arg;
	vm_copy_to(&p->mm, p->esp, frame, KFRAME_ROOM);

	ready(p);
	return p->pid;
abort:
	mm_fini(&p->mm);
	return error;
}

void create_init(void(*func)(void*))
{
	int error;
	long esp;
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
	if ((error = address_space_init(&p->mm)) < 0)
		panic("address_space_init failed with error %d", error);

	p->ifp = (char*)p->mm.kernel_stack->end - 16;
	p->esp = (char*)p->ifp - sizeof(struct ucontext);

	current = p;
	set_page_directory((void*)p->mm.pgdir);
	esp = p->mm.stack->end - 16;
	put_iret_uframe(p->esp, (ulong)func, esp);

	ready(p);

}

static int verify_exec_args(struct exec_args *args)
{
	int error;

	if (vm_verify(&current->mm, args, sizeof(*args), VM_READ))
		return -EFAULT;
	if (vm_verify(&current->mm, args->argv,
				sizeof(*(args->argv)) * args->argc, VM_READ))
		return -EFAULT;
	if (vm_verify(&current->mm, args->envp,
				sizeof(*(args->envp)) * args->envc, VM_READ))
		return -EFAULT;
	error = verify_user_string(args->pathname.str, args->pathname.len);
	if (error)
		return error;
	for (size_t i = 0; i < args->argc; i++) {
		error = verify_user_string(args->argv[i].str, args->argv[i].len);
		if (error)
			return error;
	}
	for (size_t i = 0; i < args->envc; i++) {
		error = verify_user_string(args->envp[i].str, args->envp[i].len);
		if (error)
			return error;
	}
	return 0;
}

static size_t exec_mem_needed(struct exec_args *args)
{
	size_t size = 0;
	size += (args->argc + 1) * sizeof(char*);
	size += (args->envc + 1) * sizeof(char*);
	for (size_t i = 0; i < args->argc; i++)
		size += args->argv[i].len + 1;
	for (size_t i = 0; i < args->envc; i++)
		size += args->envp[i].len + 1;
	return size;
}

static int execve_copy(struct exec_args *args, void **argv_loc, void **envp_loc)
{
	int error;
	char *mem, *kmem;
	char **argv;
	char **envp;
	size_t size = exec_mem_needed(args);
	ssize_t diff = size - vma_size(current->mm.heap);

	// need more memory in heap
	if (diff > 0) {
		error = vma_grow_up(current->mm.heap, diff, current->mm.heap->flags);
		if (error)
			return error;
	}

	// copy args/env into kernel memory
	kmem = kmalloc(size);
	if (!kmem)
		return -ENOMEM;
	mem = (char*) ((char**) kmem + args->argc + args->envc + 2);
	for (size_t i = 0; i < args->argc; i++) {
		memcpy(mem, args->argv[i].str, args->argv[i].len + 1);
		mem += args->argv[i].len + 1;
	}
	for (size_t i = 0; i < args->envc; i++) {
		memcpy(mem, args->envp[i].str, args->envp[i].len + 1);
		mem += args->envp[i].len + 1;
	}

	// copy back into user memory and populate argv/envp
	memcpy((void*) current->mm.heap->start, kmem, size);
	argv = (char**) current->mm.heap->start;
	envp = argv + args->argc + 1;
	mem = (char*) (envp + args->envc + 1);
	for (size_t i = 0; i < args->argc; i++) {
		argv[i] = mem;
		mem += args->argv[i].len + 1;
	}
	argv[args->argc] = NULL;
	for (size_t i = 0; i < args->envc; i++) {
		envp[i] = mem;
		mem += args->envp[i].len + 1;
	}
	envp[args->envc] = NULL;

	*argv_loc = argv;
	*envp_loc = envp;
	kfree(kmem);
	return 0;
}

long sys_execve(struct exec_args *args)
{
	int error;
	void *argv, *envp;
	unsigned long esp;
	unsigned long *main_args;
	struct inode *inode;

	error = verify_exec_args(args);
	if (error)
		return error;
	error = namei(args->pathname.str, &inode);
	if (error)
		return error;
	if (!S_ISFUN(inode->i_mode))
		return -EACCES;
	error = execve_copy(args, &argv, &envp);
	if (error)
		return error;

	sig_exec(&current->sig);
	current->ifp = (char*) current->mm.kernel_stack->end - 16;
	current->esp = (char*) current->ifp - sizeof(struct ucontext);

	esp = current->mm.stack->end - 16;
	put_iret_uframe(current->esp, (ulong)inode->i_private, esp);

	main_args = (unsigned long*) esp;
	main_args[0] = (unsigned long) exit;
	main_args[1] = args->argc;
	main_args[2] = (unsigned long) argv;
	main_args[3] = (unsigned long) envp;

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

	mm_fini(&current->mm);
	zombie(p);
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
