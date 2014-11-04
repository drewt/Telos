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

#ifndef _KERNEL_TIME_H_
#define _KERNEL_TIME_H_

#include <sys/type_macros.h>

#ifndef NULL
#define NULL _NULL_DEFN
#endif
#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
typedef _CLOCK_T_TYPE clock_t;
#endif
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef _SIZE_T_TYPE size_t;
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

enum {
	CLOCK_REALTIME,
	CLOCK_MONOTONIC,
	__NR_CLOCKS
};

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

#ifdef __KERNEL__

extern unsigned long tick_count;
extern unsigned long system_clock;

struct clock {
	int (*get)(struct timespec*);
	int (*set)(struct timespec*);
	struct timespec res;
};

extern int posix_rtc_get(struct timespec *tp);
extern int posix_rtc_set(struct timespec *tp);
extern int posix_monotonic_get(struct timespec *tp);

extern struct clock posix_clocks[];

static inline unsigned long tm_to_unix(struct tm *t)
{
	return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400
		+ (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400
		- ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}

#endif /* __KERNEL__ */
#endif
