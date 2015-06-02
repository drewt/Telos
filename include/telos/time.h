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

#ifndef _TELOS_TIME_H_
#define _TELOS_TIME_H_

#define CLOCK_REALTIME           0
#define CLOCK_MONOTONIC          1
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID  3
#define __NR_CLOCKS              4

#define __TICKS_PER_SEC 100
#define __NSEC_PER_TICK 10000000

#ifndef __ASSEMBLER__
#define __need_NULL
#define __need_size_t
#include <stddef.h>
#include <telos/type_defs.h>

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

#ifndef _STRUCT_TIMESPEC_DEFINED
#define _STRUCT_TIMESPEC_DEFINED
_STRUCT_TIMESPEC_DEFN
#endif

#ifndef _STRUCT_ITIMERSPEC_DEFINED
#define _STRUCT_ITIMERSPEC_DEFINED
_STRUCT_ITIMERSPEC_DEFN
#endif

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

#define __TIMESPEC_TO_TICKS(t) \
	((t)->tv_sec * __TICKS_PER_SEC + (t)->tv_nsec / __NSEC_PER_TICK)

#define __TICKS_TO_TIMESPEC(ts, ticks)                                        \
	do {                                                                  \
		(ts)->tv_nsec = (ticks) / __TICKS_PER_SEC;                    \
		(ts)->tv_nsec = ((ticks) % __TICKS_PER_SEC) * __NSEC_PER_TICK;\
	} while (0)

#endif /* !__ASSEMBLER__ */
#endif
