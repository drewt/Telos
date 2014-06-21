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
#include <kernel/mem.h>
#include <kernel/device.h>
#include <kernel/list.h>
#include <kernel/signal.h>

#define STACK_PAGES 8
#define STACK_SIZE  (FRAME_SIZE * STACK_PAGES)

extern void exit(int status);

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
	p->fds[0] = DEV_CONSOLE_0;
	p->fds[1] = DEV_CONSOLE_0;
	p->fds[2] = DEV_CONSOLE_1;
	for (int i = 3; i < FDT_SIZE; i++)
		p->fds[i] = FD_NONE;

	/* init lists */
	INIT_LIST_HEAD(&p->send_q);
	INIT_LIST_HEAD(&p->recv_q);
	INIT_LIST_HEAD(&p->repl_q);
	INIT_LIST_HEAD(&p->heap_mem);
	INIT_LIST_HEAD(&p->page_mem);
	INIT_LIST_HEAD(&p->posix_timers);

	p->timestamp = tick_count;
	p->pid += PT_SIZE;
	p->parent_pid = parent;
	p->flags = flags;
}

long sys_create(void(*func)(int,char*), int argc, char **argv)
{
	return create_user_process(func, argc, argv, 0);
}

int create_kernel_process(void(*func)(int,char*), int argc, char **argv,
		ulong flags)
{
	struct pcb	*p;
	void		*stack_mem;
	void		*frame;
	ulong		*args;

	if ((p = get_free_pcb()) == NULL)
		return -EAGAIN;

	pcb_init(p, 0, flags | PFLAG_SUPER);

	p->pgdir = (pmap_t) KERNEL_TO_PHYS(&_kernel_pgd);

	if ((stack_mem = kmalloc(STACK_SIZE)) == NULL)
		return -ENOMEM;
	list_add_tail(&(mem_ptoh(stack_mem))->chain, &p->heap_mem);

	frame = ((char*) stack_mem + STACK_SIZE - sizeof(struct kcontext) - 128);
	put_iret_kframe(frame, (ulong) func);
	args = (ulong*) ((ulong) frame + sizeof(struct kcontext));

	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = (ulong) argv;

	p->esp = frame;

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

	arg_addr = p->heap_start + 128;
	uargv = kmap_tmp_range(p->pgdir, p->heap_start, 128);
	for (int i = 0; i < argc; i++, arg_addr += 128) {
		uargv[i] = (char*) arg_addr;
		copy_string_through_user(p, current, (void*) arg_addr,
				kargv[i], 128);
	}
	kunmap_range(uargv, 128);
}

int create_user_process(void(*func)(int,char*), int argc, char **argv,
		ulong flags)
{
	int rc;
	struct pcb	*p;
	struct pf_info	*it;
	ulong		v_stack, esp;
	ulong		*args;
	void		*v_frame;
	void		*p_frame;

	if ((p = get_free_pcb()) == NULL)
		return -EAGAIN;

	pcb_init(p, current->pid, flags);

	if ((rc = address_space_init(p)) < 0)
		return rc;

	p->heap_start = 0x00100000;
	p->heap_end = p->brk = p->heap_start + FRAME_SIZE;
	if (map_pages_user(p, p->heap_start, 1, PE_U | PE_RW))
		goto abort;

	copy_args(p, argc, argv);

	v_stack = 0x0F000000;
	if (map_pages_user(p, v_stack, STACK_PAGES, PE_U | PE_RW))
		goto abort;

	/* set up iret frame */
	v_frame = (void*) (v_stack + 32 * sizeof(struct ucontext));
	p_frame = kmap_tmp_range(p->pgdir, (ulong) v_frame,
			sizeof(struct ucontext));
	esp = v_stack + STACK_SIZE - 128;
	put_iret_uframe(p_frame, (ulong) func, esp);
	kunmap_range(p_frame, sizeof(struct ucontext));

	/* set up stack for main() */
	args = kmap_tmp_range(p->pgdir, esp, sizeof(ulong) * 3);
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = (ulong) p->heap_start;
	kunmap_range(args, sizeof(ulong) * 3);

	p->esp = v_frame;
	p->ifp = (void*) ((ulong) p->esp + sizeof(struct ucontext));

	ready(p);
	return p->pid;
abort:
	dequeue_iterate (it, struct pf_info, chain, &p->page_mem)
		kfree_page(it);
	return -ENOMEM;
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
	struct pf_info		*mit;

	// TODO: see what POSIX requires vis-a-vis process data in handler
	pit = &proctab[PT_INDEX(current->parent_pid)];
	__kill(pit, SIGCHLD);

	/* free memory allocated to process */
	dequeue_iterate (mit, struct pf_info, chain, &current->page_mem)
		kfree_page(mit);
	dequeue_iterate (hit, struct mem_header, chain, &current->heap_mem)
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

	for (int i = 0; i < FDT_SIZE; i++)
		if (current->fds[i] != FD_NONE)
			sys_close(current->fds[i]);

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
