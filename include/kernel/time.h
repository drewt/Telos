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

#include <kernel/types.h>

#define __TICKS_PER_SEC 10
#define __NSEC_PER_TICK 10000000

enum {
	CLOCK_REALTIME,
	CLOCK_MONOTONIC,
};

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

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
	return t->tv_sec * __TICKS_PER_SEC + t->tv_nsec * __NSEC_PER_TICK;
}

static inline void __ticks_to_timespec(struct timespec *t, unsigned long ticks)
{
	t->tv_sec = ticks / __TICKS_PER_SEC;
	t->tv_nsec = (ticks % __TICKS_PER_SEC) * __NSEC_PER_TICK;
}

#ifdef __KERNEL__

extern unsigned long tick_count;

extern unsigned long system_clock;

static inline unsigned long tm_to_unix(struct tm *t)
{
	return t->tm_sec + t->tm_min*60 + t->tm_hour*3600 + t->tm_yday*86400
		+ (t->tm_year-70)*31536000 + ((t->tm_year-69)/4)*86400
		- ((t->tm_year-1)/100)*86400 + ((t->tm_year+299)/400)*86400;
}

#endif /* __KERNEL__ */
#endif
