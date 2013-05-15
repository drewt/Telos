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

/*
 * dispatch/mem.c
 *
 * System calls for memory allocation.  Currently these allocate memory in a
 * shared heap.  Eventually these should be replaced by a brk syscall and
 * user-space malloc routines.
 */

#include <kernel/common.h>
#include <kernel/dispatch.h>
#include <kernel/list.h>
#include <kernel/mem.h>

#define MAX_ALLOC 0x4000

void sys_malloc (unsigned int size, void **p)
{    
    struct mem_header *h;

    if (size == 0) {
        current->rc = -EINVAL;
        return;
    }

    // TODO: limit memory use in a more `global' way
    if (size > MAX_ALLOC) {
        current->rc = -ENOMEM;
        return;
    }

    if (!(*p = hmalloc (size, &h))) {
        current->rc = -ENOMEM;
        return;
    }

    // add h to list of allocated memory for current process
    list_insert_tail (&current->heap_mem, (list_entry_t) h);

    current->rc = 0;
}

void sys_free (void *ptr)
{
    struct mem_header *h = mem_ptoh (ptr);

    // XXX: unsafe!  Memory might be unallocated or allocated to another process
    list_remove (&current->heap_mem, (list_entry_t) h);
    hfree (h);
}

void sys_palloc (void **p)
{
    struct pf_info *page;

    if ((page = kalloc_page ()) == NULL) {
        current->rc = -ENOMEM;
        return;
    }

    list_insert_tail (&current->page_mem, (list_entry_t) page);

    *p = (void*) page->addr;
    current->rc = 0;
}
