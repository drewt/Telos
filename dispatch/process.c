/* process.c : process management
 */

/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
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

extern void exit (int status);

static inline struct pcb *get_free_pcb (void)
{
    int i;
    for (i = 0; i < PT_SIZE && proctab[i].state != STATE_STOPPED; i++)
        /* nothing */;
    return (i == PT_SIZE) ? NULL : &proctab[i];
}

static void pcb_init (struct pcb *p)
{
    // init signal data
    p->sig_pending = 0;
    p->sig_accept  = 0;
    p->sig_ignore  = ~0;
    for (int i = 0; i < _TELOS_SIGMAX; i++) {
        p->sigactions[i] = default_sigactions[i];
        if (default_sigactions[i].sa_handler != SIG_IGN)
            p->sig_accept |= 1 << i;
    }

    // init file data
    p->fds[0] = DEV_KBD_ECHO;
    p->fds[1] = DEV_CONSOLE_0;
    p->fds[2] = DEV_CONSOLE_1;
    for (int i = 3; i < FDT_SIZE; i++)
        p->fds[i] = FD_NONE;

    // init lists
    list_init (&p->send_q);
    list_init (&p->recv_q);
    list_init (&p->repl_q);
    list_init (&p->heap_mem);
    list_init (&p->page_mem);
}

void sys_create (void (*func)(int,char*), int argc, char **argv)
{
    current->rc = create_user_process (func, argc, argv, 0);
}

int create_kernel_process (void (*func)(int,char*), int argc, char **argv,
        ulong flags)
{
    struct pcb  *p;
    void        *stack_mem;
    void        *frame;
    ulong       *args;

    if ((p = get_free_pcb ()) == NULL)
        return -EAGAIN;

    if ((stack_mem = kmalloc (STACK_SIZE)) == NULL)
        return -ENOMEM;

    p->stack_mem = stack_mem;
    p->timestamp = tick_count;
    p->pid += PT_SIZE;
    p->parent_pid = 0;
    p->flags = flags | PFLAG_SUPER;
    p->pgdir = (pmap_t) KERNEL_TO_PHYS (&_kernel_pgd);

    pcb_init (p);

    frame = ((char*) stack_mem + STACK_SIZE - S_CONTEXT_SIZE - 128);
    put_iret_frame_super (frame, (ulong) func);
    args = (ulong*) ((ulong) frame + S_CONTEXT_SIZE);

    args[0] = (ulong) exit;
    args[1] = (ulong) argc;
    args[2] = (ulong) argv;

    p->esp = frame;

    ready (p);
    return p->pid;
}

int create_user_process (void (*func)(int,char*), int argc, char **argv,
        ulong flags)
{
    struct pcb  *p;
    ulong       v_stack, esp;
    ulong       *args;
    //void        *p_frame;
    void        *v_frame;

    if ((p = get_free_pcb ()) == NULL)
        return -EAGAIN;

    p->timestamp = tick_count;
    p->pid += PT_SIZE;
    p->parent_pid = current->pid;
    p->flags = flags;

    pcb_init (p);

    p->pgdir = pgdir_create (&p->page_mem);
    if (p->pgdir == NULL)
        return -ENOMEM;

    /*v_stack = 0x00C00000;
    if (map_pages (p->pgdir, v_stack, 8, PE_U | PE_RW, &p->page_mem))
        return -ENOMEM;

    v_frame = (void*) (v_stack + 32 * U_CONTEXT_SIZE);
    p_frame = (void*) virt_to_phys (p->pgdir, (ulong) v_frame);
    esp = v_stack + STACK_SIZE - 128;
    put_iret_frame (p_frame, (ulong) func, esp);

    args = (ulong*) virt_to_phys (p->pgdir, esp);*/

    if ((v_stack = (ulong) kmalloc (STACK_SIZE)) == 0)
        return -ENOMEM;

    v_frame = (void*) ((ulong) v_stack + 32*U_CONTEXT_SIZE);
    esp = v_stack + STACK_SIZE - 128;
    put_iret_frame (v_frame, (ulong) func, esp);
    args = (ulong*) esp;

    args[0] = (ulong) exit;
    args[1] = (ulong) argc;
    args[2] = (ulong) argv;

    p->esp = v_frame;
    p->ifp = (void*) ((ulong) p->esp + U_CONTEXT_SIZE);

    ready (p);
    return p->pid;
}

/*-----------------------------------------------------------------------------
 * Yeild control to another process */
//-----------------------------------------------------------------------------
void sys_yield (void)
{
    ready (current);
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Stop the current process and free all resources associated with it */
//-----------------------------------------------------------------------------
void sys_exit (int status)
{
    struct pcb          *pit;
    struct mem_header   *hit;
    struct pf_info      *mit;

    // TODO: see what POSIX requires vis-a-vis process data in handler
    pit = &proctab[PT_INDEX (current->parent_pid)];
    __kill (pit, SIGCHLD);

    // free memory allocated to process
    //kfree (current->stack_mem);
    dequeue_iterate (&current->page_mem, mit, struct pf_info*)
        kfree_page (mit);
    dequeue_iterate (&current->heap_mem, hit, struct mem_header*)
        kfree (hit->data);

    current->state = STATE_STOPPED;

    #define CLEAR_MSG_QUEUE(q)                \
    dequeue_iterate ((q), pit, struct pcb*) { \
        pit->rc = SYSERR;                     \
        ready (pit);                          \
    }
    CLEAR_MSG_QUEUE (&current->send_q)
    CLEAR_MSG_QUEUE (&current->recv_q)
    CLEAR_MSG_QUEUE (&current->repl_q)

    for (int i = 0; i < FDT_SIZE; i++)
        if (current->fds[i] != FD_NONE)
            sys_close (current->fds[i]);

    new_process ();
}

/*-----------------------------------------------------------------------------
 * Return the current process's pid */
//-----------------------------------------------------------------------------
void sys_getpid (void)
{
    current->rc = current->pid;
}
