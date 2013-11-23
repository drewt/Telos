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
static void sigtramp0(void(*handler)(int), void *osp, int signo)
{
	handler(signo);
	syscall1(SYS_SIGRETURN, osp);
}

/* trampoline code for sigaction signal handling */
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

static void place_sig_args(void* dst, struct pcb *p, void *osp, int sig_no)
{
	struct siginfo *info = &p->siginfos[sig_no];
	ulong *args = (ulong*) kmap_tmp_range(p->pgdir, (ulong) dst,
			12 * sizeof(ulong));

	args[0] = (ulong) sig_err;
	args[1] = (ulong) p->sigactions[sig_no].sa_handler;
	args[2] = (ulong) osp;
	args[3] = (ulong) sig_no;
	if (p->sigactions[sig_no].sa_flags & SA_SIGINFO) {
		args[4]  = info->si_errno;
		args[5]  = info->si_code;
		args[6]  = info->si_pid;
		args[7]  = (ulong) info->si_addr;
		args[8]  = info->si_status;
		args[9]  = info->si_band;
		args[10] = (ulong) info->si_value.sigval_ptr;
		args[11] = p->rc;
	} else {
		args[4]  = p->rc;
	}

	kunmap_range((ulong) args, 12 * sizeof(ulong));
}

static int send_signal_user(struct pcb *p, int sig_no)
{
	ulong *sig_esp;
	struct ctxt *old_ctxt, *sig_frame;
	struct sigaction *act = &p->sigactions[sig_no];
	bool siginfo = act->sa_flags & SA_SIGINFO;

	ulong u_oldctxt = (ulong) p->esp;
	ulong u_sigframe = u_oldctxt - U_CONTEXT_SIZE;
	ulong tramp_fn = siginfo ? (ulong) sigtramp1 : (ulong) sigtramp0;

	sig_frame = (struct ctxt*) kmap_tmp_range(p->pgdir, u_sigframe,
			U_CONTEXT_SIZE * 2);
	old_ctxt = (struct ctxt*) ((ulong) sig_frame + U_CONTEXT_SIZE);

	sig_esp = ((ulong*) old_ctxt->iret_esp - (siginfo ? 12 : 5));
	put_iret_frame(sig_frame, tramp_fn, (ulong) sig_esp);
	place_sig_args(sig_esp, p, (void*) u_oldctxt, sig_no);

	old_ctxt->reg.eax = p->sig_ignore;

	p->ifp = p->esp;
	p->esp = (void*) u_sigframe;
	p->sig_pending &= ~(BIT(sig_no));
	p->sig_ignore = siginfo ? p->sig_ignore & ~(act->sa_mask)
			: (u32) (~0 << (sig_no + 1));

	kunmap_range((ulong) sig_frame, U_CONTEXT_SIZE * 2);
	return 0;
}

static int send_signal_super(struct pcb *p, int sig_no)
{
	struct ctxt *old_ctxt, *sig_frame;
	struct sigaction *act = &p->sigactions[sig_no];

	bool siginfo = act->sa_flags & SA_SIGINFO;
	ulong tramp_fn = siginfo ? (ulong) sigtramp1 : (ulong) sigtramp0;

	old_ctxt = p->esp;
	sig_frame = (struct ctxt*) (((ulong*) p->esp) - (siginfo ? 12 : 5)) - 1;
	put_iret_frame_super(sig_frame, tramp_fn);
	place_sig_args(sig_frame->stack, p, old_ctxt, sig_no);

	old_ctxt->reg.eax = p->sig_ignore;

	p->esp = sig_frame;
	p->sig_pending &= ~(BIT(sig_no));
	p->sig_ignore = siginfo ? p->sig_ignore & ~(act->sa_mask)
			: (u32) (~0 << (sig_no + 1));
	return 0;
}

/*-----------------------------------------------------------------------------
 * Prepares a process to switch contexts into the signal handler for the given
 * signal */
//-----------------------------------------------------------------------------
int send_signal(struct pcb *p, int sig_no)
{
	if (SIGNO_INVALID(sig_no))
		return -1;

	return (p->flags & PFLAG_SUPER) ?
		send_signal_super(p, sig_no)
		: send_signal_user(p, sig_no);
}

/*-----------------------------------------------------------------------------
 * Restore CPU & signal context that was active before the current signal */
