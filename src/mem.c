/* mem.c : memory allocation
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
#include <kernel/queue.h>
#include <kernel/mem.h>

#define PARAGRAPH_MASK (~(0xF))
#define PAGE_MASK (~(0xFFF))

#define SANITY_OK   0x12
#define SANITY_FREE 0x0

extern unsigned long kend; // start of free memory
static queue_head_t free_list;

/*-----------------------------------------------------------------------------
 * Initializes the memory system */
//-----------------------------------------------------------------------------
void mem_init (void) {

    // XXX: there is more free memory below this
    unsigned long freemem = ((unsigned long) &kend + 0x1000) & PAGE_MASK;
    struct mem_header *head = (struct mem_header*) freemem;
    head->size = 0x2F0000; // TODO: use a non-arbitrary value
    head->sanity_check = SANITY_FREE;
    queue_init (&free_list);
    enqueue (&free_list, (queue_entry_t) head);
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block */
//-----------------------------------------------------------------------------
void *kmalloc (unsigned int size) {
    return hmalloc (size, NULL);
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns */
//-----------------------------------------------------------------------------
void *hmalloc (unsigned int size, struct mem_header **hdr) {

    struct mem_header *p, *r;

    // don't waste space on empty mem_headers
    if (!size)
        return NULL;

    // round up to the nearest paragraph boundary
    if (size & 0xF)
        size = (size + 0x10) & PARAGRAPH_MASK;

    // find a large enough segment of free memory
    queue_iterate (&free_list, p, struct mem_header*, chain) {
        if (p->size >= size)
            break;
    }

    if (queue_end (&free_list, (queue_entry_t) p))
        return NULL;

    // if p is just barely big enough...
    if ((p->size - size) < sizeof (struct mem_header)) {
        p->sanity_check = SANITY_OK;

        // remove p from the free list
        remqueue (&free_list, (queue_entry_t) p);
    } else {
        // split p into adjacent segments p and r
        r = (struct mem_header*)
            ((unsigned long)p + size + sizeof (struct mem_header));
        *r = *p;

        // set mem_header fields
        r->size = p->size - size - sizeof (struct mem_header);
        p->size = size;
        r->sanity_check = SANITY_FREE;
        p->sanity_check = SANITY_OK;

        // replace p with r in the free list
        queue_replace_entry ((queue_entry_t) p, (queue_entry_t) r);
    }

    if (hdr)
        *hdr = p;
    return &(p->data_start);
}

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list, given a
 * pointer to the segment */
//-----------------------------------------------------------------------------
void kfree (void *addr) {
    // compute header location and pass it to hfree
    struct mem_header *h = (struct mem_header*) 
        ((unsigned long) addr - sizeof (struct mem_header));
    hfree (h);
}

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list, given the
 * header for the segment */
//-----------------------------------------------------------------------------
void hfree (struct mem_header *hdr) {
    
    // check that the supplied address is sane
    if (hdr->sanity_check != SANITY_OK) {
        kprintf ("kfree(): detected double free or corruption\n");
        return;
    }

    // insert freed at the beginning of the free list
    enqueue (&free_list, (queue_entry_t) hdr);
}
