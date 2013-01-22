/* dispatch.c : dispatcher Mark II
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
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/interrupt.h>
#include <syscall.h>

struct pcb *current;    /* the running process     */
struct pcb *ready_head; /* head of the ready queue */
struct pcb *ready_tail; /* tail of the ready queue */

struct sysaction {
    void(*func)(); // service routine
    int nargs;     // number of args
};

static struct sysaction sysactions[100] = {
    /* hardware interrupts */
    [TIMER_INTR] = { (void(*)()) tick, 0 },
    /* system calls */
    [SYS_CREATE]    = { (void(*)()) sys_create,      2 },
    [SYS_YIELD]     = { (void(*)()) sys_yield,       0 },
    [SYS_STOP]      = { (void(*)()) sys_stop,        0 },
    [SYS_GETPID]    = { (void(*)()) sys_getpid,      0 },
    [SYS_SLEEP]     = { (void(*)()) sys_sleep,       1 },
    [SYS_SIGRETURN] = { (void(*)()) sig_restore,     1 },
    [SYS_KILL]      = { (void(*)()) sys_kill,        2 },
    [SYS_SIGWAIT]   = { (void(*)()) sys_sigwait,     0 },
    [SYS_SIGACTION] = { (void(*)()) sys_sigaction,   4 },
    [SYS_SIGNAL]    = { (void(*)()) sys_signal,      2 },
    [SYS_SIGMASK]   = { (void(*)()) sys_sigprocmask, 3 },
    [SYS_OPEN]      = { (void(*)()) sys_open,        1 },
    [SYS_CLOSE]     = { (void(*)()) sys_close,       1 },
    [SYS_READ]      = { (void(*)()) sys_read,        3 },
    [SYS_WRITE]     = { (void(*)()) sys_write,       3 }
};

/*-----------------------------------------------------------------------------
 * Initializes the dispatcher */
//-----------------------------------------------------------------------------
void dispatch_init (void) {
    // initialize sysactions that can't be initialized statically
    sysactions[KBD_INTR].func  = (void(*)()) devtab[DEV_KBD].dviint;
    sysactions[KBD_INTR].nargs = 0;

    // TODO: remove, use console write instead
    sysactions[SYS_PUTS].func       = (void(*)()) sys_puts;
    sysactions[SYS_PUTS].nargs      = 1;
    sysactions[SYS_REPORT].func     = (void(*)()) sys_report;
    sysactions[SYS_REPORT].nargs    = 1;
}

/*-----------------------------------------------------------------------------
 *  */
//*-----------------------------------------------------------------------------
void dispatch (void) {
    
    unsigned int req;
    struct sys_args *args;

    current = next ();
    for (;;) {

        if (current->sig_pending & current->sig_ignore) {
            unsigned int sig_no = 31;
            while (sig_no && !(current->sig_pending >> sig_no))
                sig_no--;
            send_signal (current->pid, sig_no);
        }

        req  = context_switch (current);
        args = current->esp;

        if (req < 100 && sysactions[req].func != NULL) {
            switch (sysactions[req].nargs) {
            case 0:
                sysactions[req].func ();
                break;
            case 1:
                sysactions[req].func (args->arg0);
                break;
            case 2:
                sysactions[req].func (args->arg0, args->arg1);
                break;
            case 3:
                sysactions[req].func (args->arg0, args->arg1, args->arg2);
                break;
            case 4:
                sysactions[req].func (args->arg0, args->arg1, args->arg2,
                        args->arg3);
                break;
            case 5:
                sysactions[req].func (args->arg0, args->arg1, args->arg2,
                        args->arg3, args->arg4);
                break;
            }
        } else {
            current->rc = SYSERR;
        }
    }
}

/*-----------------------------------------------------------------------------
 * Dequeues and returns a process from the ready queue */
//-----------------------------------------------------------------------------
struct pcb *next (void) {
    if (!ready_head) return NULL;
    struct pcb *next = ready_head;
    ready_head = ready_head->next;
    return next;
}

/*-----------------------------------------------------------------------------
 * Enqueues a process on the ready queue */
//-----------------------------------------------------------------------------
void ready (struct pcb *p) {
    p->state = STATE_READY;
    p->next  = NULL;

    if (!ready_head) {
        ready_head = p;
        ready_tail = p;
    } else {
        ready_tail->next = p;
        ready_tail = p;
    }
}

/*-----------------------------------------------------------------------------
 * Selects a new process to run, choosing the idle process only if there are no
 * other processes ready to run */
//-----------------------------------------------------------------------------
void new_process (void) {
    current = next ();

    // skip idle process if possible
    if (current->pid == idle_pid && ready_head) {
        ready (current);
        current = next ();
    }
}

void print_ready_queue (void) {
    struct pcb *it;
    for (it = ready_head; it->next; it = it->next) {
        kprintf ("%d->", it->pid);
    }
    kprintf ("%d\n", it->pid);
}
