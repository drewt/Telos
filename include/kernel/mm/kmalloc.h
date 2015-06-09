/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KERNEL_MM_KMALLOC_H_
#define _KERNEL_MM_KMALLOC_H_

#include <kernel/list.h>

/* mem_headers should align on 16 byte boundaries */
struct mem_header {
	struct list_head	chain;	// chain for free/allocated lists
	unsigned long		size;	// size of an allocated block
	unsigned long		magic;	// padding/sanity check
	unsigned char		data[];	// start of allocated block
};

void *hmalloc(size_t size, struct mem_header **hdr);
void hfree(struct mem_header *hdr);

static inline struct mem_header *mem_ptoh(void *addr)
{
	return (struct mem_header*) addr - 1;
}

static inline void *kmalloc(size_t size)
{
	return hmalloc(size, NULL);
}

static inline void kfree(void *addr)
{
	hfree(mem_ptoh(addr));
}

#endif
