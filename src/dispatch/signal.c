/* signal.c : signal handling
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

#include <errnodefs.h>
#include <syscall.h>
#include <signal.h>
#include <bit.h>

/* signals which cannot be ignored by the user */
#define SIG_NOIGNORE (BIT(SIGKILL) | BIT(SIGSTOP))

/* trampoline code */
static void sigtramp0 (void(*handler)(int), void *osp, int signo) {
    handler (signo);
    syscall1 (SYS_SIGRETURN, osp);
}

/* trampoline code for sigaction signal handling */
static void sigtramp1 (void(*handler)(int,siginfo_t*,void*), void *osp,
        siginfo_t info) {
    handler (info.si_signo, &info, osp);
    syscall1 (SYS_SIGRETURN, osp);
}

static void sig_err (void) {
    kprintf ("SIG ERROR\n");
    for (;;);
}

/*-----------------------------------------------------------------------------
 * Prepares a process to switch contexts into the signal handler for the given
 * signal */
//-----------------------------------------------------------------------------
int send_signal (pid_t pid, int sig_no) {
    int i = PT_INDEX (pid);
    if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid)
        return -1;
    if (sig_no < 0 || sig_no > 31)
        return -2;

    struct pcb *p = &proctab[i];
    struct sigaction *act = &(p->sigactions[sig_no]);
    struct siginfo *info  = &p->siginfos[sig_no];
    bool siginfo = act->sa_flags & SA_SIGINFO;

    struct ctxt *old_ctxt = p->esp;
    struct ctxt *sig_ctxt = old_ctxt - 1;

    // update pcb
    p->ifp = p->esp;
    p->esp = sig_ctxt;
    p->sig_pending &= ~(BIT(sig_no));

    // set up signal context
    sig_ctxt->iret_cs  = SEG_UCODE | 3;
    sig_ctxt->iret_eip = (siginfo ? (uint32_t) sigtramp1 :
                                    (uint32_t) sigtramp0);
    sig_ctxt->eflags   = EFLAGS_IOPL(0) | EFLAGS_IF;
    sig_ctxt->iret_esp = (uint32_t) 
        ((uint32_t*) old_ctxt->iret_esp - (siginfo ? 13 : 6));
    sig_ctxt->iret_ss  = SEG_UDATA | 3;

    // set up sigtramp frame
    uint32_t *args = (uint32_t*) sig_ctxt->iret_esp;
    args[0] = (uint32_t) sig_err;
    args[1] = siginfo ? (uint32_t) act->sa_sigaction :
                        (uint32_t) act->sa_handler;
    args[2] = (uint32_t) old_ctxt;
    args[3] = (uint32_t) sig_no;
    if (siginfo) {
        args[4]  = info->si_errno;
        args[5]  = info->si_code;;
        args[6]  = info->si_pid;
        args[7]  = (uint32_t) info->si_addr;
        args[8]  = info->si_status;
        args[9]  = info->si_band;
        args[10] = (uint32_t) info->si_value.sigval_ptr;
        args[11] = p->rc;
    } else {
        args[4]  = p->rc;
    }
    old_ctxt->reg.eax = p->sig_ignore;
    
    p->sig_ignore = siginfo ? p->sig_ignore & ~(act->sa_mask) :
                              (uint32_t) (~0 << (sig_no + 1));

    return 0;
}

/*-----------------------------------------------------------------------------
 * Restore CPU & signal context that was active before the current signal */
//-----------------------------------------------------------------------------
void sig_restore (void *osp) {

    // TODO: verify that osp is in valid range and properly aligned
    struct ctxt *cx = osp;
    current->esp = cx;
    current->ifp = cx + 1;

    // restore old signal mask and return value
    current->sig_ignore = cx->reg.eax;
    current->rc         = ((uint32_t*) cx->iret_esp)[-2];
}

/*-----------------------------------------------------------------------------
 * Wait for a signal */
//-----------------------------------------------------------------------------
void sys_sigwait (void) {
    current->state = STATE_SIGWAIT;
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Registers a signal action */
//-----------------------------------------------------------------------------
void sys_sigaction (int sig, struct sigaction *act, struct sigaction *oact) {

    if (sig < 0 || sig > 31) {
        current->rc = EINVAL;
        return;
    }

    if (oact)
        *oact = current->sigactions[sig];

    if (act) {
        current->sigactions[sig] = *act;
        current->sig_accept |= BIT(sig);
    }

    current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * Registers a signal handling function for a given signal */
//-----------------------------------------------------------------------------
void sys_signal (int sig, void(*func)(int)) {

    if (sig < 0 || sig > 31) {
        current->rc = EINVAL;
        return;
    }

    current->rc = (int) current->sigactions[sig].sa_handler;

    if ((int) func == SIG_IGN) {
       current->sig_accept &= ~(1 << sig);
    } else if ((int) func == SIG_DFL) {
        // ???
    } else {
        current->sigactions[sig].sa_handler = func;
        current->sigactions[sig].sa_mask    = 0;
        current->sigactions[sig].sa_flags   = 0;
        current->sig_accept |= BIT(sig);
    }
}

/*-----------------------------------------------------------------------------
 * Alters the signal mask */
//-----------------------------------------------------------------------------
void sys_sigprocmask (int how, uint32_t *set, uint32_t *oset) {
    if (oset)
        *oset = ~(current->sig_ignore);
    if (set) {
        switch (how) {
        case SIG_BLOCK:
            current->sig_ignore &= ~(*set);
            current->sig_ignore |= SIG_NOIGNORE;
            break;
        case SIG_SETMASK:
            current->sig_ignore = ~(*set) | SIG_NOIGNORE;
            break;
        case SIG_UNBLOCK:
            current->sig_ignore |= *set;
            break;
        default:
            current->rc = EINVAL;
            return;
        }
    }
    current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * Function to register that a signal has been sent to a process */
//-----------------------------------------------------------------------------
void sys_kill (int pid, int sig_no) {

    uint32_t sig_bit = 1 << sig_no;
    int pti = PT_INDEX (pid);

    if (pti < 0 || pti >= PT_SIZE || proctab[pti].pid != pid) {
        current->rc = ESRCH;
        return;
    }
    if (sig_no < 0 || !sig_bit) {
        current->rc = EINVAL;
        return;
    }

    struct pcb *p = &proctab[pti];

    // record signal if process accepts it
    if (p->sig_accept & sig_bit) {
        p->sig_pending |= sig_bit;

        if (p->sigactions[sig_no].sa_flags & SA_SIGINFO) {
            p->siginfos[sig_no].si_errno = 0;
            p->siginfos[sig_no].si_pid   = current->pid;
            p->siginfos[sig_no].si_addr  =
                (void*) ((struct ctxt*) p->esp)->iret_eip;
        }
        // ready process if it's blocked
        if (p->state == STATE_SIGWAIT) {
            p->rc = sig_no;
            ready (p);
        } else if (p->state == STATE_BLOCKED) {
            p->rc = -128;
            ready (p);
        } else if (p->state == STATE_SLEEPING) {
            p->rc = sq_rm (p) * 10;
            ready (p);
        }
    }
    current->rc = 0;
}
