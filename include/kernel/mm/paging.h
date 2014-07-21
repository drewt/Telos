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

/* page frame info */
struct pf_info {
	struct list_head chain;
	unsigned long addr;
	unsigned ref;
};

struct pf_info *kalloc_frame(void);
struct pf_info *kzalloc_frame(void);
void *kalloc_pages(uint n);
void kfree_pages(void *addr, uint n);
void _kfree_frame(struct pf_info *page);
void *kmap_tmp_page(ulong addr);
void kunmap_page(void *addr);
int paging_init(unsigned long start, unsigned long end);
pmap_t new_pgdir(void);
int del_pgdir(pmap_t phys_pgdir);
int map_zpages(pmap_t pgdir, ulong dst, unsigned pages, uchar attr);

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

#define page_align(n) align_up(n, FRAME_SIZE)
#define page_base(n)  align_down(n, FRAME_SIZE)

#endif
