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

#ifndef _SYS_TYPE_MACROS_H_
#define _SYS_TYPE_MACROS_H_

#define _EOF_DEFN (-1)

#define _BLKCNT_T_TYPE long
#define _BLKSIZE_T_TYPE long
#define _CLOCK_T_TYPE unsigned long
#define _CLOCKID_T_TYPE int
#define _DEV_T_TYPE unsigned long
#define _FSBLKCNT_T_TYPE unsigned long
#define _FSFILCNT_T_TYPE unsigned long
#define _GID_T_TYPE unsigned long
#define _ID_T_TYPE unsigned long
#define _INO_T_TYPE unsigned long
#define _INTPTR_T_TYPE long
#define _KEY_T_TYPE TODO
#define _MODE_T_TYPE unsigned long
#define _NLINK_T_TYPE unsigned long
#define _OFF_T_TYPE long
#define _PID_T_TYPE int
#define _PTHREAD_ATTR_T_TYPE TODO
#define _PTHREAD_BARRIER_T_TYPE TODO
#define _PTHREAD_BARRIERATTR_T_TYPE TODO
#define _PTHREAD_COND_T_TYPE TODO
#define _PTHREAD_CONDATTR_T_TYPE TODO
#define _PTHREAD_KEY_T_TYPE TODO
#define _PTHREAD_MUTEX_T_TYPE TODO
#define _PTHREAD_MUTEXATTR_T_TYPE TODO
#define _PTHREAD_ONCE_T_TYPE TODO
#define _PTHREAD_RWLOCK_T_TYPE TODO
#define _PTHREAD_RWLOCKATTR_T_TYPE TODO
#define _PTHREAD_SPINLOCK_T_TYPE TODO
#define _PTHREAD_T_TYPE TODO
#define _SSIZE_T_TYPE int
#define _SUSECONDS_T_TYPE TODO
#define _TIME_T_TYPE unsigned long
#define _TIMER_T_TYPE unsigned long
#define _TRACE_ATTR_T_TYPE TODO
#define _TRACE_EVENT_ID_T_TYPE TODO
#define _TRACE_EVENT_SET_T_TYPE TODO
#define _TRACE_ID_T_TYPE TODO
#define _UID_T_TYPE unsigned long
#define _UINTPTR_T_TYPE unsigned long

#define _FPOS_T_TYPE unsigned long
#define _LOCALE_T_TYPE unsigned long
#define _SIG_ATOMIC_T_TYPE int
#define _SIGSET_T_TYPE unsigned long

#define _STRUCT_TIMESPEC_DEFN \
	struct timespec { \
		time_t tv_sec; \
		long tv_nsec; \
	};

#define _UNION_SIGVAL_DEFN \
	union sigval { \
		int  sigval_int; \
		void *sigval_ptr; \
	};

#define _SIGINFO_T_DEFN \
	typedef struct siginfo { \
		int          si_signo; \
		int          si_code; \
		int          si_errno; \
		pid_t        si_pid; \
		uid_t        si_uid; \
		void         *si_addr; \
		int          si_status; \
		int          si_band; \
		union sigval si_value; \
	} siginfo_t;

#define _MAKEDEV_DEFN(maj, min) (((maj) << 8) | ((min) & 0xFF))
#define _MAJOR_DEFN(dev) ((dev) >> 8)
#define _MINOR_DEFN(dev) ((dev) & 0xFF)

#endif
