/*  Copyright 2013 Drew Thoreson
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

#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <kernel/list.h>
#include <kernel/mmap.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>

#include <string.h> /* memcpy */

/* magic numbers for kernel heap headers */
#define MAGIC_OK   0x600DC0DE
#define MAGIC_FREE 0xF2EEB10C

/* free list for kernel heap */
static LIST_HEAD(free_list);

static unsigned pages_needed(size_t size)
{
	return align_up(size+sizeof(struct mem_header), FRAME_SIZE) / FRAME_SIZE;
}

/*
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns.
 */
void *hmalloc(size_t size, struct mem_header **hdr)
{
	struct mem_header *p, *r;

	size = align_up(size, 16);

	// find a large enough segment of free memory
	list_for_each_entry(p, &free_list, chain) {
		if (p->size >= size)
			break;
	}

	// not enough memory
	if (&p->chain == &free_list) {
		unsigned pages = pages_needed(size);
		p = kalloc_pages(pages);
		p->size = pages*FRAME_SIZE - sizeof(struct mem_header);
		list_add(&p->chain, &free_list);
	}

	// if p is a perfect fit...
	if (p->size - size <= sizeof(struct mem_header) + 16) {
		p->magic = MAGIC_OK;
		list_del(&p->chain);
	} else {
		// split p into adjacent segments p and r
		r = (struct mem_header*) (p->data + size);
		*r = *p;

		r->size = p->size - size - sizeof(struct mem_header);
		p->size = size;
		r->magic = MAGIC_FREE;
		p->magic = MAGIC_OK;

		// replace p with r in the free list
		list_replace(&p->chain, &r->chain);
	}

	if (hdr)
		*hdr = p;

	return p->data;
}

/*
 * Returns a segment of previously allocated memory to the free list, given the
 * header for the segment.
 */
void hfree(struct mem_header *hdr)
{
	// check that the supplied address is sane
	if (hdr->magic == MAGIC_FREE) {
		kprintf("kfree(): detected double free\n");
		return;
	}
	if (hdr->magic != MAGIC_OK) {
		kprintf("kfree(): detected double free or corruption\n");
		return;
	}

	// insert freed at the beginning of the free list
	list_add(&hdr->chain, &free_list);
}
