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

#ifndef _KERNEL_WAITQ_H_
#define _KERNEL_WAITQ_H_

#include <kernel/list.h>
#include <kernel/process.h>

struct wait_queue {
	struct list_head waiting;
};

#define WAIT_QUEUE_INIT(name) \
{ \
	.waiting = LIST_HEAD_INIT(name.waiting), \
}

static inline void INIT_WAIT_QUEUE(struct wait_queue *wait)
{
	INIT_LIST_HEAD(&wait->waiting);
}

static inline bool wait_queue_empty(struct wait_queue *q)
{
	return list_empty(&q->waiting);
}

static inline void __wake_first(struct wait_queue *q, int rc)
{
	// XXX: assumes queue has at least one member
	wake(list_first_entry(&q->waiting, struct pcb, wait_chain), rc);
}

static inline void wake_first(struct wait_queue *q, int rc)
{
	if (!wait_queue_empty(q))
		__wake_first(q, rc);
}

static inline void wake_all(struct wait_queue *q, int rc)
{
	struct pcb *p;
	list_for_each_entry(p, &q->waiting, wait_chain) {
		wake(p, rc);
	}
}

int wait_interruptible(struct wait_queue *q);

#endif
