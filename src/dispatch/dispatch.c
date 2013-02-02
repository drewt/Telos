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

/* routines defined in other files */
extern unsigned int context_switch (struct pcb *p);
extern int send_signal (int pid, int sig_no);
extern void tick (void);

struct pcb *current    = NULL; /* the running process     */
struct pcb *ready_head = NULL; /* head of the ready queue */
struct pcb *ready_tail = NULL; /* tail of the ready queue */

struct sysaction {
    void(*func)(); // service routine
    int nargs;     // number of args
};

/* table of actions to be taken for interrupts/system calls */
static struct sysaction sysactions[SYSCALL_MAX] = {
//    - INDEX -             - ACTION -            - ARGS -
    [TIMER_INTR]    = { (void(*)()) tick,            0 },
    [SYS_CREATE]    = { (void(*)()) sys_create,      3 },
    [SYS_YIELD]     = { (void(*)()) sys_yield,       0 },
    [SYS_STOP]      = { (void(*)()) sys_stop,        0 },
    [SYS_GETPID]    = { (void(*)()) sys_getpid,      0 },
    [SYS_REPORT]    = { (void(*)()) sys_report,      1 },
    [SYS_SLEEP]     = { (void(*)()) sys_sleep,       1 },
    [SYS_SIGRETURN] = { (void(*)()) sig_restore,     1 },
    [SYS_KILL]      = { (void(*)()) sys_kill,        2 },
    [SYS_SIGWAIT]   = { (void(*)()) sys_sigwait,     0 },
    [SYS_SIGACTION] = { (void(*)()) sys_sigaction,   4 },
    [SYS_SIGNAL]    = { (void(*)()) sys_signal,      2 },
    [SYS_SIGMASK]   = { (void(*)()) sys_sigprocmask, 3 },
    [SYS_OPEN]      = { (void(*)()) sys_open,        3 },
    [SYS_CLOSE]     = { (void(*)()) sys_close,       1 },
    [SYS_READ]      = { (void(*)()) sys_read,        3 },
    [SYS_WRITE]     = { (void(*)()) sys_write,       3 },
    [SYS_IOCTL]     = { (void(*)()) sys_ioctl,       3 },
    [SYS_ALARM]     = { (void(*)()) sys_alarm,       1 },
    [SYS_SEND]      = { (void(*)()) sys_send,        5 },
    [SYS_RECV]      = { (void(*)()) sys_recv,        3 },
    [SYS_REPLY]     = { (void(*)()) sys_reply,       3 },
    [SYS_MALLOC]    = { (void(*)()) sys_malloc,      2 },
    [SYS_FREE]      = { (void(*)()) sys_free,        1 }
};

static inline void set_action (unsigned int vector, void(*f)(), int nargs) {
    sysactions[vector] = (struct sysaction) { f, nargs };
}

/*-----------------------------------------------------------------------------
 * Initializes the dispatcher.  Must be called *after* the device table is
 * initialized, if any device ISRs are assigned dynamically */
//-----------------------------------------------------------------------------
void dispatch_init (void) {
    // initialize actions that can't be initialized statically
    set_action (KBD_INTR, (void(*)()) devtab[DEV_KBD].dviint, 0);
}

/*-----------------------------------------------------------------------------
 * The dispatcher.  Passes control to the appropriate routines to handle 
 * interrupts, system calls, and other events */
//*----------------------------------------------------------------------------
void dispatch (void) {
    
    unsigned int req, sig_no;
    struct sys_args *args;

    current = next ();
    for (;;) {

        if (current->sig_pending & current->sig_ignore) {
            sig_no = 31;
            while (sig_no && !(current->sig_pending >> sig_no))
                sig_no--;
            send_signal (current->pid, sig_no);
        }

        req  = context_switch (current);
        args = current->esp;

        if (req < SYSCALL_MAX && sysactions[req].func != NULL) {
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
