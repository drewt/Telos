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

#include <kernel/process.h>
#include <kernel/list.h>

#define FRAME_SIZE 4096

#define kernel_to_phys(addr) \
	((ulong) (addr) - (ulong) &KERNEL_PAGE_OFFSET)

#define phys_to_kernel(addr) \
	((ulong) (addr) + (ulong) &KERNEL_PAGE_OFFSET)

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4

extern pte_t _kernel_pgd;
extern unsigned long _kernel_high_pgt;

/* linker variables */
extern ulong KERNEL_PAGE_OFFSET;
extern ulong _kstart;   // start of the kernel
extern ulong _kend;     // end of the kernel
extern ulong _ustart;   // start of user-space
extern ulong _uend;     // end of user-space
extern ulong _urostart; // start of user-space read-only memory
extern ulong _uroend;   // end of user-space read-only memory
extern ulong _krostart; // start of kernel read-only memory
extern ulong _kroend;   // end of kernel read-only memory

#define kstart   ((ulong)&_kstart)
#define kend     ((ulong)&_kend)
#define ustart   ((ulong)&_ustart)
#define uend     ((ulong)&_uend)
#define urostart ((ulong)&_urostart)
#define uroend   ((ulong)&_uroend)
#define krostart ((ulong)&_krostart)
#define kroend   ((ulong)&_kroend)

extern ulong kheap_start;
extern ulong kheap_end;

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
struct pf_info *kalloc_frame(void);
struct pf_info *kzalloc_frame(void);
void kfree_frame(struct pf_info *page);
int paging_init(unsigned long start, unsigned long end);
int address_space_init(struct pcb *p);
int map_pages(pmap_t pgdir, ulong start, int pages, uchar attr,
		struct list_head *page_list);

#define map_pages_user(p, start, pages, attr) \
	map_pages((p)->pgdir, start, pages, attr, &(p)->page_mem)

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

static inline ulong align_up(ulong addr, unsigned align)
{
	unsigned mask = align - 1;
	return (addr & mask) ? (addr + align) & ~mask : addr;
}

static inline ulong align_down(ulong addr, unsigned align)
{
	return addr & ~(align - 1);
}

#define page_align(n) align_up(n, FRAME_SIZE)
#define page_base(n)  align_down(n, FRAME_SIZE)

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
