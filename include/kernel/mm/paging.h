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

#ifndef _KERNEL_MM_PAGING_H_
#define _KERNEL_MM_PAGING_H_

#include <kernel/process.h>

#define FRAME_SIZE 4096

#define kernel_to_phys(addr) \
	((ulong) (addr) - (ulong) &KERNEL_PAGE_OFFSET)

#define phys_to_kernel(addr) \
	((ulong) (addr) + (ulong) &KERNEL_PAGE_OFFSET)

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4
#define PE_A  0x20
#define PE_D  0x40

/* page frame info */
struct pf_info {
	struct list_head chain;
	unsigned long addr;
	unsigned ref;
};

struct vma;

struct pf_info *kalloc_frame(ulong flags);
void *kalloc_pages(uint n);
void kfree_pages(void *addr, uint n);
void _kfree_frame(struct pf_info *page);
void *kmap_tmp_page(ulong addr);
void kunmap_tmp_page(void *addr);
void kunmap_page(void *addr);
void *kmap_tmp_range(pmap_t pgdir, ulong addr, size_t len, ulong flags);
void kunmap_tmp_range(void *addrp, size_t len);
int paging_init(unsigned long start, unsigned long end);
pmap_t clone_pgdir(void);
pmap_t new_pgdir(void);
int del_pgdir(pmap_t phys_pgdir);
int map_pages(pmap_t phys_pgdir, ulong dst, unsigned pages, ulong flags);
int map_frame(struct pf_info *frame, void *addr, ulong flags);
int map_page(void *addr, ulong flags);
int pm_unmap(struct vma *vma);

static inline void kfree_frame(struct pf_info *frame)
{
	if (--frame->ref == 0)
		_kfree_frame(frame);
}

static inline unsigned pages_in_range(ulong addr, ulong len)
{
	return (((addr & 0xFFF) + (len - 1)) / FRAME_SIZE) + 1;
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

static inline int page_aligned(void *addr)
{
	return !((ulong)addr & (FRAME_SIZE-1));
}

#define page_align(n) align_up((ulong)n, FRAME_SIZE)
#define page_base(n)  align_down((ulong)n, FRAME_SIZE)

#endif
