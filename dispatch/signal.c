/*  Copyright 2013-2015 Drew Thoreson
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
#include <telos/syscall.h>

/* signals which cannot be ignored by the user */
#define SIG_NOIGNORE (BIT(SIGKILL) | BIT(SIGSTOP))

static inline int sig_unblockable(int signo)
{
	return signo == SIGKILL || signo == SIGSTOP;
}

/* Handler for SIG_DFL Terminate action */
static void sig_terminate(int sig_no)
{
	syscall1(SYS_EXIT, -1);
}

/* Handler for SIG_DFL Stop action */
static void sig_stop(int sig_no)
{
	// FIXME: this kills the process
	syscall1(SYS_EXIT, -1);
}

typedef void(*handler_t)(int);
static handler_t sig_default[] = {
	[SIGABRT]   = sig_terminate,
	[SIGALRM]   = sig_terminate,
	[SIGBUS]    = sig_terminate,
	[SIGCHLD]   = SIG_IGN,
	[SIGCONT]   = SIG_IGN,
	[SIGFPE]    = sig_terminate,
	[SIGHUP]    = sig_terminate,
	[SIGILL]    = sig_terminate,
	[SIGINT]    = sig_terminate,
	[SIGKILL]   = sig_terminate,
	[SIGPIPE]   = sig_terminate,
	[SIGQUIT]   = sig_terminate,
	[SIGSEGV]   = sig_terminate,
	[SIGSTOP]   = sig_stop,
	[SIGTERM]   = sig_terminate,
	[SIGTSTP]   = sig_stop,
	[SIGTTIN]   = sig_stop,
	[SIGTTOU]   = sig_stop,
	[SIGUSR1]   = sig_terminate,
	[SIGUSR2]   = sig_terminate,
	[SIGPOLL]   = sig_terminate,
	[SIGPROF]   = sig_terminate,
	[SIGSYS]    = sig_terminate,
	[SIGTRAP]   = sig_terminate,
	[SIGURG]    = SIG_IGN,
	[SIGVTALRM] = sig_terminate,
	[SIGXCPU]   = sig_terminate,
	[SIGXFSZ]   = sig_terminate,
};

static void sig_set_default(struct sigaction *act, int sig_no)
{
	act->sa_handler = sig_default[sig_no];
}

void sig_init(struct sig_struct *sig)
{
	sig->pending = 0;
	sig->mask = ~0;
	sig->restore = 0;
	for (int i = 0; i < _TELOS_SIGMAX; i++)
		sig_set_default(&sig->actions[i], i);
}

void sig_clone(struct sig_struct *dst, struct sig_struct *src)
{
	dst->pending = 0;
	dst->mask = src->mask;
	dst->restore = src->restore;
	for (int i = 0; i < _TELOS_SIGMAX; i++)
		dst->actions[i] = src->actions[i];
}

void sig_exec(struct sig_struct *sig)
{
	// set signal actions to SIG_DFL, unless they are SIG_IGN
	for (int i = 0; i < _TELOS_SIGMAX; i++) {
		if (sig->actions[i].sa_handler != SIG_IGN)
			sig_set_default(&sig->actions[i], i);
	}
}

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
	sig_cx->stack[1] = (ulong) current->sig.actions[sig_no].sa_handler;
	sig_cx->stack[2] = (ulong) current->esp;
	sig_cx->stack[3] = (ulong) sig_no;
	if (current->sig.actions[sig_no].sa_flags & SA_SIGINFO) {
		struct siginfo *info = &current->sig.infos[sig_no];
		sig_cx->stack[4]  = info->si_errno;
		sig_cx->stack[5]  = info->si_code;
		sig_cx->stack[6]  = info->si_pid;
		sig_cx->stack[7]  = (ulong) info->si_addr;
		sig_cx->stack[8]  = info->si_status;
		sig_cx->stack[9]  = info->si_band;
		sig_cx->stack[10] = (ulong) info->si_value.sigval_ptr;
	}
	// sigsuspend: save original signal mask on the stack
	if (current->sig.restore) {
		sig_cx->stack[11] = current->sig.restore;
		current->sig.restore = 0;
	} else {
		sig_cx->stack[11] = current->sig.mask;
	}
}

static inline struct ucontext *frame_pos(ulong old_esp)
{
	return (struct ucontext*) ((ulong*)old_esp - 30);
}

static int send_signal_user(int sig_no)
{
	struct ucontext *old_cx = current->esp;
	struct sigaction *act = &current->sig.actions[sig_no];
	struct ucontext *sig_frame = frame_pos(old_cx->iret_esp);

	if (act->sa_handler == SIG_IGN)
		goto end;

	place_sig_frame(sig_frame, sig_no);

	current->ifp = current->esp;
	current->esp = sig_frame;
	current->sig.mask = current->sig.mask & ~(act->sa_mask);
end:
	clear_bit(sig_no-1, &current->sig.pending);
	return 0;
}

/*-----------------------------------------------------------------------------
 * Prepares a process to switch contexts into the signal handler for the given
 * signal */
//-----------------------------------------------------------------------------
static int send_signal(struct pcb *p, int sig_no)
{
	if (!signo_valid(sig_no))
		return -EINVAL;
	if (p->flags & PFLAG_SUPER)
		return -ENOTSUP;
	return send_signal_user(sig_no);
}

