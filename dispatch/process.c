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

/*-----------------------------------------------------------------------------
 * Create a new process */
//-----------------------------------------------------------------------------
int sys_create (void (*func)(int,char*), int argc, char **argv)
{
    int pti;
    void *pstack;
    struct pcb *p;

    /* find a free PCB */
    for (pti = 0; pti < PT_SIZE && proctab[pti].state != STATE_STOPPED; pti++)
        /* nothing */;
    if (pti == PT_SIZE) {
        current->rc = -EAGAIN;
        return -EAGAIN;
    }

    if (!(pstack = kmalloc (STACK_SIZE))) {
        current->rc = -ENOMEM;
        return -ENOMEM;
    }

    /* set process metadata */
    p = &proctab[pti];
    p->stack_mem = pstack;
    p->timestamp = tick_count;
    p->pid += PT_SIZE;
    p->parent_pid = current->pid;
    p->pgdir = &_kernel_pgd;

    list_init (&p->send_q);
    list_init (&p->recv_q);
    list_init (&p->repl_q);
    list_init (&p->heap_mem);
    list_init (&p->page_mem);

    sig_init (p);
    files_init (p);

    struct ctxt *f = (struct ctxt*) pstack + 32;
    put_iret_frame (f, (unsigned long) func,
            (unsigned long) ((char *) pstack + STACK_SIZE - 256));
    p->esp = f;
    p->ifp = f + 1;

    /* pass arguments to process */
    unsigned long *args = (unsigned long*) f->iret_esp;
    args[0] = (unsigned long) exit;
    args[1] = (unsigned long) argc;
    args[2] = (unsigned long) argv;

    ready (p);

    current->rc = p->pid;
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
    sys_kill (current->parent_pid, SIGCHLD);

    // free memory allocated to process
    kfree (current->stack_mem);
    dequeue_iterate (&current->page_mem, mit, struct pf_info*)
        kfree_page (mit);
    dequeue_iterate (&current->heap_mem, hit, struct mem_header*)
        kfree (hit->data_start);

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
