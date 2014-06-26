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
#include <kernel/dispatch.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/signal.h>
#include <kernel/hashtable.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>

struct posix_timer {
	struct list_head chain;
	struct hlist_node t_hash;
	struct sigevent sev;
	struct itimerspec spec;
	struct timer *timer;
	pid_t pid;
	timer_t timerid;
};

struct clock posix_clocks[__NR_CLOCKS] = {
	[CLOCK_REALTIME] = {
		.get = posix_rtc_get,
		.set = posix_rtc_set,
		.res = { .tv_sec = 1, .tv_nsec = 0 }
	},
	[CLOCK_MONOTONIC] = {
		.get = posix_monotonic_get,
		.set = NULL,
		.res = { .tv_sec = 0, .tv_nsec = __NSEC_PER_TICK }
	}
};

LIST_HEAD(free_timers);
DEFINE_HASHTABLE(posix_timers, 9);

unsigned long tick_count = 0; /* global tick count */
unsigned long system_clock;   /* seconds since the Epoch */

DEFINE_ALLOCATOR(get_posix_timer, struct posix_timer, &free_timers, chain)

/*
 * Wakes a sleeping process.
 */
static void wake_action(void *data)
{
	struct pcb *p = (struct pcb*) data;
	p->rc = 0;
	ready(p);
}

/*
 * Sends SIGALRM to the process.
 */
static void alrm_action(void *data)
{
	struct pcb *p = (struct pcb*) data;
	__kill(p, SIGALRM);
}

/*
 * Puts the current process to sleep for a given number of milliseconds.
 */
long sys_sleep(unsigned long ticks)
{
	ktimer_init(&current->t_sleep, wake_action, current, TF_ALWAYS | TF_REF);
	ktimer_start(&current->t_sleep, ticks);
	new_process();
	return 0;
}

/*
 * Registers an alarm for the current process, to go off in the given number of
 * seconds.
 */
long sys_alarm(unsigned long ticks)
{
	long rc;

	if (current->t_alarm.flags & TF_ARMED) {
		rc = ktimer_remove(&current->t_alarm);
	} else {
		rc = 0;
	}

	if (ticks == 0)
		return rc;

	ktimer_init(&current->t_alarm, alrm_action, current, TF_REF);
	ktimer_start(&current->t_alarm, ticks);
	return rc;
}

long sys_time(time_t *t)
{
	/* FIXME: check address */
	if (t != NULL)
		copy_to_current(t, &system_clock, sizeof(time_t));

	return system_clock;
}

int posix_rtc_get(struct timespec *tp)
{
	tp->tv_sec  = system_clock;
	tp->tv_nsec = 0;
	return 0;
}

int posix_rtc_set(struct timespec *tp)
{
	system_clock = tp->tv_sec;
	return 0;
}

int posix_monotonic_get(struct timespec *tp)
{
	tp->tv_sec  = tick_count / __TICKS_PER_SEC;
	tp->tv_nsec = (tick_count % __TICKS_PER_SEC) * __NSEC_PER_TICK;
	return 0;
}

static int clock_valid(clockid_t clock)
{
	return clock >= 0 && clock < __NR_CLOCKS;
}

long sys_clock_getres(clockid_t clockid, struct timespec *res)
{
	if (!clock_valid(clockid))
		return -EINVAL;

	/* FIXME: check address */
	copy_to_current(res, &posix_clocks[clockid], sizeof(*res));
	return 0;
}

long sys_clock_gettime(clockid_t clockid, struct timespec *tp)
{
	struct timespec spec;

	if (!clock_valid(clockid))
		return -EINVAL;

	posix_clocks[clockid].get(&spec);

	/* FIXME: check address */
	copy_to_current(tp, &spec, sizeof(*tp));
	return 0;
}

long sys_clock_settime(clockid_t clockid, struct timespec *tp)
{
	struct timespec spec;

	if (!clock_valid(clockid))
		return -EINVAL;

	if (posix_clocks[clockid].set == NULL)
		return -EPERM;

	/* FIXME: check address */
	copy_from_current(&spec, tp, sizeof(spec));
	posix_clocks[clockid].set(&spec);
	return 0;
}