void handle_signal(void)
{
	if (signal_pending(&current->sig))
		send_signal(current, pending_signal(&current->sig));
	//if (current->sig.pending & current->sig.mask)
		//send_signal(current, fls(current->sig.pending));
}

/*-----------------------------------------------------------------------------
 * Restore CPU & signal context that was active before the current signal */
//-----------------------------------------------------------------------------
long sig_restore(struct ucontext *old_cx)
{
	// TODO: verify that cx is in valid range and properly aligned
	current->esp = old_cx;
	current->ifp = old_cx + 1;

	// restore old signal mask
	current->sig.mask = frame_pos(old_cx->iret_esp)->stack[11];
	return 0;
}

static int verify_sigaction(struct sigaction *act)
{
	if (vm_verify(&current->mm, act, sizeof(*act), VM_READ))
		return -1;
	if (act->sa_handler == SIG_DFL || act->sa_handler == SIG_IGN)
		return 0;
	// FIXME: handler should have VM_EXEC
	if (vm_verify(&current->mm, act->sa_handler,
				sizeof(*(act->sa_handler)), VM_READ))
		return -1;
	return 0;
}

long sys_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	struct sigaction *sap = &current->sig.actions[sig];

	if (act && verify_sigaction(act))
		return -EFAULT;
	if (oact && vm_verify(&current->mm, oact, sizeof(*oact), VM_WRITE))
		return -EFAULT;
	if (!signo_valid(sig) || sig_unblockable(sig))
		return -EINVAL;

	if (oact)
		*oact = *sap;

	if (act) {
		*sap = *act;
		if (act->sa_handler == SIG_DFL)
			sig_set_default(sap, sig);
		set_bit(sig-1, &current->sig.mask);
	}

	return 0;
}

long sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	if (set && vm_verify(&current->mm, set, sizeof(*set), VM_READ))
		return -EFAULT;
	if (oset && vm_verify(&current->mm, oset, sizeof(*oset), VM_WRITE))
		return -EFAULT;

	if (oset)
		*oset = ~(current->sig.mask);

	if (set) {
		switch (how) {
		case SIG_BLOCK:
			current->sig.mask &= ~(*set);
			current->sig.mask |= SIG_NOIGNORE;
			break;
		case SIG_SETMASK:
			current->sig.mask = ~(*set) | SIG_NOIGNORE;
			break;
		case SIG_UNBLOCK:
			current->sig.mask |= *set;
			break;
		default:
			return -EINVAL;
		}
	}
	return 0;
}

long sys_sigwait(sigset_t set, int *sig)
{
	int signo;

	if (vm_verify(&current->mm, sig, sizeof(*sig), VM_WRITE))
		return -EFAULT;

	set &= ~SIG_NOIGNORE;
	if (current->sig.pending & set) {
		signo = fls(current->sig.pending & set);
	} else {
		for (;;) {
			current->state = PROC_SIGWAIT;
			signo = schedule();
			if (sigismember(&set, signo))
				break;
			if (signal_accepted(&current->sig, signo))
				return -EINTR;
		}
	}
	*sig = signo;
	clear_bit(signo-1, &current->sig.pending);
	return 0;
}

long sys_sigsuspend(sigset_t mask)
{
	mask = ~mask | SIG_NOIGNORE;
	current->sig.restore = current->sig.mask;
	current->sig.mask = mask;

	current->state = PROC_INTERRUPTIBLE;
	schedule();
	return -EINTR;
}

static inline bool needs_wake(struct pcb *p, int sig_no)
{
	return p->state == PROC_SIGWAIT ||
		(p->state == PROC_INTERRUPTIBLE && signal_accepted(&p->sig, sig_no)) ||
		(p->state == PROC_WAITING &&
			(sig_no == SIGCHLD || signal_accepted(&p->sig, sig_no)));
}

void __kill(struct pcb *p, int sig_no, int code)
{
	set_bit(sig_no-1, &p->sig.pending);
	if (p->sig.actions[sig_no].sa_flags & SA_SIGINFO) {
		struct ucontext *cx = p->esp;
		p->sig.infos[sig_no].si_code = code;
		p->sig.infos[sig_no].si_errno = 0;
		p->sig.infos[sig_no].si_pid   = current->pid;
		p->sig.infos[sig_no].si_addr = (void*) cx->iret_eip;
	}
	if (needs_wake(p, sig_no))
		wake(p, sig_no);
}

long sys_kill(pid_t pid, int sig)
{
	struct pcb *p = get_pcb(pid);
	if (!p)
		return -ESRCH;
	if (sig == 0)
		return 0;
	if (!signo_valid(sig))
		return -EINVAL;
	__kill(p, sig, SI_USER);
	return 0;
}

long sys_sigqueue(pid_t pid, int sig, const union sigval value)
{
	struct pcb *p = get_pcb(pid);
	if (!p)
		return -ESRCH;
	if (sig == 0)
		return 0;
	if (!signo_valid(sig))
		return -EINVAL;
	p->sig.infos[sig].si_value = value;
	__kill(p, sig, SI_QUEUE);
	return 0;
}
