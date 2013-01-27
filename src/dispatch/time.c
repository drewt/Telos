/* sleep.c : sleep device
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
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
#include <signal.h>

static int delta_list_rm (int evno, struct pcb *p);
static void delta_list_tick (void);
static void delta_list_insert (unsigned int evno, struct pcb *p,
        unsigned int ms, void (*act)(struct pcb*));

#define N_EVENTS 2
enum event_types {
    EVENT_WAKE,
    EVENT_ALRM
};

/* Event structure.  When the event occurs, action(proc) will be called */
struct event {
    void (*action)(struct pcb*);
    int  delta;
    struct pcb *proc;
    struct event *next;
    struct event *prev;
};

/* the event queue */
static struct event events[PT_SIZE][N_EVENTS];
static struct event *dl_head = NULL;

unsigned int tick_count = 0; /* global tick count */

/*-----------------------------------------------------------------------------
 * Wakes a sleeping process */
//-----------------------------------------------------------------------------
static void wake_action (struct pcb *p) {
    p->rc = 0;
    ready (p);
}

/*-----------------------------------------------------------------------------
 * Sends SIGALRM to the process */
//-----------------------------------------------------------------------------
static void alrm_action (struct pcb *p) {
    sys_kill (p->pid, SIGALRM);
}

/*-----------------------------------------------------------------------------
 * Puts the current process to sleep for a given number of milliseconds */
//-----------------------------------------------------------------------------
void sys_sleep (unsigned int milliseconds) {
    delta_list_insert (EVENT_WAKE, current, milliseconds, wake_action);
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Registers an alarm for the current process, to go off in the given number of
 * seconds */
//-----------------------------------------------------------------------------
void sys_alarm (unsigned int seconds) {
    delta_list_insert (EVENT_ALRM, current, seconds * 100, alrm_action);
}

/*-----------------------------------------------------------------------------
 * Removes a process from the sleeping queue, returning the total number of 
 * ticks left to sleep */
//-----------------------------------------------------------------------------
int sq_rm (struct pcb *p) {
    return delta_list_rm (EVENT_WAKE, p);
}

/*-----------------------------------------------------------------------------
 * Schedule an event */
//-----------------------------------------------------------------------------
static void delta_list_insert (unsigned int evno, struct pcb *p,
        unsigned int ms, void (*act)(struct pcb*)) {

    int ticks;
    int pti = PT_INDEX(p->pid);
    struct event *it, *last = NULL;
    struct event *nev = &events[pti][evno];

    // convert milliseconds to timer intervals
    ticks = (ms % 10) ? ms/10 + 1 : ms/10;
    nev->proc   = p;
    nev->action = act;

    // empty list; p becomes the head
    if (!dl_head) {
        dl_head = nev;
        nev->next = NULL;
        nev->prev = NULL;
    } else {
        // find p's place in the list, updating delta along the way
        for (it = dl_head; it; it = it->next) {
            if (ticks > it->delta)
                ticks -= it->delta;
            else
                break;
            last = it;
        }

        // if last is NULL, p goes at the head of the queue;
        // otherwise p goes between last and it
        if (!last) {
            nev->next = dl_head;
            dl_head = nev;
        } else {
            last->next = nev;
            nev->next = it;
        }
        nev->prev = last;
        if (it)
            it->delta -= ticks;
    }
    nev->delta = ticks;
}

/*-----------------------------------------------------------------------------
 * Remove an event from the event queue, returning the number of ticks left
 * before the event would have occurred */
//-----------------------------------------------------------------------------
static int delta_list_rm (int evno, struct pcb *p) {
    int ticks = 0;
    struct event *it;
    struct event *del = &events[PT_INDEX(p->pid)][evno];

    for (it = dl_head; it && it != del; it = it->next)
        ticks += it->delta;
    if (!it)
        return -1;
    if (it == dl_head) {
        dl_head = it->next;
        dl_head->prev = NULL;
    } else {
        it->prev->next = it->next;
        if (it->next)
            it->next->prev = it->prev;
    }
    for (it = it->next; it; it = it->next)
        it->delta += del->delta;
    return ticks + del->delta;
}

/*-----------------------------------------------------------------------------
 * Move the event queue one tick forward, and execute any events that become
 * pending */
//-----------------------------------------------------------------------------
static void delta_list_tick (void) {
    if (!dl_head)
        return;

    struct event *tmp;
    dl_head->delta--;
    while (dl_head && dl_head->delta <= 0) {
        tmp     = dl_head;
        dl_head = dl_head->next;
        dl_head->prev = NULL;
        tmp->action (tmp->proc);
    }
}

/*-----------------------------------------------------------------------------
 * Called on every timer interrupt; updates global tick count, the event queue,
 * and switches to another process */
//-----------------------------------------------------------------------------
void tick (void) {

    tick_count++;

    delta_list_tick ();

    // choose new process to run
    ready (current);
    new_process ();
    pic_eoi ();
}
