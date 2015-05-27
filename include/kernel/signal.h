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

#ifndef _KERNEL_SIGNAL_H_
#define _KERNEL_SIGNAL_H_

#include <kernel/bitops.h>
#include <sys/signal.h>

#define signo_valid __signo_valid

struct sig_struct {
	sigset_t pending;
	sigset_t mask;
	sigset_t restore;
	struct sigaction actions[_TELOS_SIGMAX];
	struct siginfo infos[_TELOS_SIGMAX];
};
void sig_init(struct sig_struct *sig);
void sig_clone(struct sig_struct *dst, struct sig_struct *src);
void sig_exec(struct sig_struct *sig);

static inline int signal_pending(struct sig_struct *sig)
{
	return sig->pending & sig->mask;
}

static inline int pending_signal(struct sig_struct *sig)
{
	return fls(sig->pending & sig->mask);
}

static inline int signal_ignored(struct sig_struct *sig, int sig_no)
{
	return sig->actions[sig_no].sa_handler == SIG_IGN;
}

static inline int signal_blocked(struct sig_struct *sig, int sig_no)
{
	return !sigismember(&sig->mask, sig_no);
}

static inline int signal_accepted(struct sig_struct *sig, int sig_no)
{
	return !signal_ignored(sig, sig_no) && !signal_blocked(sig, sig_no);
}

static inline int signal_from_user(int code)
{
	return code == SI_USER || code == SI_QUEUE || code == SI_TIMER ||
		code == SI_ASYNCIO || code == SI_MESGQ;
}

#endif
