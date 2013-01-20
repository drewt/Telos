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

/* the sleeping queue is a delta list */
static struct pcb *sq_head; // the head of the sleeping queue

unsigned int tick_count = 0;

/*-----------------------------------------------------------------------------
 * Puts a process p on the queue of sleeping processes, to sleep for a given
 * number of milliseconds.
 *-----------------------------------------------------------------------------
 */
void sys_sleep (unsigned int milliseconds) {

    // compute the number of timer intervals to sleep
    int ticks = (milliseconds%10)? milliseconds/10 + 1 : milliseconds/10;

    // if the head of the sleeping queue is NULL, p becomes the head
    if (!sq_head) {
        sq_head        = current;
        sq_head->next  = NULL;
        sq_head->prev  = NULL;
        current->sleep_delta = ticks;
        new_process ();
        return;
    }
    
    // find p's place in the queue, updating delta along the way
    struct pcb *it, *last = 0;
    for (it = sq_head; it; it = it->next) {
        if ( ticks > it->sleep_delta )
            ticks -= it->sleep_delta;
        else break;
        last = it;
    }

    // if last is NULL, p goes at the head of the queue;
    // otherwise p goes between last and it
    if (!last) {
        current->next = sq_head;
        sq_head = current;
    } else {
        last->next = current;
        current->next = it;
    }
    current->prev = last;

    if (it) 
        it->sleep_delta -= ticks;
    current->sleep_delta = ticks;
    current->state = STATE_SLEEPING;

    new_process ();
}

/*-----------------------------------------------------------------------------
 * Removes a process from the sleeping queue, returning the total number of 
 * ticks left to sleep */
//-----------------------------------------------------------------------------
int sq_rm (struct pcb *p) {
    int ticks = 0;
    struct pcb *it;
    for (it = sq_head; it != p; it = it->next)
        ticks += it->sleep_delta;
    if (it == sq_head)
        sq_head = it->next;
    else
        it->prev->next = it->next;
    for (it = it->next; it; it = it->next)
        it->sleep_delta += p->sleep_delta;
    return ticks + p->sleep_delta;
}

/*-----------------------------------------------------------------------------
 * Called on every timer interrupt; updates the queue of sleeping processes,
 * and more stuff to come... */
//-----------------------------------------------------------------------------
void tick (void) {

    tick_count++;

    if (sq_head) {
        struct pcb *tmp;
        sq_head->sleep_delta--;

        // wake all processes that are ready to resume
        while (sq_head && sq_head->sleep_delta <= 0) {
            tmp      = sq_head;
            sq_head  = sq_head->next;
            tmp->rc = 0;
            ready (tmp);
        }
    }

    ready (current);
    new_process ();
    pic_eoi ();
}
