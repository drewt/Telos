/* process.c : process management
 */

/*  Copyright 2013 Drew T.
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

#include <telos/devices.h>

#include <errnodefs.h>
#include <signal.h>
#include <unistd.h>

extern void sysstop (void);

/*-----------------------------------------------------------------------------
 * Create a new process */
//-----------------------------------------------------------------------------
int sys_create (void (*func)(int,char*), int argc, char **argv) {
    int i;
    void *pstack;
    struct pcb *p;

    // find a free PCB
    for (i = 0; i < PT_SIZE && proctab[i].state != STATE_STOPPED; i++);
    if (i == PT_SIZE) {
        current->rc = EAGAIN;
        return EAGAIN;
    }
    if (!(pstack = kmalloc (STACK_SIZE))) {
        current->rc = ENOMEM;
        return ENOMEM;
    }

    p = &proctab[i];

    p->stack_mem = pstack;

    p->timestamp  = tick_count;

    p->pid += PT_SIZE;
    p->parent_pid = current->pid;

    proc_initq (&p->send_q);
    proc_initq (&p->recv_q);
    proc_initq (&p->repl_q);

    p->sig_pending = 0;
    p->sig_accept  = 0;
    p->sig_ignore  = ~0;

    // initialize file descriptor table
    p->fds[STDIN_FILENO]  = DEV_KBD_ECHO;
    p->fds[STDOUT_FILENO] = DEV_CONSOLE_0;
    p->fds[STDERR_FILENO] = DEV_CONSOLE_0;
    for (int j = 3; j < FDT_SIZE; j++)
        p->fds[j] = FD_NONE;

    // set up the context stack
    struct ctxt *f = (struct ctxt*) pstack + 32;
    f->iret_cs  = SEG_UCODE | 3;
    f->iret_eip = (uint32_t) func;
    f->eflags   = EFLAGS_IOPL(0) | EFLAGS_IF;
    f->iret_esp = (uint32_t) ((char*) pstack + STACK_SIZE - 256);
    f->iret_ss  = SEG_UDATA | 3;
    p->esp = f;
    p->ifp = f + 1;

    // pass arguments to process
    uint32_t *args = (uint32_t*) f->iret_esp;
    args[0] = (uint32_t) sysstop; // return address
    args[1] = (uint32_t) argc;
    args[2] = (uint32_t) argv;

    ready (p);

    current->rc = p->pid;
    return p->pid;
}

/*-----------------------------------------------------------------------------
 * Yeild control to another process */
//-----------------------------------------------------------------------------
void sys_yield (void) {
    ready (current);
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Stop the current process and free all resources associated with it */
//-----------------------------------------------------------------------------
void sys_stop (void) {
    struct pcb *tmp;

    // send SIGCHLD to parent
    sys_kill (current->parent_pid, SIGCHLD);

    kfree (current->stack_mem);
    current->state = STATE_STOPPED;
    current->next = NULL;

    for (tmp = proc_dequeue (&current->send_q); tmp;
            tmp = proc_dequeue (&current->send_q)) {
        tmp->rc = SYSERR;
        ready (tmp);
    }
    for (tmp = proc_dequeue (&current->recv_q); tmp;
            tmp = proc_dequeue (&current->recv_q)) {
        tmp->rc = SYSERR;
        ready (tmp);
    }
    for (tmp = proc_dequeue (&current->repl_q); tmp;
            tmp = proc_dequeue (&current->repl_q)) {
        tmp->rc = SYSERR;
        ready (tmp);
    }

    new_process ();
}

/*-----------------------------------------------------------------------------
 * Return the current process's pid */
//-----------------------------------------------------------------------------
void sys_getpid (void) {
    current->rc = current->pid;
}
