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

#define kernel_to_phys(addr) ((uintptr_t) (addr) - kernel_base)
#define phys_to_kernel(addr) ((uintptr_t) (addr) + kernel_base)

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4
#define PE_A  0x20
#define PE_D  0x40

/* page frame info */
struct pf_info {
	struct list_head chain;
	uintptr_t addr;
	unsigned int ref;
};

struct vma;

struct pf_info *kalloc_frame(int flags);
void *kalloc_pages(unsigned int n);
void kfree_pages(void *addr, unsigned int n);
void _kfree_frame(struct pf_info *page);
void *kmap_tmp_page(uintptr_t addr);
void kunmap_tmp_page(void *addr);
void kunmap_page(void *addr);
void *kmap_tmp_range(uintptr_t pgdir, uintptr_t addr, size_t len, int flags);
void kunmap_tmp_range(void *addrp, size_t len);
int paging_init(uintptr_t start, uintptr_t end);
uintptr_t clone_pgdir(void);
uintptr_t new_pgdir(void);
int free_pgdir(uintptr_t phys_pgdir);
int map_pages(uintptr_t phys_pgdir, uintptr_t dst, unsigned int pages, int flags);
int map_frame(struct pf_info *frame, void *addr, int flags);
int map_page(void *addr, int flags);
int copy_page(void *addr, int flags);
int pm_unmap(struct vma *vma);
int pm_disable_write(struct vma *vma);
int pm_copy(struct vma *vma);
int pm_copy_to(uintptr_t phys_pgdir, void *dst, const void *src, size_t len);

static inline void kfree_frame(struct pf_info *frame)
{
	if (--frame->ref == 0)
		_kfree_frame(frame);
}

static inline unsigned int pages_in_range(uintptr_t addr, size_t len)
{
	return (((addr & 0xFFF) + (len - 1)) / FRAME_SIZE) + 1;
}

static inline uintptr_t align_up(uintptr_t addr, unsigned int align)
{
	unsigned mask = align - 1;
	return (addr & mask) ? (addr + align) & ~mask : addr;
}

static inline uintptr_t align_down(uintptr_t addr, unsigned int align)
{
	return addr & ~(align - 1);
}

static inline bool page_aligned(void *addr)
{
	return !((uintptr_t)addr & (FRAME_SIZE-1));
}

#define page_align(n) align_up((uintptr_t)n, FRAME_SIZE)
#define page_base(n)  align_down((uintptr_t)n, FRAME_SIZE)

#endif
