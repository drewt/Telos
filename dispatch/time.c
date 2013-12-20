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

struct posix_timer {
	struct list_head chain;
	struct hlist_node t_hash;
	struct sigevent sev;
	struct itimerspec spec;
	struct timer *timer;
	pid_t pid;
	timer_t timerid;
};

LIST_HEAD(free_timers);
DEFINE_HASHTABLE(posix_timers, 9);

unsigned int tick_count = 0; /* global tick count */

DEFINE_ALLOCATOR(get_posix_timer, struct posix_timer, &free_timers, chain)

/*
 * Wakes a sleeping process.
 */
static void wake_action(void *data)
{
	struct pcb *p = (struct pcb*) data;
	p->rc = 0;
	ready(p);

	timer_unref(p->t_sleep);
	p->t_sleep = NULL;
}

/*
 * Sends SIGALRM to the process.
 */
static void alrm_action(void *data)
{
	struct pcb *p = (struct pcb*) data;
	__kill(p, SIGALRM);

	timer_unref(p->t_alarm);
	p->t_alarm = NULL;
}

/*
 * Puts the current process to sleep for a given number of milliseconds.
 */
long sys_sleep(unsigned int seconds)
{
	current->t_sleep = timer_create(wake_action, current,
			TF_ALWAYS | TF_REF);
	if (current->t_sleep == NULL)
		return -ENOMEM;
	timer_start(current->t_sleep, seconds*100);
	new_process();
	return 0;
}

/*
 * Registers an alarm for the current process, to go off in the given number of
 * seconds.
 */
long sys_alarm(unsigned int seconds)
{
	long rc;

	if (current->t_alarm != NULL) {
		rc = timer_remove(current->t_alarm) / 10;
		timer_unref(current->t_alarm);
		current->t_alarm = NULL;
	} else {
		rc = 0;
	}

	if (seconds == 0)
		return rc;

	current->t_alarm = timer_create(alrm_action, current, TF_REF);
	if (current->t_alarm == NULL)
		return -ENOMEM;
	timer_start(current->t_alarm, seconds*100);
	return rc;
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

	__kill(p, pt->sev.sigev_signo);
}

long sys_timer_create(clockid_t clockid, struct sigevent *sevp,
		timer_t *timerid)
{
	long error;
	struct posix_timer *pt;

	if (clockid != CLOCK_REALTIME)
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
		if (pt->sev.sigev_notify != SIGEV_SIGNAL) {
			error = -ENOTSUP;
			goto err0;
		}
	}

	pt->timer = timer_create(posix_timer_action, pt, TF_REF);
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
	return -ENOTSUP;
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

	timer_start(pt->timer, v->it_value.tv_sec*100 + v->it_value.tv_nsec);
	return 0;
}

/*
 * Called on every timer interrupt; updates global tick count, the event queue,
 * and switches to another process
 */
void tick(void)
{
	tick_count++;

	timers_tick();

	/* choose new process to run */
	ready(current);
	new_process();
	pic_eoi();
}
