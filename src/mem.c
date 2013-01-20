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
#include <mem.h>

#define PARAGRAPH_MASK (~(0xF))
#define PAGE_MASK (~(0xFFF))

#define SANITY_OK   0x12
#define SANITY_FREE 0x0

/* mem_headers should align on 16 byte boundaries; beware of pointer size */
struct mem_header {
    uint32_t          size;         // size of an allocated block
    struct mem_header *prev;        // previous node in the free list
    struct mem_header *next;        // next node in the free list
    uint32_t          sanity_check; // padding/sanity check
    unsigned char data_start[0];    // start of allocated block
};

extern uint32_t kend;         // start of free memory
struct mem_header *free_list; // head of the free list

/*-----------------------------------------------------------------------------
 * Initializes the memory system */
//-----------------------------------------------------------------------------
void mem_init (void) {

    // XXX: bad!  There is more free memory below this
    uint32_t freemem = ((uint32_t) &kend + 0x1000) & PAGE_MASK;
    free_list = (struct mem_header*) freemem;
    free_list->size = 0x2F0000;
    free_list->next = NULL;
    free_list->prev = NULL;
    free_list->sanity_check = SANITY_FREE;
}

/* wrapper for paragraph aligned allocations */
void *kmalloc (uint32_t size) {
    return kmalloca (size, PARAGRAPH_MASK);
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, aligned with the given mask */
//-----------------------------------------------------------------------------
void *kmalloca (uint32_t size, uint32_t mask) {

    struct mem_header *p, *r;

    // don't waste space on empty mem_headers
    if (!size)
        return NULL;

    // round up to the nearest paragraph boundary
    if (size & 0xF)
        size = (size + 0x10) & mask;

    // find a large enough segment of free memory
    for (p = free_list; p && (p->size < size); p = p->next);

    if (!p) return NULL; // not enough memory

    // if p is just barely big enough...
    if ((p->size - size) < sizeof (struct mem_header)) {
        p->sanity_check = SANITY_OK;

        // remove p from the free list
        if (p->next)
            p->next->prev = p->prev;
        if (p->prev)
            p->prev->next = p->next;
        else
            free_list = p->next;
    } else {
        // split p into adjacent segments p and r
        r = (struct mem_header*) ((uint32_t)p+size+sizeof(struct mem_header));
        *r = *p;

        // set mem_header fields
        r->size = p->size - size - sizeof (struct mem_header);
        p->size = size;
        r->sanity_check = SANITY_FREE;
        p->sanity_check = SANITY_OK;

        // replace p with r in the free list
        if (p->next)
            p->next->prev = r;
        if (p->prev)
            p->prev->next = r;
        else
            free_list = r;
    }

    return &(p->data_start);
}

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list */
//-----------------------------------------------------------------------------
void kfree (void *addr) {
    
    struct mem_header *freed = (struct mem_header*) 
        ((uint32_t) addr - sizeof (struct mem_header));

    // check that the supplied address is sane
    if (freed->sanity_check != SANITY_OK) {
        kprintf ("kfree(): detected double free or corruption\n");
        return;
    }

    // insert freed at the beginning of the free list
    freed->next = free_list;
    freed->prev = NULL;
    if (free_list)
        free_list->prev = freed;
    free_list = freed;
}
