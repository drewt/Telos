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

#define KERNEL_END \
	(PAGE_ALIGN((ulong)&_kend))

/* free list for kernel heap */
static LIST_HEAD(free_list);

/* Multiboot info structure */
static struct multiboot_info mb_info;

/* ELF section headers */
static struct elf32_shdr elf_shtab[32];
static char elf_strtab[512];

/*-----------------------------------------------------------------------------
 * Initializes the memory system */
//-----------------------------------------------------------------------------
unsigned long mem_init(struct multiboot_info **info)
{
	struct elf32_shdr *str_hdr;
	struct mem_header *heap;

	memcpy(&mb_info, *info, sizeof(struct multiboot_info));
	*info = &mb_info;

	/* copy ELF section headers & string table into kernel memory */
	memcpy(elf_shtab, (char*) mb_info.elf_sec.addr,
			mb_info.elf_sec.num * mb_info.elf_sec.size);
	str_hdr = &elf_shtab[mb_info.elf_sec.shndx];
	memcpy(elf_strtab, (char*) str_hdr->sh_addr, str_hdr->sh_size);

	if (!MULTIBOOT_MEM_VALID(&mb_info)) {
		wprints("failed to detect memory limits; assuming 8MB total");
		mb_info.mem_upper = 0x800000;
	}

	heap = (void*) KERNEL_END;
	heap->size = ((ulong)&KERNEL_PAGE_OFFSET + 0x00400000) - KERNEL_END;
	heap->magic = MAGIC_FREE;
	list_add(&heap->chain, &free_list);

	paging_init(0x00400000, PAGE_BASE(MULTIBOOT_MEM_MAX(&mb_info)));

	return MULTIBOOT_MEM_MAX(&mb_info) - KERNEL_TO_PHYS(KERNEL_END);
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns */
//-----------------------------------------------------------------------------
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

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list, given the
 * header for the segment */
//-----------------------------------------------------------------------------
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
