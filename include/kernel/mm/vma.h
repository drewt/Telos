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

/*
 * The first three flags are the familar mode bits; these should not be changed
 * as mmap, etc. assume they have the usual meaning.
 */
enum {
	VM_EXEC    = 0x1,  /* execute permission */
	VM_WRITE   = 0x2,  /* write permission */
	VM_READ    = 0x4,  /* read permission */
	VM_ZERO    = 0x8,  /* area should be initialized with zeros */
	VM_KEEP    = 0x10, /* never unmap */
	VM_CLOEXEC = 0x20, /* unmap on exec */
	VM_SHARE   = 0x40, /* share across fork */
};

struct vma_operations;

struct vma {
	struct list_head chain;
	struct mm_struct *mmap;
	uintptr_t start;
	uintptr_t end;
	int flags;
	void *private;
	struct vma_operations *op;
};

struct vma_operations {
	int (*map)(struct vma *, void *);
	int (*writeback)(struct vma *, void *, size_t);
	int (*read_perm)(struct vma *, void *);
	int (*write_perm)(struct vma *, void *);
	int (*unmap)(struct vma *);
	int (*clone)(struct vma *, struct vma *);
	int (*split)(struct vma *, struct vma *);
};

struct mm_struct {
	uintptr_t pgdir;
	struct list_head map;
	struct list_head kheap;
	uintptr_t brk;
};

int mm_init(struct mm_struct *mm);
void mm_fini(struct mm_struct *mm);
int mm_clone(struct mm_struct *dst, struct mm_struct *src);

struct vma *create_vma(struct mm_struct *mm, uintptr_t start, uintptr_t end,
		size_t len, int flags);
struct vma *create_vma_high(struct mm_struct *mm, uintptr_t start, uintptr_t end,
		size_t len, int flags);
struct vma *vma_find(const struct mm_struct *mm, const void *addr);
struct vma *vma_map(struct mm_struct *mm, uintptr_t dst, size_t len, int flags);
int vma_grow_up(struct vma *vma, size_t amount, int flags);
int vma_bisect(struct vma *vma, uintptr_t split, int lflags, int rflags);

static inline bool vma_contains(struct vma *vma, const void *addr)
{
	return (uintptr_t)addr >= vma->start && (uintptr_t)addr < vma->end;
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

int vm_map_page(struct vma *vma, void *addr);
int vm_writeback(struct vma *vma, void *addr, size_t len);
int vm_read_perm(struct vma *vma, void *addr);
int vm_write_perm(struct vma *vma, void *addr);
int vm_unmap(struct vma *vma);
int vm_clone(struct vma *dst, struct vma *src);
int vm_split(struct vma *new, struct vma *old);

int vm_verify(const struct mm_struct *mm, const void *start, size_t len,
		int flags);
int vm_verify_unmapped(const struct mm_struct *mm, const void *start,
		size_t len);
int vm_copy_from(const struct mm_struct *mm, void *dst, const void *src,
		size_t len);
int vm_copy_to(const struct mm_struct *mm, void *dst, const void *src,
		size_t len);
int vm_copy_through(const struct mm_struct *dst_mm,
		const struct mm_struct *src_mm, void *dst, const void *src,
		size_t len);

#endif
