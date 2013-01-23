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

static int delta_list_rm (struct pcb **head, int evno, struct pcb *p);
static void delta_list_tick (struct pcb **head, int evno);
static void delta_list_insert (struct pcb **head, unsigned int evno,
        struct pcb *p, unsigned int ms, void (*act)(struct pcb*));

/* the sleeping queue is a delta list */
static struct pcb *sq_head; // the head of the sleeping queue

unsigned int tick_count = 0;

static void wake_action (struct pcb *p) {
    p->rc = (p->events[EVENT_WAKE].delta > 0) ? p->events[EVENT_WAKE].delta :
                                                0;
    ready (p);
}

/*-----------------------------------------------------------------------------
 * Puts a process p on the queue of sleeping processes, to sleep for a given
 * number of milliseconds.
 *-----------------------------------------------------------------------------
 */
void sys_sleep (unsigned int milliseconds) {

    delta_list_insert (&sq_head, EVENT_WAKE, current, milliseconds, wake_action);
    new_process ();
}

/*-----------------------------------------------------------------------------
 * Removes a process from the sleeping queue, returning the total number of 
 * ticks left to sleep */
//-----------------------------------------------------------------------------
int sq_rm (struct pcb *p) {
    return delta_list_rm (&sq_head, EVENT_WAKE, p);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
static void delta_list_insert (struct pcb **head, unsigned int evno,
        struct pcb *p, unsigned int ms, void (*act)(struct pcb*)) {

    int ticks;
    struct pcb *it, *last;

    // convert milliseconds to timer intervals
    ticks = (ms % 10) ? ms/10 + 1 : ms/10;
    p->events[evno].action = act;

    // empty list; p becomes the head
    if (!(*head)) {
        *head = p;
        p->events[evno].next = NULL;
        p->events[evno].prev = NULL;
        p->events[evno].delta = ticks;
    } else {
        // find p's place in the list, updating deltas along the way
        for (it = *head; it; it = it->events[evno].next) {
            if (ticks > it->events[evno].delta)
                ticks -= it->events[evno].delta;
            else
                break;
            last = it;
        }

        // if last is NULL, p goes at the head of the queue;
        // otherwise p goes between last and it
        if (!last) {
            p->events[evno].next = *head;
            *head = p;
        } else {
            last->events[evno].next = p;
            p->events[evno].next = it;
        }
        p->events[evno].prev = last;
        if (it)
            it->events[evno].delta -= ticks;
        p->events[evno].delta = ticks;
    }
}

static int delta_list_rm (struct pcb **head, int evno, struct pcb *p) {
    int ticks = 0;
    struct pcb *it;
    for (it = *head; it != p; it = it->events[evno].next)
        ticks += it->events[evno].delta;
    if (it == *head)
        *head = it->events[evno].next;
    else
        it->events[evno].prev->events[evno].next = it->events[evno].next;
    for (it = it->events[evno].next; it; it = it->events[evno].next)
        it->events[evno].delta += p->events[evno].delta;
    return ticks + p->events[evno].delta;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
static void delta_list_tick (struct pcb **head, int evno) {
    if (!(*head))
        return;

    struct pcb *tmp;
    (*head)->events[evno].delta--;
    while (*head && (*head)->events[evno].delta <= 0) {
        tmp   = *head;
        *head = (*head)->events[evno].next;
        tmp->events[evno].action (tmp);
    }
}

/*-----------------------------------------------------------------------------
 * Called on every timer interrupt; updates the queue of sleeping processes,
 * and more stuff to come... */
//-----------------------------------------------------------------------------
void tick (void) {

    tick_count++;

    delta_list_tick (&sq_head, EVENT_WAKE);

    // choose new process to run
    ready (current);
    new_process ();
    pic_eoi ();
}
