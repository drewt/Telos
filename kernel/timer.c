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

#include <kernel/list.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>

/*
 * Kernel timers are implemented as a delta list of callbacks.
 */

static LIST_HEAD(timers);

static DEFINE_SLAB_CACHE(ktimers_cachep, sizeof(struct timer));
#define get_timer() slab_alloc(ktimers_cachep)
#define free_timer(p) slab_free(ktimers_cachep, p)

int ktimer_start(struct timer *timer, unsigned long ticks)
{
	struct timer *t;

	timer->expires = tick_count + ticks;
	timer->flags |= TF_ARMED;

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

struct timer *ktimer_create(void(*act)(void*), void *data, unsigned int flags)
{
	struct timer *timer;

	if ((timer = get_timer()) == NULL)
		return NULL;

	ktimer_init(timer, act, data, flags);

	return timer;
}

unsigned long ktimer_destroy(struct timer *t)
{
	unsigned long expires = t->expires;

	if (t->flags & TF_ARMED && t->flags & TF_ALWAYS && expires > tick_count)
		t->action(t->data);

	list_del(&t->chain);
	if (!(t->flags & TF_STATIC))
		free_timer(&t);

	return (expires <= tick_count) ? 0 : expires - tick_count;
}

/*
 * Stop a timer.  If the timer is not externally referenced, it is destroyed.
 * TODO: pause/resume functionality
 */
unsigned long ktimer_remove(struct timer *timer)
{
	list_del(&timer->chain);
	return timer->expires - tick_count;
}

/*
 * Called whenever the hardware timer goes off.  Updates software timers.
 */
void ktimers_tick(void)
{
	struct timer *t, *n;

	if (list_empty(&timers))
		return;

	list_for_each_entry_safe(t, n, &timers, chain) {
		if (t->expires > tick_count)
			break;

		t->flags &= ~TF_ARMED;
		t->action(t->data);
		ktimer_destroy(t);
	}
}
