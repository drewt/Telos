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

#include <kernel/list.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <kernel/timer.h>

/*
 * Kernel timers are implemented as a delta list of callbacks.
 */

/*
 * A "zombie" timer is expired/destroyed but still referenced.
 */
static LIST_HEAD(zombie_timers);
static LIST_HEAD(free_timers);
static LIST_HEAD(timers);

DEFINE_ALLOCATOR(get_timer, struct timer, &free_timers, chain)

int timer_start(struct timer *timer, unsigned long ticks)
{
	struct timer *t;

	timer_ref(timer);
	timer->expires = tick_count + ticks;
	//timer->expires = tick_count + ((ms % 10) ? ms/10 + 1 : ms/10);

	/* overflow */
	if (timer->expires < tick_count) {
		list_for_each_entry_reverse(t, &timers, chain) {
			/* too far */
			if (t->expires > tick_count) {
				t = list_entry(t->chain.next, struct timer, chain);
				break;
			}
			if (t->expires > timer->expires)
				break;
		}
	} else {
		list_for_each_entry(t, &timers, chain) {
			if (t->expires > timer->expires)
				break;
		}
	}

	list_add_tail(&timer->chain, &t->chain);

	return 0;
}

struct timer *timer_create(void(*act)(void*), void *data, unsigned int flags)
{
	struct timer *timer;

	if ((timer = get_timer()) == NULL)
		return NULL;

	timer->action = act;
	timer->data = data;
	timer->flags = flags;
	timer->ref = flags & TF_REF ? 1 : 0;

	return timer;
}

int __timer_destroy(struct timer *timer)
{
	list_del(&timer->chain);
	list_add(&timer->chain, &free_timers);

	if (timer->flags & TF_ALWAYS && timer->expires > tick_count)
		timer->action(timer->data);

	timer->expires = 0;
	return 0;
}

/*
 * Stop a timer.  If the timer is not externally referenced, it is destroyed.
 * TODO: pause/resume functionality
 */
unsigned long timer_remove(struct timer *timer)
{
	int ticks = timer->expires - tick_count;

	list_del(&timer->chain);
	list_add(&timer->chain, &zombie_timers);
	timer_unref(timer);

	return ticks;
}

/*
 * Called whenever the hardware timer goes off.  Updates software timers.
 */
void timers_tick(void)
{
	struct timer *t, *n;

	if (list_empty(&timers))
		return;

	list_for_each_entry_safe(t, n, &timers, chain) {
		if (t->expires > tick_count)
			break;

		t->action(t->data);
		list_del(&t->chain);
		list_add(&t->chain, &zombie_timers);
		timer_unref(t);
	}
}