static struct posix_timer *get_timer_by_id(timer_t timerid)
{
	struct posix_timer *it;

	hash_for_each_possible(posix_timers, it, t_hash, timerid) {
		if (it->pid == current->pid && it->timerid == timerid)
			return it;
	}
	return NULL;
}

static void posix_timer_action(void *data)
{
	struct posix_timer *pt = data;
	struct pcb *p = &proctab[PT_INDEX(pt->pid)];

	current->siginfos[pt->sev.sigev_signo].si_value =
		pt->sev.sigev_value;

	switch (pt->sev.sigev_notify) {
	case SIGEV_SIGNAL:
		__kill(p, pt->sev.sigev_signo);
		break;
	default:
		break;
	}
}

static inline int sigev_valid(struct sigevent *sevp)
{
	switch (sevp->sigev_notify) {
	case SIGEV_NONE:
	case SIGEV_SIGNAL:
		break;
	default:
		return 0;
	}

	if (!signo_valid(sevp->sigev_signo))
		return 0;

	return 1;
}

long sys_timer_create(clockid_t clockid, struct sigevent *sevp,
		timer_t *timerid)
{
	long error;
	struct posix_timer *pt;

	if (clockid != CLOCK_MONOTONIC)
		return -ENOTSUP;

	if ((pt = get_posix_timer()) == NULL)
		return -ENOMEM;

	pt->pid = current->pid;
	pt->timerid = current->posix_timer_id++;

	if (sevp == NULL) {
		pt->sev.sigev_notify = SIGEV_SIGNAL;
		pt->sev.sigev_signo = SIGALRM;
		pt->sev.sigev_value.sigval_int = pt->timerid;
	} else {
		copy_from_current(&pt->sev, sevp, sizeof(*sevp));
		if (!sigev_valid(&pt->sev)) {
			error = -EINVAL;
			goto err0;
		}
	}

	pt->timer = ktimer_create(posix_timer_action, pt, TF_REF);
	if (pt->timer == NULL) {
		error = -ENOMEM;
		goto err0;
	}

	list_add_tail(&pt->chain, &current->posix_timers);
	hash_add(posix_timers, &pt->t_hash, pt->timerid);

	copy_to_current(timerid, &pt->timerid, sizeof(*timerid));
	return 0;
err0:
	list_add(&pt->chain, &free_timers);
	return error;
}

long sys_timer_delete(timer_t timerid)
{
	struct posix_timer *pt = get_timer_by_id(timerid);

	if (pt == NULL)
		return -EINVAL;

	list_del(&pt->chain);
	list_add(&pt->chain, &free_timers);
	hash_del(&pt->t_hash);

	return 0;
}

long sys_timer_gettime(timer_t timerid, struct itimerspec *curr_value)
{
	unsigned long ticks;
	struct posix_timer *pt = get_timer_by_id(timerid);

	if (pt == NULL)
		return -EINVAL;

	ticks = pt->timer->expires - tick_count;
	pt->spec.it_value.tv_sec = (ticks % 100) ? ticks/100 + 1 : ticks/100;
	pt->spec.it_value.tv_nsec = 0;

	copy_to_current(curr_value, &pt->spec, sizeof(*curr_value));
	return 0;
}

long sys_timer_settime(timer_t timerid, int flags,
		const struct itimerspec *new_value,
		struct itimerspec *old_value)
{
	struct posix_timer *pt = get_timer_by_id(timerid);
	struct itimerspec *v = &pt->spec;

	if (pt == NULL)
		return -EINVAL;

	copy_from_current(v, new_value, sizeof(*new_value));

	ktimer_start(pt->timer, __timespec_to_ticks(&v->it_value));
	return 0;
}

/*
 * Called on every timer interrupt; updates global tick count, the event queue,
 * and switches to another process
 */
void tick(void)
{
	if ((++tick_count % __TICKS_PER_SEC) == 0)
		system_clock++;

	ktimers_tick();

	/* choose new process to run */
	ready(current);
	new_process();
	pic_eoi();
}
