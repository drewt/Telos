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
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/timer.h>
#include <signal.h>

unsigned int tick_count = 0; /* global tick count */

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
void sys_sleep(unsigned int seconds)
{
	current->t_sleep = timer_create(wake_action, current,
			TF_ALWAYS | TF_REF);
	if (current->t_sleep == NULL) {
		current->rc = -ENOMEM;
		return;
	}
	timer_start(current->t_sleep, seconds*100);
	new_process();
}

/*
 * Registers an alarm for the current process, to go off in the given number of
 * seconds.
 */
void sys_alarm(unsigned int seconds)
{
	if (current->t_alarm != NULL) {
		current->rc = timer_remove(current->t_alarm) / 10;
		timer_unref(current->t_alarm);
		current->t_alarm = NULL;
	} else {
		current->rc = 0;
	}

	if (seconds == 0)
		return;

	current->t_alarm = timer_create(alrm_action, current, TF_REF);
	if (current->t_alarm == NULL) {
		current->rc = -ENOMEM;
		return;
	}
	timer_start(current->t_alarm, seconds*100);
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
