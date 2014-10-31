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

#include <sys/type_macros.h>

#ifndef _PTHREAD_T_DEFINED
#define _PTHREAD_T_DEFINED
//typedef _PTHREAD_T_TYPE pthread_t;
#endif
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef _SIZE_T_TYPE size_t;
#endif
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef _UID_T_TYPE uid_t;
#endif
#ifndef _SIG_ATOMIC_T_DEFINED
#define _SIG_ATOMIC_T_DEFINED
typedef _SIG_ATOMIC_T_TYPE sig_atomic_t;
#endif
#ifndef _SIGSET_T_DEFINED
#define _SIGSET_T_DEFINED
typedef _SIGSET_T_TYPE sigset_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef _TIME_T_TYPE time_t;
#endif
#ifndef _STRUCT_TIMESPEC_DEFINED
#define _STRUCT_TIMESPEC_DEFINED
_STRUCT_TIMESPEC_DEFN
#endif

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
	int          si_signo;  // signal number
	int          si_code;   // signal code
	int          si_errno;  // errno value associated with this signal
	pid_t        si_pid;    // sending process ID
	uid_t        si_uid;    // real user ID of sending process
	void         *si_addr;  // address of faulting instruction
	int          si_status; // exit value or signal
	int          si_band;   // band event for SIGPOLL
	union sigval si_value;  // signal value
} siginfo_t;

struct sigaction {
	union {
		void(*sa_handler)(int);
		void(*sa_sigaction)(int,siginfo_t*,void*);
	};
	sigset_t	sa_mask;
	int		sa_flags;
};

#endif
