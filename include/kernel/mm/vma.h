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

#ifndef _KERNEL_MM_VMA_H_
#define _KERNEL_MM_VMA_H_

#include <kernel/list.h>

enum {
	VM_WRITE   = 0x1,  /* write permission */
	VM_EXEC    = 0x2,  /* execute permission */
	VM_COW     = 0x4,  /* copy-on-write */
	VM_ZERO    = 0x8,  /* area should be initialized with zeros */
	VM_NOALLOC = 0x10, /* pages already allocated and mapped */
};

struct vma {
	struct list_head chain;
	struct mm_struct *mmap;
	ulong start;
	ulong end;
	ulong flags;
};

struct mm_struct {
	pmap_t pgdir;
	struct list_head map;
	struct list_head kheap;
	struct vma *heap;
	struct vma *stack;
	struct vma *kernel_stack;
	ulong  brk;
};

int mm_init(struct mm_struct *mm);
int mm_kinit(struct mm_struct *mm);
void mm_fini(struct mm_struct *mm);
int mm_clone(struct mm_struct *dst, struct mm_struct *src);

struct vma *vma_find(const struct mm_struct *mm, const void *addr);
struct vma *vma_map(struct mm_struct *mm, ulong dst, size_t len, ulong flags);
int vma_grow_up(struct vma *vma, size_t amount, ulong flags);

static inline bool vma_contains(struct vma *vma, const void *addr)
{
	return (ulong)addr >= vma->start && (ulong)addr < vma->end;
}

static inline struct vma *vma_get(const struct mm_struct *mm, const void *addr)
{
	struct vma *vma = vma_find(mm, addr);
	return (vma && vma_contains(vma, addr)) ? vma : NULL;
}

static inline size_t vma_size(const struct vma *vma)
{
	return vma->end - vma->start;
}

int vm_verify(const struct mm_struct *mm, const void *start, size_t len,
		ulong flags);
int vm_copy_from(const struct mm_struct *mm, void *dst, const void *src,
		size_t len);
int vm_copy_to(const struct mm_struct *mm, void *dst, const void *src,
		size_t len);
int vm_copy_through(const struct mm_struct *dst_mm,
		const struct mm_struct *src_mm, void *dst, const void *src,
		size_t len);

#endif
