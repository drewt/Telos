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
#include <syscall.h>

struct pcb *current;    /* the running process     */
struct pcb *ready_head; /* head of the ready queue */
struct pcb *ready_tail; /* tail of the ready queue */

struct sysaction {
    void(*func)(); // service routine
    int nargs;     // number of args
};

static struct sysaction sysactions[100]; /* ISR table */

/*-----------------------------------------------------------------------------
 * Initializes the dispatcher */
//-----------------------------------------------------------------------------
void dispatch_init (void) {
    // hardware interrupts
    sysactions[32].func  = (void(*)()) tick;
    sysactions[32].nargs = 0;
    sysactions[33].func  = (void(*)()) devtab[DEV_KBD].dviint;
    sysactions[33].nargs = 0;

    // system calls
    sysactions[SYS_CREATE].func     = (void(*)()) sys_create;
    sysactions[SYS_CREATE].nargs    = 2;
    sysactions[SYS_YIELD].func      = (void(*)()) sys_yield;
    sysactions[SYS_YIELD].nargs     = 0;
    sysactions[SYS_STOP].func       = (void(*)()) sys_stop;
    sysactions[SYS_STOP].nargs      = 0;
    sysactions[SYS_GETPID].func     = (void(*)()) sys_getpid;
    sysactions[SYS_GETPID].nargs    = 0;
    sysactions[SYS_PUTS].func       = (void(*)()) sys_puts;
    sysactions[SYS_PUTS].nargs      = 1;
    sysactions[SYS_REPORT].func     = (void(*)()) sys_report;
    sysactions[SYS_REPORT].nargs    = 1;
    sysactions[SYS_SLEEP].func      = (void(*)()) sys_sleep;
    sysactions[SYS_SLEEP].nargs     = 1;
    sysactions[SYS_SIGRETURN].func  = (void(*)()) sig_restore;
    sysactions[SYS_SIGRETURN].nargs = 1;
    sysactions[SYS_KILL].func       = (void(*)()) sys_kill;
    sysactions[SYS_KILL].nargs      = 2;
    sysactions[SYS_SIGWAIT].func    = (void(*)()) sys_sigwait;
    sysactions[SYS_SIGWAIT].nargs   = 0;
    sysactions[SYS_SIGACTION].func  = (void(*)()) sys_sigaction;
    sysactions[SYS_SIGACTION].nargs = 3;
    sysactions[SYS_SIGNAL].func     = (void(*)()) sys_signal;
    sysactions[SYS_SIGNAL].nargs    = 2;
    sysactions[SYS_SIGMASK].func    = (void(*)()) sys_sigprocmask;
    sysactions[SYS_SIGMASK].nargs   = 3;
    sysactions[SYS_OPEN].func       = (void(*)()) sys_open;
    sysactions[SYS_OPEN].nargs      = 1;
    sysactions[SYS_CLOSE].func      = (void(*)()) sys_close;
    sysactions[SYS_CLOSE].nargs     = 1;
    sysactions[SYS_READ].func       = (void(*)()) sys_read;
    sysactions[SYS_READ].nargs      = 3;
    sysactions[SYS_WRITE].func      = (void(*)()) sys_write;
    sysactions[SYS_WRITE].nargs     = 3;
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
