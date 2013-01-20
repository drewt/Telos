/* procqueue.c : process queues
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
#include <kernel/process.h>

void proc_initq (procqueue_t *queue) {
    queue->tail = NULL;
    queue->head = NULL;
    queue->size = 0;
}

void proc_enqueue (procqueue_t *queue, struct pcb *p) {
    if (queue->tail) {
        p->prev           = queue->tail;
        p->next           = NULL;
        queue->tail->next = p;
        queue->tail       = p;
    } else {
        p->next     = NULL;
        p->prev     = NULL;
        queue->head = p;
        queue->tail = p;
    }
    queue->size++;
}

struct pcb *proc_dequeue (procqueue_t *queue) {
    struct pcb *p;

    if (!queue->head)
        return NULL;

    p = queue->head;
    queue->head = queue->head->next;
    if (queue->head)
        queue->head->prev = NULL;
    else
        queue->tail = NULL;

    queue->size--;
    return p;
}

struct pcb *proc_peek (procqueue_t *queue) {
    return queue->head;
}

void proc_rm (procqueue_t *queue, struct pcb *p) {
    if (p == queue->head && p == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else if (p == queue->head) {
        queue->head = p->next;
        queue->head->prev = NULL;
    } else if (p == queue->tail) {
        queue->tail = p->prev;
        queue->tail->next = NULL;
    } else {
        p->prev->next = p->next;
        p->next->prev = p->prev;
    }
    queue->size--;
}
