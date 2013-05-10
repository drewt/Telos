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
 * mem.c
 *
 * Simple memory manager for shared heap.
 *
 */

#include <kernel/common.h>
#include <kernel/list.h>
#include <kernel/mem.h>

/*
 * unsigned long PARAGRAPH_ALIGN (unsigned long a)
 *      Takes an address and rounds it up to the nearest paragraph boundary.
 */
#define PARAGRAPH_MASK (~(0xF))
#define PARAGRAPH_ALIGN(a) \
    ((a) & PARAGRAPH_MASK ? ((a) + 0x10) & PARAGRAPH_MASK : (a))

/*
 * unsigned long PAGE_ALIGN (unsigned long a)
 *      Takes an address and rounds it up to the nearest paragraph boundary.
 */
#define PAGE_MASK (~(0xFFF))
#define PAGE_ALIGN(a) \
    ((a) & PAGE_MASK ? ((a) + 0x1000) & PAGE_MASK : (a))

#define MAGIC_OK   0x600DC0DE
#define MAGIC_FREE 0xF2EEB10C

extern unsigned long kend; // start of free memory
static list_head_t free_list;

/*-----------------------------------------------------------------------------
 * Initializes the memory system */
//-----------------------------------------------------------------------------
void mem_init (void)
{
    // XXX: there is more free memory below this
    unsigned long freemem = PAGE_ALIGN ((unsigned long) &kend);
    struct mem_header *head = (struct mem_header*) freemem;
    head->size = 0x400000 - freemem; // TODO: don't assume 4MB
    head->magic = MAGIC_FREE;
    list_init (&free_list);
    list_insert_tail (&free_list, (list_entry_t) head);
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns */
//-----------------------------------------------------------------------------
void *hmalloc (unsigned int size, struct mem_header **hdr)
{
    struct mem_header *p, *r;

    size = PARAGRAPH_ALIGN (size);

    // find a large enough segment of free memory
    list_iterate (&free_list, p, struct mem_header*, chain) {
        if (p->size >= size)
            break;
    }

    // not enough memory
    if (list_end (&free_list, (list_entry_t) p))
        return NULL;



    // if p is a perfect fit...
    if (p->size - size <= sizeof (struct mem_header)) {
        p->magic = MAGIC_OK;
        list_remove (&free_list, (list_entry_t) p);
    } else {
        // split p into adjacent segments p and r
        r = (struct mem_header*) (p->data_start + size);
        *r = *p;

        r->size = p->size - size - sizeof (struct mem_header);
        p->size = size;
        r->magic = MAGIC_FREE;
        p->magic = MAGIC_OK;

        // replace p with r in the free list
        list_replace_entry ((list_entry_t) p, (list_entry_t) r);
    }

    if (hdr)
        *hdr = p;

    return p->data_start;
}

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list, given the
 * header for the segment */
//-----------------------------------------------------------------------------
void hfree (struct mem_header *hdr)
{
    // check that the supplied address is sane
    if (hdr->magic == MAGIC_FREE) {
        kprintf ("kfree(): detected double free\n");
        return;
    }
    if (hdr->magic != MAGIC_OK) {
        kprintf ("kfree(): detected double free or corruption\n");
        return;
    }

    // insert freed at the beginning of the free list
    list_insert_head (&free_list, (list_entry_t) hdr);
}
