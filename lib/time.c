/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include <errno.h>
#include <syscall.h>

#include <stdio.h>

#define YEAR0 1900
#define SECS_PER_DAY (24L * 60L * 60L)
#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year) (LEAPYEAR(year) ? 366 : 365)


static const unsigned _ytab[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	unsigned long left;

	if (req->tv_nsec < 0 || req->tv_nsec > 999999999)
		return -1; /* EINVAL */

	left = syscall1(SYS_SLEEP, __timespec_to_ticks(req));

	if (left != 0) {
		__ticks_to_timespec(rem, left);
		return -1; /* EINTR */
	}
	return 0;
}

time_t time(time_t *t)
{
	time_t tm;
	int rv = syscall1(SYS_TIME, &tm);
	if (rv < 0)
		return (time_t)-1;
	if (t)
		*t = tm;
	return tm;
}

int clock_getres(clockid_t clockid, struct timespec *res)
{
	return syscall2(SYS_CLOCK_GETRES, clockid, res);
}

int clock_gettime(clockid_t clockid, struct timespec *tp)
{
	return syscall2(SYS_CLOCK_GETTIME, clockid, tp);
}

int clock_settime(clockid_t clockid, struct timespec *tp)
{
	return syscall2(SYS_CLOCK_SETTIME, clockid, tp);
}

int timer_create(clockid_t clockid, struct sigevent *restrict sevp,
		timer_t *restrict timerid)
{
	return syscall3(SYS_TIMER_CREATE, clockid, sevp,
			timerid);
}

int timer_delete(timer_t timerid)
{
	return syscall1(SYS_TIMER_DELETE, timerid);
}

int timer_gettime(timer_t timerid, struct itimerspec *curr_value)
{
	return syscall2(SYS_TIMER_GETTIME, timerid, curr_value);
}

int timer_settime(timer_t timerid, int flags,
		const struct itimerspec *restrict new_value,
		struct itimerspec *restrict old_value)
{
	return syscall4(SYS_TIMER_SETTIME, timerid, flags, new_value, old_value);
}

struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
	unsigned long dayclock, dayno;
	time_t time = *timep;
	int year = 1970;

	dayclock = (unsigned long)time % SECS_PER_DAY;
	dayno    = (unsigned long)time / SECS_PER_DAY;

	result->tm_sec = dayclock % 60;
	result->tm_min = (dayclock % 3600) / 60;
	result->tm_hour = dayclock / 3600;
	result->tm_wday = (dayno + 4) % 7;
	while (dayno >= YEARSIZE(year)) {
		dayno -= YEARSIZE(year);
		year++;
	}
	result->tm_year = year - YEAR0;
	result->tm_yday = dayno;
	result->tm_mon = 0;
	while (dayno >= _ytab[LEAPYEAR(year)][result->tm_mon]) {
		dayno -= _ytab[LEAPYEAR(year)][result->tm_mon];
		result->tm_mon++;
	}
	result->tm_mday = dayno + 1;
	result->tm_isdst = 0;

	return result;
}

struct tm *gmtime(const time_t *timep)
{
	static struct tm tm;
	return gmtime_r(timep, &tm);
}
