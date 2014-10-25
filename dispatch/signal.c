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

#include <kernel/i386.h>
#include <kernel/bitops.h>
#include <kernel/dispatch.h>
#include <kernel/timer.h>
#include <kernel/signal.h>

#include <syscall.h>

/* signals which cannot be ignored by the user */
#define SIG_NOIGNORE (BIT(SIGKILL) | BIT(SIGSTOP))

#define SIGNO_INVALID(signo) ((signo) < 1 || (signo) >= _TELOS_SIGMAX)
#define SIG_UNBLOCKABLE(signo) ((signo) == SIGKILL || (signo) == SIGSTOP)

/* trampoline for signal handling */
static void sigtramp1(void(*handler)(int,siginfo_t*,void*), void *osp,
		siginfo_t info)
{
	handler(info.si_signo, &info, osp);
	syscall1(SYS_SIGRETURN, osp);
}

static void sig_err(void)
{
	kprintf("SIG ERROR\n");
	for (;;);
}

static void place_sig_frame(struct ucontext *sig_cx, int sig_no)
{
	/* place trampoline context */
	put_iret_uframe(sig_cx, (ulong)sigtramp1, (ulong)sig_cx->stack);

	/* place trampoline args */
	sig_cx->stack[0] = (ulong) sig_err;
	sig_cx->stack[1] = (ulong) current->sigactions[sig_no].sa_handler;
	sig_cx->stack[2] = (ulong) current->esp;
	sig_cx->stack[3] = (ulong) sig_no;
	if (current->sigactions[sig_no].sa_flags & SA_SIGINFO) {
		struct siginfo *info = &current->siginfos[sig_no];
		sig_cx->stack[4]  = info->si_errno;
		sig_cx->stack[5]  = info->si_code;
		sig_cx->stack[6]  = info->si_pid;
		sig_cx->stack[7]  = (ulong) info->si_addr;
		sig_cx->stack[8]  = info->si_status;
		sig_cx->stack[9]  = info->si_band;
		sig_cx->stack[10] = (ulong) info->si_value.sigval_ptr;
	}
	sig_cx->stack[11] = current->rc;
}

static inline struct ucontext *frame_pos(ulong old_esp)
{
	return (struct ucontext*) ((ulong*)old_esp - 20);
}

static inline u32 next_mask(struct sigaction *act, u32 old_mask, int sig_no)
{
	if (act->sa_flags & SA_SIGINFO)
		return old_mask & ~(act->sa_mask);
	return (u32) (~0 << (sig_no + 1));
}

static int send_signal_user(int sig_no)
{
	struct ucontext *old_cx = current->esp;
	struct sigaction *act = &current->sigactions[sig_no];
	struct ucontext *sig_frame = frame_pos(old_cx->iret_esp);

	place_sig_frame(sig_frame, sig_no);

	old_cx->reg.eax = current->sig_ignore;
	current->ifp = current->esp;
	current->esp = sig_frame;
	current->sig_ignore = next_mask(act, current->sig_ignore, sig_no);
	clear_bit(sig_no, &current->sig_pending);
	return 0;
}

/*-----------------------------------------------------------------------------
 * Prepares a process to switch contexts into the signal handler for the given
 * signal */
//-----------------------------------------------------------------------------
static int send_signal(struct pcb *p, int sig_no)
{
	if (SIGNO_INVALID(sig_no))
		return -EINVAL;
	if (p->flags & PFLAG_SUPER)
		return -ENOTSUP;
	return send_signal_user(sig_no);
}

void handle_signal(void)
{
	if (current->sig_pending & current->sig_ignore)
		send_signal(current, fls(current->sig_pending)-1);
}

/*-----------------------------------------------------------------------------
 * Restore CPU & signal context that was active before the current signal */
