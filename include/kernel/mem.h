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

#ifndef _KERNEL_MEM_H_
#define _KERNEL_MEM_H_

#include <kernel/list.h>

#define FRAME_POOL_ADDR 0x400000
#define FRAME_POOL_SIZE 0x400000
#define FRAME_POOL_END  (FRAME_POOL_ADDR+FRAME_POOL_SIZE)
#define FRAME_SIZE 4096
#define NR_FRAMES (FRAME_POOL_SIZE / FRAME_SIZE)

#define KERNEL_TO_PHYS(addr) \
	((ulong) (addr) - (ulong) &KERNEL_PAGE_OFFSET)

/*
 * unsigned long PAGE_ALIGN (unsigned long a)
 *	Takes an address and rounds it up to the nearest page boundary.
 */
#define PAGE_ALIGN(a) \
	((a) & 0xFFF ? ((a) + 0x1000) & ~0xFFF : (a))

/*
 * unsigned long PAGE_BASE (unsigned long a)
 *	Takes an address and returns the base address of the page frame it
 *	belongs to.
 */
#define PAGE_BASE(a) \
	((a) & ~0xFFF)

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4

extern unsigned long _kernel_pgd;
extern unsigned long _kernel_high_pgt;

/* linker variables */
extern unsigned long KERNEL_PAGE_OFFSET;
extern unsigned long _kstart;		// start of the kernel
extern unsigned long _kend;		// end of the kernel
extern unsigned long _ustart;		// start of user-space
extern unsigned long _uend;		// end of user-space
extern unsigned long _urostart;		// start of user-space read-only memory
extern unsigned long _uroend;		// end of user-space read-only memory
extern unsigned long _krostart;		// start of kernel read-only memory
extern unsigned long _kroend;		// end of kernel read-only memory

/* mem_headers should align on 16 byte boundaries */
struct mem_header {
	struct list_head	chain;	// chain for free/allocated lists
	unsigned long		size;	// size of an allocated block
	unsigned long		magic;	// padding/sanity check
	unsigned char		data[];	// start of allocated block
};

/* page frame info */
struct pf_info {
	struct list_head chain;
	unsigned long addr;
};

struct multiboot_info;

unsigned long mem_init(struct multiboot_info **info);
void *hmalloc(unsigned int size, struct mem_header **hdr);
void hfree(struct mem_header *hdr);
struct pf_info *kalloc_page(void);
void kfree_page(struct pf_info *page);
int paging_init(unsigned long start, unsigned long end);
pmap_t pgdir_create(struct list_head *page_list);
ulong virt_to_phys(pmap_t pgdir, ulong addr);
int map_pages(pmap_t pgdir, ulong start, int pages, uchar attr,
		struct list_head *page_list);

static inline struct mem_header *mem_ptoh(void *addr)
{
	return (struct mem_header*) addr - 1;
}

static inline void *kmalloc(unsigned int size)
{
	return hmalloc(size, NULL);
}

static inline void kfree(void *addr)
{
	hfree(mem_ptoh(addr));
}

/*
 * Defines page-at-a-time allocation functions for the given type.
 *
 * @name the name of the allocation function
 * @type the type of object to allocate
 * @list the &struct list_head to add newly allocated objects to
 * @member the name of the struct list_head within the struct
 */
#define DEFINE_ALLOCATOR(name, type, list, member)	\
static int __ ## name(void)				\
{							\
	type *__tmp;					\
	int __i;					\
							\
	if ((__tmp = kmalloc(FRAME_SIZE)) == NULL)	\
		return -1;				\
							\
	__i = FRAME_SIZE / sizeof(type);		\
	for (; __i > 0; __i--, __tmp++)			\
		list_enqueue(&__tmp->member, list);	\
							\
	return 0;					\
}							\
							\
type *name(void)					\
{							\
	if (list_empty(list) && __ ## name() == -1)	\
		return NULL;				\
	return list_dequeue(list, type, member);	\
}


#endif
