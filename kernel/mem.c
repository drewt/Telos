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
#include <kernel/mem.h>

#include <string.h> /* memcpy */

/*
 * unsigned long PARAGRAPH_ALIGN(unsigned long a)
 *      Takes an address and rounds it up to the nearest paragraph boundary.
 */
#define PARAGRAPH_ALIGN(a) \
	((a) & 0xF ? ((a) + 0x10) & ~0xF : (a))

/* magic numbers for kernel heap headers */
#define MAGIC_OK   0x600DC0DE
#define MAGIC_FREE 0xF2EEB10C

/* free list for kernel heap */
static LIST_HEAD(free_list);

static inline ulong MAX(ulong a, ulong b)
{
	return a > b ? a : b;
}

/*
 * XXX: assumes bootloader puts modules, etc. in sane places (i.e. reasonably
 *      close to the kernel).  If it puts them somewhere dumb, like the end of
 *      memory, this will fail catastrophically.
 */
static ulong get_heap_start(struct multiboot_info *info)
{
	ulong max = PAGE_ALIGN((ulong)&_kend);

	if (MULTIBOOT_MODS_VALID(info)) {
		struct multiboot_mod_list *mods = (void*) info->mods_addr;
		max = MAX(max, info->mods_addr);

		for (unsigned i = 0; i < info->mods_count; i++) {
			max = MAX(max, mods[i].end);
		}
	}

	if (MULTIBOOT_ELFSEC_VALID(info)) {
		struct elf_section_header_table *tab = (void*) &info->elf_sec;
		max = MAX(max, tab->addr + tab->num * tab->size);
	}

	if (MULTIBOOT_MMAP_VALID(info))
		max = MAX(max, info->mmap_addr + info->mmap_length);

	return PAGE_ALIGN(max);
}

/*
 * Returns a 4MB aligned address, at least 1MB after start.
 */
static ulong get_heap_end(ulong start)
{
	return (start + 0x00500000) & 0xFFC00000;
}

/*
 * Translates physical addresses in the multiboot_info structure to kernel
 * (virtual) addresses.
 */
static void fix_multiboot_info(struct multiboot_info *info)
{
	if (MULTIBOOT_MODS_VALID(info)) {
		struct multiboot_mod_list *mods;
		info->mods_addr = PHYS_TO_KERNEL(info->mods_addr);
		mods = (void*) info->mods_addr;

		for (unsigned i = 0; i < info->mods_count; i++) {
			mods[i].start = PHYS_TO_KERNEL(mods[i].start);
			mods[i].end = PHYS_TO_KERNEL(mods[i].end);
		}
	}

	if (MULTIBOOT_ELFSEC_VALID(info))
		info->elf_sec.addr = PHYS_TO_KERNEL(info->elf_sec.addr);

	if (MULTIBOOT_MMAP_VALID(info))
		info->mmap_addr = PHYS_TO_KERNEL(info->mmap_addr);
}

unsigned long mem_init(struct multiboot_info **info)
{
	struct mem_header *heap;
	ulong heap_start;

	*info = (void*) PHYS_TO_KERNEL(*info);

	fix_multiboot_info(*info);
	if (!MULTIBOOT_MEM_VALID(*info)) {
		wprints("failed to detect memory limits; assuming 8MB total");
		(*info)->mem_upper = 0x800000;
	}

	heap_start = get_heap_start(*info);
	heap = (void*) heap_start;
	heap->size = get_heap_end(heap_start) - heap_start;
	heap->magic = MAGIC_FREE;
	list_add(&heap->chain, &free_list);

	paging_init(0x00400000, PAGE_BASE(MULTIBOOT_MEM_MAX(*info)));

	return MULTIBOOT_MEM_MAX(*info) - KERNEL_TO_PHYS(heap_start);
}

/*
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns.
 */
void *hmalloc(unsigned int size, struct mem_header **hdr)
{
	struct mem_header *p, *r;
	struct list_head *it;

	size = PARAGRAPH_ALIGN(size);

	/* find a large enough segment of free memory */
	list_for_each(it, &free_list) {
		p = (struct mem_header*) it;
		if (p->size >= size)
			break;
	}

	/* not enough memory */
	if (&p->chain == &free_list)
		return NULL;

	/* if p is a perfect fit... */
	if (p->size - size <= sizeof(struct mem_header)) {
		p->magic = MAGIC_OK;
		list_del(&p->chain);
	} else {
		/* split p into adjacent segments p and r */
		r = (struct mem_header*) (p->data + size);
		*r = *p;

		r->size = p->size - size - sizeof(struct mem_header);
		p->size = size;
		r->magic = MAGIC_FREE;
		p->magic = MAGIC_OK;

		/* replace p with r in the free list */
		list_replace(&p->chain, &r->chain);
	}

	if (hdr)
		*hdr = p;

	return p->data;
}

/* Returns a segment of previously allocated memory to the free list, given the
 * header for the segment.
 */
void hfree(struct mem_header *hdr)
{
	/* check that the supplied address is sane */
	if (hdr->magic == MAGIC_FREE) {
		kprintf("kfree(): detected double free\n");
		return;
	}
	if (hdr->magic != MAGIC_OK) {
		kprintf("kfree(): detected double free or corruption\n");
		return;
	}

	/* insert freed at the beginning of the free list */
	list_add(&hdr->chain, &free_list);
}
