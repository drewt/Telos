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

#include <kernel/common.h>
#include <kernel/list.h>
#include <kernel/mem.h>
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

static int grow_timers(void)
{
	struct pf_info *page;
	struct timer *timer;
	int i;

	if ((page = kalloc_page()) == NULL)
		return -1;

	timer = (struct timer*) page->addr;

	i = FRAME_SIZE / sizeof(struct timer);

	for (; i > 0; i--, timer++)
		list_enqueue(&timer->chain, &free_timers);

	return 0;
}

struct timer *get_timer(void)
{
	if (list_empty(&free_timers) && grow_timers() == -1)
		return NULL;

	return list_entry(list_dequeue(&free_timers), struct timer, chain);
}

struct timer *timer_create(unsigned int ms, void(*act)(void*), void *data,
		unsigned int flags)
{
	int ticks;
	struct timer *timer, *t;

	if ((timer = get_timer()) == NULL)
		return NULL;

	timer->action = act;
	timer->data = data;
	timer->flags = flags;
	timer->ref = flags & TF_REF ? 2 : 1;

	ticks = (ms % 10) ? ms/10 + 1 : ms/10;

	list_for_each_entry(t, &timers, chain) {
		if (ticks < t->delta)
			break;
		ticks -= t->delta;
	}

	if (&t->chain != &timers)
		t->delta -= ticks;

	timer->delta = ticks;
	list_add_tail(&timer->chain, &t->chain);

	return timer;
}

int __timer_destroy(struct timer *timer)
{
	list_del(&timer->chain);
	list_add(&timer->chain, &free_timers);

	if (timer->flags & TF_ALWAYS && timer->delta > 0)
		timer->action(timer->data);

	timer->delta = -1;
}

/*
 * Stop a timer.  If the timer is not externally referenced, it is destroyed.
 * TODO: pause/resume functionality
 */
int timer_remove(struct timer *timer)
{
	struct timer *t;
	int ticks = 0;
	int phase = 0;

	list_for_each_entry(t, &timers, chain) {

		/* phase 0: count ticks */
		if (phase == 0) {
			ticks += t->delta;
			if (t == timer)
				phase = 1;
			continue;
		}

		/* phase 1: adjust deltas */
		t->delta += timer->delta;
	}

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

	(list_entry(timers.next, struct timer, chain))->delta--;

	list_for_each_entry_safe(t, n, &timers, chain) {
		if (t->delta > 0)
			break;

		t->action(t->data);
		list_del(&t->chain);
		list_add(&t->chain, &zombie_timers);
		timer_unref(t);
	}
}
