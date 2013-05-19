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

static void sig_init (struct pcb *p)
{
    p->sig_pending = 0;
    p->sig_accept  = 0;
    p->sig_ignore  = ~0;

    for (int i = 0; i < _TELOS_SIGMAX; i++) {
        p->sigactions[i] = default_sigactions[i];
        if (default_sigactions[i].sa_handler != SIG_IGN)
            p->sig_accept |= 1 << i;
    }
}

static void files_init (struct pcb *p)
{
    p->fds[0] = DEV_KBD_ECHO;
    p->fds[1] = DEV_CONSOLE_0;
    p->fds[2] = DEV_CONSOLE_1;
    for (int i = 3; i < FDT_SIZE; i++)
        p->fds[i] = FD_NONE;
}

void sys_create (void (*func)(int,char*), int argc, char **argv)
{
    current->rc = create_process (func, argc, argv, 0);
}

/*-----------------------------------------------------------------------------
 * Create a new process */
//-----------------------------------------------------------------------------
int create_process (void (*func)(int,char*), int argc, char **argv,
        unsigned long flags)
{
    int pti;
    void *pstack;
    struct pcb *p;
    unsigned long *args;

    /* find a free PCB */
    for (pti = 0; pti < PT_SIZE && proctab[pti].state != STATE_STOPPED; pti++)
        /* nothing */;
    if (pti == PT_SIZE)
        return -EAGAIN;

    if (!(pstack = kmalloc (STACK_SIZE)))
        return -ENOMEM;

    /* set process metadata */
    p = &proctab[pti];
    p->stack_mem = pstack;
    p->timestamp = tick_count;
    p->pid += PT_SIZE;
    p->parent_pid = current->pid;
    p->flags = flags;
    p->pgdir = (unsigned long*)
        ((unsigned long) &_kernel_pgd - (unsigned long) &KERNEL_PAGE_OFFSET);

    list_init (&p->send_q);
    list_init (&p->recv_q);
    list_init (&p->repl_q);
    list_init (&p->heap_mem);
    list_init (&p->page_mem);

    sig_init (p);
    files_init (p);

    if (flags & PFLAG_SUPER) {
        struct ctxt *f = (struct ctxt*)
            ((char*) pstack + STACK_SIZE - sizeof (struct ctxt) - 128);
        put_iret_frame_super (f, (unsigned long) func);
        p->esp = f;
        args = (unsigned long*) (f + 1);
    } else {
        struct ctxt *f = (struct ctxt*) pstack + 32;
        put_iret_frame (f, (unsigned long) func,
                (unsigned long) ((char*) pstack + STACK_SIZE - 256));
        p->esp = f;
        p->ifp = f + 1;
        args = (unsigned long*) f->iret_esp;
    }

    /* pass arguments to process */
    args[0] = (unsigned long) exit;
    args[1] = (unsigned long) argc;
    args[2] = (unsigned long) argv;

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
    struct pcb *pit;
    struct mem_header *hit;
    struct pf_info *mit;

    // TODO: see what POSIX requires vis-a-vis process data in handler
    pit = &proctab[PT_INDEX (current->parent_pid)];
    __kill (pit, SIGCHLD);

    // free memory allocated to process
    kfree (current->stack_mem);
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
