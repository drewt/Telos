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
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <kernel/device.h>
#include <kernel/list.h>

#include <signal.h>

extern void exit(int status);

static inline struct pcb *get_free_pcb(void)
{
	int i;
	for (i = 0; i < PT_SIZE && proctab[i].state != STATE_STOPPED; i++)
		/* nothing */;
	return (i == PT_SIZE) ? NULL : &proctab[i];
}

static void pcb_init(struct pcb *p)
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
	p->fds[0] = DEV_KBD_ECHO;
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

	/* init timers */
	p->t_alarm = NULL;
	p->t_sleep = NULL;
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

	p->timestamp = tick_count;
	p->pid += PT_SIZE;
	p->parent_pid = 0;
	p->flags = flags | PFLAG_SUPER;
	p->pgdir = (pmap_t) KERNEL_TO_PHYS(&_kernel_pgd);

	pcb_init(p);

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

int create_user_process(void(*func)(int,char*), int argc, char **argv,
		ulong flags)
{
	struct pcb	*p;
	ulong		v_stack, esp;
	ulong		*args;
	void		*v_frame;

	if ((p = get_free_pcb()) == NULL)
		return -EAGAIN;

	p->timestamp = tick_count;
	p->pid += PT_SIZE;
	p->parent_pid = current->pid;
	p->flags = flags;

	pcb_init(p);

	p->pgdir = pgdir_create(&p->page_mem);
	if (p->pgdir == NULL)
		return -ENOMEM;

#if 1
	ulong v_heap = 0x01000000;
	if (map_pages(p->pgdir, v_heap, 1, PE_U | PE_RW, &p->page_mem))
		return -ENOMEM;

	char *kargv[argc];
	copy_from_current(kargv, argv, sizeof(char*) * argc);

	ulong arg_addr = v_heap + 128;
	char ** uargv = (void*) kmap_tmp_range(p->pgdir, v_heap, 128);
	for (int i = 0; i < argc; i++, arg_addr += 128) {
		uargv[i] = (char*) arg_addr;
		copy_string_through_user(p, current, (void*) arg_addr,
				kargv[i], 128);
	}

	void *p_frame;
	v_stack = 0x00C00000;
	if (map_pages(p->pgdir, v_stack, 8, PE_U | PE_RW, &p->page_mem))
		return -ENOMEM;

	v_frame = (void*) (v_stack + 32 * sizeof(struct ucontext));
	p_frame = (void*) kmap_tmp_range(p->pgdir, (ulong) v_frame,
			sizeof(struct ucontext));
	esp = v_stack + STACK_SIZE - 128;
	put_iret_uframe(p_frame, (ulong) func, esp);
	kunmap_range((ulong) p_frame, sizeof(struct ucontext));

	args = (void*) kmap_tmp_range(p->pgdir, esp, sizeof(ulong) * 3);
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = (ulong) v_heap;
	kunmap_range((ulong) args, sizeof(ulong) * 3);

#else
	if ((v_stack = (ulong) kmalloc(STACK_SIZE)) == 0)
		return -ENOMEM;
	list_insert_tail(&p->heap_mem, (list_entry_t)mem_ptoh((void*)v_stack));

	v_frame = (void*) ((ulong) v_stack + 32*U_CONTEXT_SIZE);
	esp = v_stack + STACK_SIZE - 128;
	put_iret_frame(v_frame, (ulong) func, esp);

	args = (ulong*) esp;
	args[0] = (ulong) exit;
	args[1] = (ulong) argc;
	args[2] = (ulong) argv;
#endif

	p->esp = v_frame;
	p->ifp = (void*) ((ulong) p->esp + sizeof(struct ucontext));

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
	struct pf_info		*mit;

	// TODO: see what POSIX requires vis-a-vis process data in handler
	pit = &proctab[PT_INDEX(current->parent_pid)];
	__kill(pit, SIGCHLD);

	// free memory allocated to process
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
