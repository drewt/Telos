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

#include <syscall.h>
#include <signal.h>
#include <bit.h>

/* signals which cannot be ignored by the user */
#define SIG_NOIGNORE (BIT(SIGKILL) | BIT(SIGSTOP))

#define SIGNO_INVALID(signo) ((signo) < 1 || (signo) >= _TELOS_SIGMAX)
#define SIG_UNBLOCKABLE(signo) ((signo) == SIGKILL || (signo) == SIGSTOP)

/* trampoline code */
static void sigtramp0 (void(*handler)(int), void *osp, int signo)
{
    handler (signo);
    syscall1 (SYS_SIGRETURN, osp);
}

/* trampoline code for sigaction signal handling */
static void sigtramp1 (void(*handler)(int,siginfo_t*,void*), void *osp,
        siginfo_t info)
{
    handler (info.si_signo, &info, osp);
    syscall1 (SYS_SIGRETURN, osp);
}

static void sig_err (void)
{
    kprintf ("SIG ERROR\n");
    for (;;);
}

/*-----------------------------------------------------------------------------
 * Prepares a process to switch contexts into the signal handler for the given
 * signal */
//-----------------------------------------------------------------------------
int send_signal (struct pcb *p, int sig_no)
{
    unsigned long sig_eip;
    unsigned long *args;
    struct ctxt *old_ctxt, *sig_frame;
    struct sigaction *act = &p->sigactions[sig_no];
    struct siginfo *info = &p->siginfos[sig_no];
    bool siginfo = act->sa_flags & SA_SIGINFO;

    if (SIGNO_INVALID (sig_no))
        return -1;

    old_ctxt = p->esp;
    sig_eip = siginfo ? (ulong) sigtramp1 : (ulong) sigtramp0;

    // the frame and arguments are placed differently if the process
    // runs with supervisor privileges
    sig_frame = p->flags & PFLAG_SUPER ?
        (struct ctxt*) (((ulong*) p->esp) - (siginfo ? 12 : 5)) - 1
        : (struct ctxt*) ((char*) old_ctxt - U_CONTEXT_SIZE);
    args = p->flags & PFLAG_SUPER ? sig_frame->stack
        : ((ulong*) old_ctxt->iret_esp - (siginfo ? 12 : 5));
    p->flags & PFLAG_SUPER ? put_iret_frame_super (sig_frame, sig_eip)
        : put_iret_frame (sig_frame, sig_eip, (ulong) args);

    args[0] = (unsigned long) sig_err;
    args[1] = (unsigned long) act->sa_handler;
    args[2] = (unsigned long) old_ctxt;
    args[3] = (unsigned long) sig_no;
    if (siginfo) {
        args[4]  = info->si_errno;
        args[5]  = info->si_code;
        args[6]  = info->si_pid;
        args[7]  = (unsigned long) info->si_addr;
        args[8]  = info->si_status;
        args[9]  = info->si_band;
        args[10] = (unsigned long) info->si_value.sigval_ptr;
        args[11] = p->rc;
    } else {
        args[4]  = p->rc;
    }
    old_ctxt->reg.eax = p->sig_ignore;

    p->ifp = p->esp;
    p->esp = sig_frame;
    p->sig_pending &= ~(BIT (sig_no));
    p->sig_ignore = siginfo ? p->sig_ignore & ~(act->sa_mask)
                            : (u32) (~0 << (sig_no + 1));
    return 0;
}

/*-----------------------------------------------------------------------------
 * Restore CPU & signal context that was active before the current signal */
//-----------------------------------------------------------------------------
void sig_restore (void *osp)
{
    // TODO: verify that osp is in valid range and properly aligned
    struct ctxt *cx = osp;
    current->esp = cx;
    current->ifp = (struct ctxt*) ((ulong) cx + U_CONTEXT_SIZE);

    unsigned long *old_esp;
    old_esp = current->flags & PFLAG_SUPER
            ? (unsigned long*) osp
            : (unsigned long*) cx->iret_esp;

    // restore old signal mask and return value
    current->sig_ignore = cx->reg.eax;
    current->rc         = old_esp[-2];
}

/*-----------------------------------------------------------------------------
 * Wait for a signal */
//-----------------------------------------------------------------------------
void sys_sigwait (void)
{
    current->state = STATE_SIGWAIT;
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Registers a signal action */
//-----------------------------------------------------------------------------
void sys_sigaction (int sig, struct sigaction *act, struct sigaction *oact)
{
    if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig)) {
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
void sys_signal (int sig, void(*func)(int))
{
    if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig)) {
        current->rc = EINVAL;
        return;
    }

    current->rc = (long) current->sigactions[sig].sa_handler;

    if (func == SIG_IGN) {
       current->sig_accept &= ~(1 << sig);
    } else if (func == SIG_DFL) {
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
void sys_sigprocmask (int how, sigset_t *set, sigset_t *oset)
{
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
void __kill (struct pcb *p, int sig_no)
{
    u32 sig_bit = 1 << sig_no;

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
}

void sys_kill (pid_t pid, int sig)
{
    int i = PT_INDEX (pid);

    if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid) {
        current->rc = -ESRCH;
        return;
    }

    if (sig == 0) {
        current->rc = 0;
        return;
    }

    if (SIGNO_INVALID (sig)) {
        current->rc = -EINVAL;
        return;
    }

    proctab[i].siginfos[sig].si_code = SI_USER;

    __kill (&proctab[i], sig);
    current->rc = 0;
}
void sys_sigqueue (pid_t pid, int sig, const union sigval value)
{
    int i = PT_INDEX (pid);

    if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid) {
        current->rc = -ESRCH;
        return;
    }

    if (sig == 0) {
        current->rc = 0;
        return;
    }

    if (SIGNO_INVALID (sig)) {
        current->rc = -EINVAL;
        return;
    }

    proctab[i].siginfos[sig].si_value = value;
    proctab[i].siginfos[sig].si_code = SI_QUEUE;

    __kill (&proctab[i], sig);
    current->rc = 0;
}
