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

#define kernel_to_phys(addr) ((unsigned long) (addr) - kernel_base)
#define phys_to_kernel(addr) ((unsigned long) (addr) + kernel_base)

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4
#define PE_A  0x20
#define PE_D  0x40

/* page frame info */
struct pf_info {
	struct list_head chain;
	unsigned long addr;
	unsigned int ref;
};

struct vma;

struct pf_info *kalloc_frame(unsigned long flags);
void *kalloc_pages(unsigned int n);
void kfree_pages(void *addr, unsigned int n);
void _kfree_frame(struct pf_info *page);
void *kmap_tmp_page(unsigned long addr);
void kunmap_tmp_page(void *addr);
void kunmap_page(void *addr);
void *kmap_tmp_range(pmap_t pgdir, unsigned long addr, size_t len,
		unsigned long flags);
void kunmap_tmp_range(void *addrp, size_t len);
int paging_init(unsigned long start, unsigned long end);
pmap_t clone_pgdir(void);
pmap_t new_pgdir(void);
int free_pgdir(pmap_t phys_pgdir);
int map_pages(pmap_t phys_pgdir, unsigned long dst, unsigned int pages,
		unsigned long flags);
int map_frame(struct pf_info *frame, void *addr, unsigned long flags);
int map_page(void *addr, unsigned long flags);
int copy_page(void *addr, unsigned long flags);
int pm_unmap(struct vma *vma);
int pm_disable_write(struct vma *vma);
int pm_copy(struct vma *vma);
int pm_copy_to(pmap_t phys_pgdir, void *dst, const void *src, size_t len);

static inline void kfree_frame(struct pf_info *frame)
{
	if (--frame->ref == 0)
		_kfree_frame(frame);
}

static inline unsigned int pages_in_range(unsigned long addr, unsigned long len)
{
	return (((addr & 0xFFF) + (len - 1)) / FRAME_SIZE) + 1;
}

static inline unsigned long align_up(unsigned long addr, unsigned int align)
{
	unsigned mask = align - 1;
	return (addr & mask) ? (addr + align) & ~mask : addr;
}

static inline unsigned long align_down(unsigned long addr, unsigned int align)
{
	return addr & ~(align - 1);
}

static inline int page_aligned(void *addr)
{
	return !((unsigned long)addr & (FRAME_SIZE-1));
}

#define page_align(n) align_up((unsigned long)n, FRAME_SIZE)
#define page_base(n)  align_down((unsigned long)n, FRAME_SIZE)

#endif
