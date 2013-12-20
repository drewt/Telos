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

#ifndef _KERNEL_SIGDEFS_H_
#define _KERNEL_SIGDEFS_H_

#ifdef __KERNEL__
#include <kernel/types.h>
#else
#include <stddef.h>
typedef int pid_t;
#endif

typedef unsigned long sigset_t;

typedef struct sigstack {
	int	ss_onstack;	// non-zero when signal stack is in use
	void	*ss_sp;		// stack base or pointer
	size_t	ss_size;	// stack size
	int	ss_flags;	// flags
} stack_t;

union sigval {
	int  sigval_int;
	void *sigval_ptr;
};

struct sigevent {
	int		sigev_notify;
	int		sigev_signo;
	union sigval	sigev_value;
	void(*sigev_notify_function)(union sigval);
};

typedef struct siginfo {
	int		si_signo;	// signal number
	int		si_errno;	// errno value associated with this signal
	int		si_code;	// signal code
	pid_t		si_pid;		// sending process ID
	void		*si_addr;	// address of faulting instruction
	int		si_status;	// exit value or signal
	int		si_band;	// band event for SIGPOLL
	union sigval	si_value;	// signal value
} siginfo_t;

struct sigaction {
	union {
		void(*sa_handler)(int);
		void(*sa_sigaction)(int,siginfo_t*,void*);
	} _u;
	sigset_t	sa_mask;
	int		sa_flags;
};

#define sa_handler   _u.sa_handler
#define sa_sigaction _u.sa_sigaction

#endif
