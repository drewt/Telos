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

#include <kernel/list.h>

#define TF_ALWAYS 1 /* call the handler if the timer is prematurely destroyed */
#define TF_STATIC 2 /* timer memory is to be managed manually */
#define TF_ARMED  4 /* timer is currently armed */

struct timer {
	struct list_head chain;
	void(*action)(void*);
	void *data;
	unsigned long expires;
	unsigned long flags;
};

struct timer *ktimer_create(void(*act)(void*), void *data, unsigned int flags);
int ktimer_start(struct timer *timer, unsigned long ticks);
unsigned long ktimer_destroy(struct timer *timer);
unsigned long ktimer_remove(struct timer *timer);
void ktimers_tick(void);

static inline void ktimer_init(struct timer *dst, void(*act)(void*), void *data,
		unsigned int flags)
{
	dst->action = act;
	dst->data = data;
	dst->flags = flags & ~TF_ARMED;
}

#endif
