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

#ifndef _KERNEL_TIME_H_
#define _KERNEL_TIME_H_

#include <sys/telos_time.h>

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

#endif
