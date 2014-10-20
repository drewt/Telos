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
#include <kernel/major.h>
#include <kernel/signal.h>
#include <kernel/stat.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/vma.h>
#include <kernel/drivers/console.h>

extern void exit(int status);

#define STDIO_FILE(name) \
	struct file name = { \
		.f_count = 2, \
	}

STDIO_FILE(stdin_file);
STDIO_FILE(stdout_file);
STDIO_FILE(stderr_file);

static void stdio_sysinit(void)
{
	struct file_operations *fops = get_chrfops(TTY_MAJOR);

	stdin_file.f_op = fops;
	stdin_file.f_inode = devfs_geti(DEVICE(TTY_MAJOR, 0));

	stdout_file.f_op = fops;
	stdout_file.f_inode = devfs_geti(DEVICE(TTY_MAJOR, 0));

	stderr_file.f_op = fops;
	stderr_file.f_inode = devfs_geti(DEVICE(TTY_MAJOR, 0));
}
EXPORT_KINIT(stdio, SUB_PROCESS, stdio_sysinit);

static inline struct pcb *get_free_pcb(void)
{
	int i;
	for (i = 0; i < PT_SIZE && proctab[i].state != STATE_STOPPED; i++)
		/* nothing */;
	return (i == PT_SIZE) ? NULL : &proctab[i];
}

static void pcb_init(struct pcb *p, pid_t parent, ulong flags)
{
	/* init signal data */
	p->sig_pending = 0;
	p->sig_accept  = 0;
	p->sig_ignore  = ~0;
	for (int i = 0; i < _TELOS_SIGMAX; i++) {
		p->sigactions[i] = default_sigactions[i];
		if (default_sigactions[i].sa_handler != SIG_IGN)
			p->sig_accept |= 1 << i;
	}

	/* init file data */
	p->filp[0] = &stdin_file;
	p->filp[1] = &stdout_file;
	p->filp[2] = &stderr_file;
	stdin_file.f_count++;
	stdout_file.f_count++;
	stderr_file.f_count++;
	for (int i = 3; i < NR_FILES; i++)
		p->filp[i] = NULL;

	/* init lists */
	INIT_LIST_HEAD(&p->send_q);
	INIT_LIST_HEAD(&p->recv_q);
	INIT_LIST_HEAD(&p->repl_q);
	INIT_LIST_HEAD(&p->mm.map);
	INIT_LIST_HEAD(&p->mm.kheap);
	INIT_LIST_HEAD(&p->posix_timers);

	p->t_alarm.flags = 0;
	p->t_sleep.flags = 0;

	p->timestamp = tick_count;
	p->pid += PT_SIZE;
	p->parent_pid = parent;
	p->flags = flags;
	p->state = STATE_NASCENT;

	struct pcb *pp = &proctab[PT_INDEX(parent)];
	if (pp->pid == parent) {
		p->root = pp->root;
		p->pwd = pp->pwd;
	} else {
		p->root = root_inode;
		p->pwd = root_inode;
	}
}

static struct pcb *pcb_clone(struct pcb *src)
{
	struct pcb *p;

	if (!(p = get_free_pcb()))
		return NULL;

	p->esp = src->esp;
	p->ksp = src->ksp;
	p->ifp = src->ifp;

	p->sig_pending = 0;
	p->sig_accept = src->sig_accept;
	p->sig_ignore = src->sig_ignore;
	for (int i = 0; i < _TELOS_SIGMAX; i++)
		p->sigactions[i] = src->sigactions[i];

	for (int i = 0; i < NR_FILES; i++)
		if ((p->filp[i] = src->filp[i]))
			p->filp[i]->f_count++;

	INIT_LIST_HEAD(&p->send_q);
	INIT_LIST_HEAD(&p->recv_q);
	INIT_LIST_HEAD(&p->repl_q);
	INIT_LIST_HEAD(&p->mm.map);
	INIT_LIST_HEAD(&p->mm.kheap);
	INIT_LIST_HEAD(&p->posix_timers);

	p->t_alarm.flags = 0;
	p->t_sleep.flags = 0;

	p->timestamp = tick_count;
	p->pid += PT_SIZE;
	p->parent_pid = src->pid;
	p->flags = src->flags;
	p->state = STATE_NASCENT;

	p->root = src->root;
	p->pwd = src->pwd;
	return p;
}

long sys_create(void(*func)(int,char*), int argc, char **argv)
{
	return create_user_process(func, argc, argv, 0);
}

int create_kernel_process(void(*func)(void*), void *arg, ulong flags)
{
	int rc;
	struct pcb *p;
	void *frame;
	ulong *args;

	if ((p = get_free_pcb()) == NULL)
		return -EAGAIN;

	pcb_init(p, 0, flags | PFLAG_SUPER);

	if ((rc = mm_init(&p->mm)) < 0)
		return rc;

	p->ksp = (void*)p->mm.kernel_stack->end;
	p->esp = ((char*)p->mm.stack->end - sizeof(struct kcontext) - 128);

	frame = kmap_tmp_range(p->mm.pgdir, (ulong)p->esp, sizeof(struct kcontext) + 16);
	put_iret_kframe((void*)frame, (ulong)func);

	args = (ulong*) ((ulong)frame + sizeof(struct kcontext));
	args[0] = (ulong) exit;
	args[1] = (ulong) arg;
	kunmap_tmp_range(frame, sizeof(struct kcontext) + 16);

	ready(p);
	return p->pid;
}

