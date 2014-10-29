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
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/signal.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <kernel/mm/vma.h>

#include <kernel/hashtable.h>

struct posix_timer {
	struct list_head chain;
	struct hlist_node t_hash;
	struct sigevent sev;
	struct itimerspec spec;
	struct timer timer;
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

DEFINE_HASHTABLE(posix_timers, 9);

unsigned long tick_count = 0; /* global tick count */
unsigned long system_clock;   /* seconds since the Epoch */

static DEFINE_SLAB_CACHE(posix_timers_cachep, sizeof(struct posix_timer));
#define get_posix_timer() slab_alloc(posix_timers_cachep)
#define free_posix_timer(p) slab_free(posix_timers_cachep, p)

/*
 * Wakes a sleeping process.
 */
static void wake_action(void *data)
{
	wake(data, 0);
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
	current->state = STATE_SLEEPING;
	ktimer_init(&current->t_sleep, wake_action, current, TF_STATIC);
	ktimer_start(&current->t_sleep, ticks);
	return schedule();
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

	ktimer_init(&current->t_alarm, alrm_action, current, TF_STATIC);
	ktimer_start(&current->t_alarm, ticks);
	return rc;
}

long sys_time(time_t *t)
{
	if (t != NULL) {
		if (vm_verify(&current->mm, t, sizeof (*t), VM_WRITE))
			return -EFAULT;
		*t = system_clock;
	}

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
	if (vm_verify(&current->mm, res, sizeof(*res), VM_WRITE))
		return -EFAULT;
	*res = posix_clocks[clockid].res;
	return 0;
}

long sys_clock_gettime(clockid_t clockid, struct timespec *tp)
{
	if (!clock_valid(clockid))
		return -EINVAL;
	if (vm_verify(&current->mm, tp, sizeof(*tp), VM_WRITE))
		return -EFAULT;
	posix_clocks[clockid].get(tp);
	return 0;
}

long sys_clock_settime(clockid_t clockid, struct timespec *tp)
{
	if (!clock_valid(clockid))
		return -EINVAL;
	if (posix_clocks[clockid].set == NULL)
		return -EPERM;
	if (vm_verify(&current->mm, tp, sizeof(*tp), 0))
		return -EFAULT;
	posix_clocks[clockid].set(tp);
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

	current->sig.infos[pt->sev.sigev_signo].si_value =
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
	if (sevp && vm_verify(&current->mm, sevp, sizeof(*sevp), 0))
		return -EFAULT;
	if (vm_verify(&current->mm, timerid, sizeof(*timerid), VM_WRITE))
		return -EFAULT;
	if ((pt = get_posix_timer()) == NULL)
		return -ENOMEM;

	pt->pid = current->pid;
	pt->timerid = current->posix_timer_id++;

	if (sevp == NULL) {
		pt->sev.sigev_notify = SIGEV_SIGNAL;
		pt->sev.sigev_signo = SIGALRM;
		pt->sev.sigev_value.sigval_int = pt->timerid;
	} else {
		pt->sev = *sevp;
		if (!sigev_valid(&pt->sev)) {
			error = -EINVAL;
			goto err0;
		}
	}

	ktimer_init(&pt->timer, posix_timer_action, pt, TF_STATIC);

	list_add_tail(&pt->chain, &current->posix_timers);
	hash_add(posix_timers, &pt->t_hash, pt->timerid);

	*timerid = pt->timerid;
	return 0;
err0:
	free_posix_timer(pt);
	return error;
}

long sys_timer_delete(timer_t timerid)
{
	struct posix_timer *pt = get_timer_by_id(timerid);

	if (pt == NULL)
		return -EINVAL;

	list_del(&pt->chain);
	free_posix_timer(pt);
	hash_del(&pt->t_hash);

	return 0;
}

long sys_timer_gettime(timer_t timerid, struct itimerspec *curr_value)
{
	unsigned long ticks;
	struct posix_timer *pt = get_timer_by_id(timerid);

	if (pt == NULL)
		return -EINVAL;
	if (vm_verify(&current->mm, curr_value, sizeof(*curr_value), VM_WRITE))
		return -EFAULT;

	ticks = pt->timer.expires - tick_count;
	pt->spec.it_value.tv_sec = (ticks % 100) ? ticks/100 + 1 : ticks/100;
	pt->spec.it_value.tv_nsec = 0;

	*curr_value = pt->spec;
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
	if (vm_verify(&current->mm, new_value, sizeof(*new_value), 0))
		return -EFAULT;

	*v = *new_value;
	ktimer_start(&pt->timer, __timespec_to_ticks(&v->it_value));
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
	pic_eoi();
	ready(current);
	schedule();
}
