/* Copyright (c) 2013-2015, Drew Thoreson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _TELOS_TYPE_DEFS_H_
#define _TELOS_TYPE_DEFS_H_

/*
 * Special case for ssize_t.  Since gcc is in charge of size_t (via stddef.h),
 * we try to define ssize_t based on what gcc gives us.  Instead of defining
 * _SSIZE_T_TYPE, we do a typedef for _ssize_t.  Any header needing ssize_t
 * ssize_t can then do "typedef _ssize_t ssize_t;".
 */
#ifdef __SIZE_TYPE__
/*
 * If __SIZE_TYPE__ is defined (gcc) we define ssize_t based on size_t.
 * We simply change "unsigned" to "signed" for this single definition
 * to make sure ssize_t and size_t only differ by their signedness.
 */
#define unsigned signed
typedef __SIZE_TYPE__ _ssize_t;
#undef unsigned
#else
#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
typedef int _ssize_t;
#else
typedef long _ssize_t;
#endif
#endif

#define _EOF_DEFN (-1)

#define _BLKCNT_T_TYPE long
#define _BLKSIZE_T_TYPE long
#define _CC_T_TYPE unsigned int
#define _SPEED_T_TYPE unsigned int
#define _TCFLAG_T_TYPE unsigned long
#define _CLOCK_T_TYPE unsigned long
#define _CLOCKID_T_TYPE int
#define _DEV_T_TYPE unsigned long
#define _FPOS_T_TYPE unsigned long
#define _FSBLKCNT_T_TYPE unsigned long
#define _FSFILCNT_T_TYPE unsigned long
#define _GID_T_TYPE unsigned long
#define _ID_T_TYPE unsigned long
#define _INO_T_TYPE unsigned long
#define _INTPTR_T_TYPE long
#define _KEY_T_TYPE long
#define _LOCALE_T_TYPE unsigned long
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
#define _SIG_ATOMIC_T_TYPE int
#define _SIGSET_T_TYPE unsigned long
#define _SUSECONDS_T_TYPE long
#define _TIME_T_TYPE unsigned long
#define _TIMER_T_TYPE unsigned long
#define _TRACE_ATTR_T_TYPE TODO
#define _TRACE_EVENT_ID_T_TYPE TODO
#define _TRACE_EVENT_SET_T_TYPE TODO
#define _TRACE_ID_T_TYPE TODO
#define _UID_T_TYPE unsigned long
#define _UINTPTR_T_TYPE unsigned long
#define _USECONDS_T_TYPE unsigned long

#define _STRUCT_TIMESPEC_DEFN \
	struct timespec { \
		time_t tv_sec; \
		long tv_nsec; \
	};

#define _STRUCT_ITIMERSPEC_DEFN \
	struct itimerspec { \
		struct timespec it_interval; \
		struct timespec it_value; \
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