/* TODO: something better than this crap */
static void copy_args(struct pcb *p, int argc, char **argv)
{
	char *kargv[argc];
	char **uargv;
	ulong arg_addr;

	copy_from_current(kargv, argv, sizeof(char*) * argc);

	arg_addr = p->mm.heap->start + 128;
	uargv = kmap_tmp_range(p->mm.pgdir, p->mm.heap->start, 128);
	for (int i = 0; i < argc; i++, arg_addr += 128) {
		uargv[i] = (char*) arg_addr;
		copy_string_through_user(p, current, (void*) arg_addr,
				kargv[i], 128);
	}
	kunmap_tmp_range(uargv, 128);
}

int create_user_process(void(*func)(int,char*), int argc, char **argv,
		ulong flags)
{
	int rc;
	struct pcb	*p;
	ulong		esp;
	ulong		*args;
	void		*v_frame;
	void		*p_frame;

	if ((p = get_free_pcb()) == NULL)
		return -EAGAIN;

	pcb_init(p, current->pid, flags);

	if ((rc = mm_init(&p->mm)) < 0)
		return rc;

	p->ksp = (char*)p->mm.kernel_stack->end;

	copy_args(p, argc, argv);

	/* set up iret frame */
	v_frame = (void*) (p->mm.stack->start + 32 * sizeof(struct ucontext));
	p_frame = kmap_tmp_range(p->mm.pgdir, (ulong) v_frame,
			sizeof(struct ucontext));
	esp = p->mm.stack->end - 128;
	put_iret_uframe(p_frame, (ulong) func, esp);
	kunmap_tmp_range(p_frame, sizeof(struct ucontext));

	/* set up stack for main() */
	args = kmap_tmp_range(p->mm.pgdir, esp, sizeof(ulong) * 3);
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = p->mm.heap->start;
	kunmap_tmp_range(args, sizeof(ulong) * 3);

	p->esp = v_frame;
	p->ifp = (void*) ((ulong) p->esp + sizeof(struct ucontext));

	ready(p);
	return p->pid;
}

long sys_fcreate(const char *pathname, int argc, char **argv)
{
	int error;
	struct inode *inode;

	error = namei(pathname, &inode);
	if (error)
		return error;
	if (!S_ISFUN(inode->i_mode))
		return -EINVAL;

	return create_user_process(inode->i_private, argc, argv, 0);
}

#define ARG_MAX 32
#include <string.h>
static int _copy_args(char **argv)
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
	void *v_frame, *p_frame;
	struct inode *inode;

	error = namei(pathname, &inode);
	if (error)
		return error;
	if (!S_ISFUN(inode->i_mode))
		return -EINVAL;

	argc = _copy_args(argv);

	/* set up iret frame */
	v_frame = (void*) (current->mm.stack->start + 32 * sizeof(struct ucontext));
	p_frame = kmap_tmp_range(current->mm.pgdir, (ulong)v_frame,
			sizeof(struct ucontext));
	esp = current->mm.stack->end - 128;
	put_iret_uframe(p_frame, (ulong)inode->i_private, esp);
	kunmap_tmp_range(p_frame, sizeof(struct ucontext));

	/* set up stack for main() */
	args = kmap_tmp_range(current->mm.pgdir, esp, sizeof(ulong) * 3);
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = current->mm.heap->start;
	kunmap_tmp_range(args, sizeof(ulong) * 3);

	current->esp = v_frame;
	current->ifp = (void*) ((ulong)current->esp + sizeof(struct ucontext));
	return 0;
}

long sys_fork(void)
{
	int rc;
	struct pcb *p;

	if (!(p = pcb_clone(current)))
		return -EAGAIN;

	if ((rc = mm_clone(&p->mm, &current->mm)) < 0)
		return rc;

	p->rc = 0;
	ready(p);
	return p->pid;
}

/*-----------------------------------------------------------------------------
 * Yeild control to another process */
//-----------------------------------------------------------------------------
long sys_yield(void)
{
	ready(current);
	new_process();
	return 0;
}

/*-----------------------------------------------------------------------------
 * Stop the current process and free all resources associated with it */
//-----------------------------------------------------------------------------
long sys_exit(int status)
{
	struct pcb		*pit;
	struct mem_header	*hit;

	// TODO: see what POSIX requires vis-a-vis process data in handler
	pit = &proctab[PT_INDEX(current->parent_pid)];
	__kill(pit, SIGCHLD);

	/* free memory allocated to process */
	dequeue_iterate (hit, struct mem_header, chain, &current->mm.kheap)
		kfree(hit->data);

	current->state = STATE_STOPPED;

	#define CLEAR_MSG_QUEUE(q)			\
	dequeue_iterate (pit, struct pcb, chain, (q)) {	\
		pit->rc = SYSERR;			\
		ready(pit);				\
	}
	CLEAR_MSG_QUEUE(&current->send_q)
	CLEAR_MSG_QUEUE(&current->recv_q)
	CLEAR_MSG_QUEUE(&current->repl_q)

	for (int i = 0; i < NR_FILES; i++)
		if (current->filp[i] != NULL)
			sys_close(i);

	del_pgdir(current->mm.pgdir);
	mm_fini(&current->mm);

	new_process();
	return 0;
}

/*-----------------------------------------------------------------------------
 * Return the current process's pid */
//-----------------------------------------------------------------------------
long sys_getpid(void)
{
	return current->pid;
}
