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

#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

/* mem_headers should align on 16 byte boundaries */
struct mem_header {
    uint32_t          size;         // size of an allocated block
    struct mem_header *prev;        // previous node in the free list
    struct mem_header *next;        // next node in the free list
    uint32_t          sanity_check; // padding/sanity check
    unsigned char data_start[0];    // start of allocated block
};

void kprintmem (void);
void *kmalloc (unsigned int size);
void *hmalloc (unsigned int size, struct mem_header **hdr);
void kfree (void *addr);
void hfree (struct mem_header *hdr);

#endif // __MEM_H_