//-----------------------------------------------------------------------------
void sig_restore(void *osp)
{
	// TODO: verify that osp is in valid range and properly aligned
	//struct ctxt *cx = osp;
	current->esp = osp;
	current->ifp = (void*) ((ulong) osp + U_CONTEXT_SIZE);

	struct ctxt *cx = (void*) kmap_tmp_range(current->pgdir, (ulong) osp,
			U_CONTEXT_SIZE);

	ulong old_esp = current->flags & PFLAG_SUPER
			? (ulong) osp
			: (ulong) cx->iret_esp;

	ulong *rc = (void*) kmap_tmp_range(current->pgdir, old_esp,
			sizeof(ulong));

	// restore old signal mask and return value
	current->sig_ignore = cx->reg.eax;
	current->rc         = rc[-2];

	kunmap_range((ulong) cx, U_CONTEXT_SIZE);
	kunmap_range((ulong) rc, sizeof(ulong));
}

/*-----------------------------------------------------------------------------
 * Wait for a signal */
//-----------------------------------------------------------------------------
void sys_sigwait(void)
{
	current->state = STATE_SIGWAIT;
	new_process();
}

/*-----------------------------------------------------------------------------
 * Registers a signal action */
//-----------------------------------------------------------------------------
void sys_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig)) {
		current->rc = -EINVAL;
		return;
	}

	if (oact)
		copy_to_userspace(current->pgdir, oact, &current->sigactions[sig],
				sizeof(struct sigaction));

	if (act) {
		copy_from_userspace(current->pgdir, &current->sigactions[sig], act,
				sizeof(struct sigaction));
		current->sig_accept |= BIT(sig);
	}

	current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * Registers a signal handling function for a given signal */
//-----------------------------------------------------------------------------
void sys_signal(int sig, void(*func)(int))
{
	if (SIGNO_INVALID(sig) || SIG_UNBLOCKABLE(sig)) {
		current->rc = -EINVAL;
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
void sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	if (oset) {
		u32 tmp = ~(current->sig_ignore);
		copy_to_userspace(current->pgdir, oset, &tmp, sizeof(u32));
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
			current->rc = -EINVAL;
			return;
		}
	}
	current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * Function to register that a signal has been sent to a process */
//-----------------------------------------------------------------------------
void __kill(struct pcb *p, int sig_no)
{
	u32 sig_bit = 1 << sig_no;

	/* record signal if process accepts it */
	if (p->sig_accept & sig_bit) {
		p->sig_pending |= sig_bit;

		if (p->sigactions[sig_no].sa_flags & SA_SIGINFO) {
			p->siginfos[sig_no].si_errno = 0;
			p->siginfos[sig_no].si_pid   = current->pid;
			p->siginfos[sig_no].si_addr  =
				(void*) ((struct ctxt*) p->esp)->iret_eip;
		}
		/* ready process if it's blocked */
		if (p->state == STATE_SIGWAIT) {
			p->rc = sig_no;
			ready(p);
		} else if (p->state == STATE_BLOCKED) {
			p->rc = -128;
			ready(p);
		} else if (p->state == STATE_SLEEPING) {
			p->rc = sq_rm(p) * 10;
			ready(p);
		}
	}
}

void sys_kill(pid_t pid, int sig)
{
	int i = PT_INDEX(pid);

	if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid) {
		current->rc = -ESRCH;
		return;
	}

	if (sig == 0) {
		current->rc = 0;
		return;
	}

	if (SIGNO_INVALID(sig)) {
		current->rc = -EINVAL;
		return;
	}

	proctab[i].siginfos[sig].si_code = SI_USER;

	__kill(&proctab[i], sig);
	current->rc = 0;
}

void sys_sigqueue(pid_t pid, int sig, const union sigval value)
{
	int i = PT_INDEX(pid);

	if (i < 0 || i >= PT_SIZE || proctab[i].pid != pid) {
		current->rc = -ESRCH;
		return;
	}

	if (sig == 0) {
		current->rc = 0;
		return;
	}

	if (SIGNO_INVALID(sig)) {
		current->rc = -EINVAL;
		return;
	}

	proctab[i].siginfos[sig].si_value = value;
	proctab[i].siginfos[sig].si_code = SI_QUEUE;

	__kill(&proctab[i], sig);
	current->rc = 0;
}
