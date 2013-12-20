/*  Copyright 2013 Drew Thoreson
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

#ifndef _TIME_H_
#define _TIME_H_

#include <kernel/time.h>
#include <stddef.h> /* NULL */
#include <sys/types.h>

struct sigevent;

int nanosleep(const struct timespec *req, struct timespec *rem);

int timer_create(clockid_t clockid, struct sigevent *restrict sevp,
		timer_t *restrict timerid);

int timer_delete(timer_t timerid);

int timer_gettime(timer_t timerid, struct itimerspec *curr_value);

int timer_settime(timer_t timerid, int flags,
		const struct itimerspec *restrict new_value,
		struct itimerspec *restrict old_value);

#endif
