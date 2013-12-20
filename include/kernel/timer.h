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

#ifndef _KERNEL_TIMER_H_
#define _KERNEL_TIMER_H_

#define TF_ALWAYS 1 /* call the handler if the timer is prematurely destroyed */
#define TF_REF    2 /* directive to timer_create to add an extra ref */

struct timer {
	struct list_head chain;
	void(*action)(void*);
	void *data;
	unsigned long expires;
	unsigned long flags;
	int ref;
};

struct timer *timer_create(void(*act)(void*), void *data, unsigned int flags);
int timer_start(struct timer *timer, unsigned long ticks);
int __timer_destroy(struct timer *timer);
unsigned long timer_remove(struct timer *timer);
void timers_tick(void);

static inline void timer_ref(struct timer *timer)
{
	timer->ref++;
}

static inline void timer_unref(struct timer *timer)
{
	if (--timer->ref == 0)
		__timer_destroy(timer);
}

#endif
