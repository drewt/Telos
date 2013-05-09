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
#include <kernel/dispatch.h>
#include <kernel/queue.h>
#include <kernel/mem.h>

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
    enqueue (&current->heap_mem, (queue_entry_t) h);

    current->rc = 0;
}

void sys_free (void *ptr) {

    struct mem_header *m = (struct mem_header*)
        ((unsigned long) ptr - sizeof (struct mem_header));

    remqueue (&current->heap_mem, (queue_entry_t) m);
    kfree (ptr);
}
