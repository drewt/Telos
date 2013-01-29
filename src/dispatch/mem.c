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

#include <kernel/dispatch.h>
#include <kernel/mem.h>

#include <errnodefs.h>

#define MAX_ALLOC 0x4000

void sys_malloc (unsigned int size, void **p) {
    
    struct mem_header *h;

    // TODO: limit memory use in a more `global' way
    if (size > MAX_ALLOC) {
        current->rc = ENOMEM;
        return;
    }

    if (!(*p = hmalloc (size, &h))) {
        current->rc = ENOMEM;
        return;
    }

    // put h at head of heap_mem list
    h->prev = NULL;
    h->next = current->heap_mem;
    current->heap_mem = h;
    if (h->next)
        h->next->prev = h;

    current->rc = 0;
}

void sys_free (void *ptr) {

    struct mem_header *m = (struct mem_header*)
        ((unsigned long) ptr - sizeof (struct mem_header));

    // remove ptr from heap_mem list
    if (m == current->heap_mem) {
        current->heap_mem = current->heap_mem->next;
        if (current->heap_mem)
            current->heap_mem->prev = NULL;
    } else {
        m->prev->next = m->next;
        if (m->next)
            m->next->prev = m->prev;
    }

    kfree (ptr);
}