//-----------------------------------------------------------------------------
long sig_restore(struct ucontext *old_cx)
{
	// TODO: verify that cx is in valid range and properly aligned
	current->esp = old_cx;
	current->ifp = old_cx + 1;

	// restore old signal mask and return value
	current->sig_ignore = old_cx->reg.eax;
	return frame_pos(old_cx->iret_esp)->stack[11];
}

/*-----------------------------------------------------------------------------
 * Wait for a signal */
//-----------------------------------------------------------------------------
long sys_sigwait(void)
{
	current->state = STATE_SIGWAIT;
	new_process();
	return 0;
}

/*-----------------------------------------------------------------------------
 * Registers a signal action */
//-----------------------------------------------------------------------------
long sys_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	struct sigaction *sap = &current->sigactions[sig];

	if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig))
		return -EINVAL;

	if (oact)
		copy_to_current(oact, sap, sizeof(*sap));

	if (act) {
		copy_from_current(sap, act, sizeof(*sap));
		set_bit(sig, &current->sig_accept);
	}

	return 0;
}

/*-----------------------------------------------------------------------------
 * Registers a signal handling function for a given signal */
//-----------------------------------------------------------------------------
long sys_signal(int sig, void(*func)(int))
{
	long rv = (long) current->sigactions[sig].sa_handler;;

	if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig))
		return -EINVAL;

	if (func == SIG_IGN) {
		clear_bit(sig, &current->sig_accept);
	} else if (func == SIG_DFL) {
		// ???
	} else {
		current->sigactions[sig].sa_handler = func;
		current->sigactions[sig].sa_mask    = 0;
		current->sigactions[sig].sa_flags   = 0;
		set_bit(sig, &current->sig_accept);
	}
	return rv;
}

/*-----------------------------------------------------------------------------
 * Alters the signal mask */
//-----------------------------------------------------------------------------
long sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	if (oset) {
		u32 tmp = ~(current->sig_ignore);
		copy_to_current(oset, &tmp, sizeof(u32));
	}

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
			return -EINVAL;
		}
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * Function to register that a signal has been sent to a process */
//-----------------------------------------------------------------------------
void __kill(struct pcb *p, int sig_no)
{
	/* record signal if process accepts it */
	if (!(p->sig_accept & BIT(sig_no)))
		return;

	set_bit(sig_no, &p->sig_pending);

	if (p->sigactions[sig_no].sa_flags & SA_SIGINFO) {
		struct ucontext cx;

		copy_from_user(p, &cx, (void*) p->esp, sizeof(cx));

		p->siginfos[sig_no].si_errno = 0;
		p->siginfos[sig_no].si_pid   = current->pid;
		p->siginfos[sig_no].si_addr = (void*) cx.iret_eip;
	}

	/* ready process if it's blocked */
	if (p->state == STATE_SIGWAIT) {
		p->rc = sig_no;
		ready(p);
	} else if (p->state == STATE_BLOCKED) {
		p->rc = -128;
		ready(p);
	} else if (p->state == STATE_SLEEPING) {
		/* sleep timer has TF_ALWAYS */
		p->rc = ktimer_destroy(&p->t_sleep);
	}
}

long sys_kill(pid_t pid, int sig)
{
	int i = PT_INDEX(pid);

	if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid)
		return -ESRCH;

	if (sig == 0)
		return 0;

	if (SIGNO_INVALID(sig))
		return -EINVAL;

	proctab[i].siginfos[sig].si_code = SI_USER;

	__kill(&proctab[i], sig);
	return 0;
}

long sys_sigqueue(pid_t pid, int sig, const union sigval value)
{
	int i = PT_INDEX(pid);

	if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid)
		return -ESRCH;

	if (sig == 0)
		return 0;

	if (SIGNO_INVALID(sig))
		return -EINVAL;

	proctab[i].siginfos[sig].si_value = value;
	proctab[i].siginfos[sig].si_code = SI_QUEUE;

	__kill(&proctab[i], sig);
	return 0;
}
