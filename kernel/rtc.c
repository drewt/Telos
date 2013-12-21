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

#include <kernel/i386.h>
#include <kernel/time.h>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

#define RTC_SECOND   0x00
#define RTC_MINUTE   0x02
#define RTC_HOUR     0x04
#define RTC_DAY      0x07
#define RTC_MONTH    0x08
#define RTC_YEAR     0x09

#define RTC_STATUSA  0x0A
#define RTC_STATUSB  0x0B

#define RTCF_24HOUR  0x02
#define RTCF_BINARY  0x04

static inline unsigned char get_rtc_register(short reg)
{
	outb(CMOS_ADDRESS, reg);
	return inb(CMOS_DATA);
}

static inline int update_in_progress(void)
{
	return get_rtc_register(RTC_STATUSA) & 0x80;
}

static void rtc_read_date(struct tm *date)
{
	while (update_in_progress())
		/* nothing */;
	date->tm_sec  = get_rtc_register(RTC_SECOND);
	date->tm_min  = get_rtc_register(RTC_MINUTE);
	date->tm_hour = get_rtc_register(RTC_HOUR);
	date->tm_mday = get_rtc_register(RTC_DAY);
	date->tm_mon  = get_rtc_register(RTC_MONTH);
	date->tm_year = get_rtc_register(RTC_YEAR);
}

static void bcd_to_binary(struct tm *t)
{
	t->tm_sec  = (t->tm_sec & 0x0F) + ((t->tm_sec / 16) * 10);
	t->tm_min  = (t->tm_min & 0x0F) + ((t->tm_min / 16) * 10);
	t->tm_hour = ((t->tm_hour & 0x0F) + (((t->tm_hour & 0x70) / 16) * 10))
		| (t->tm_hour & 0x80);
	t->tm_mday = (t->tm_mday & 0x0F) + ((t->tm_mday / 16) * 10);
	t->tm_mon  = (t->tm_mon & 0x0F) + ((t->tm_mon / 16) * 10);
	t->tm_year = (t->tm_year & 0x0F) + ((t->tm_year / 16) * 10);
}

static int rtc_date_equal(struct tm *a, struct tm *b)
{
	return (a->tm_sec == b->tm_sec) && (a->tm_min == b->tm_min)
		&& (a->tm_hour == b->tm_hour) && (a->tm_mday == b->tm_mday)
		&& (a->tm_mon == b->tm_mon) && (a->tm_year == b->tm_year);
}

static void rtc_to_posix(struct tm *t)
{
	static const int days_by_month[] =
		{0,31,59,90,120,151,181,212,243,273,304,334};

	int leap = (t->tm_year % 4 == 0) && (t->tm_year != 0) && (t->tm_mon > 2);

	t->tm_mday--;
	t->tm_year += 100;
	t->tm_yday = days_by_month[t->tm_mon-1] + leap + t->tm_mday;
	t->tm_isdst = -1;
	/* TODO: wday */
}

void rtc_date(struct tm *date)
{
	unsigned char status;
	struct tm last_date;

	rtc_read_date(date);

	do {
		last_date = *date;
		rtc_read_date(date);
	} while (!rtc_date_equal(&last_date, date));

	status = get_rtc_register(RTC_STATUSB);

	if (!(status & RTCF_BINARY))
		bcd_to_binary(date);

	if (!(status & RTCF_24HOUR) && (date->tm_hour & 0x80))
		date->tm_hour = ((date->tm_hour & 0x7F) + 12) % 24;

	rtc_to_posix(date);
}

void clock_init(void)
{
	struct tm date;

	rtc_date(&date);
	system_clock = tm_to_unix(&date);
}
