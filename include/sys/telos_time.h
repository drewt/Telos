/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#ifndef _SYS_TELOS_TIME_H_
#define _SYS_TELOS_TIME_H_

#define __need_NULL
#define __need_size_t
#include <stddef.h>
#include <sys/type_defs.h>

#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
typedef _CLOCK_T_TYPE clock_t;
#endif
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef _TIME_T_TYPE time_t;
#endif
#ifndef _CLOCKID_T_DEFINED
#define _CLOCKID_T_DEFINED
typedef _CLOCKID_T_TYPE clockid_t;
#endif
#ifndef _TIMER_T_DEFINED
#define _TIMER_T_DEFINED
typedef _TIMER_T_TYPE timer_t;
#endif
#ifndef _LOCALE_T_DEFINED
#define _LOCALE_T_DEFINED
typedef _LOCALE_T_TYPE locale_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif

#define __TICKS_PER_SEC 100
#define __NSEC_PER_TICK 10000000

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define __NR_CLOCKS              4

#ifndef _STRUCT_TIMESPEC_DEFINED
#define _STRUCT_TIMESPEC_DEFINED
_STRUCT_TIMESPEC_DEFN
#endif

struct itimerspec {
	struct timespec it_interval;
	struct timespec it_value;
};

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

static inline unsigned long __timespec_to_ticks(const struct timespec *t)
{
	return t->tv_sec * __TICKS_PER_SEC + t->tv_nsec / __NSEC_PER_TICK;
}

static inline void __ticks_to_timespec(struct timespec *t, unsigned long ticks)
{
	t->tv_sec = ticks / __TICKS_PER_SEC;
	t->tv_nsec = (ticks % __TICKS_PER_SEC) * __NSEC_PER_TICK;
}

#endif
