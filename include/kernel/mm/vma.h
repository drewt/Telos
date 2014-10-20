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

#define VM_READ  1
#define VM_WRITE 2
#define _VM_EXEC 4
#define _VM_COW  8

#define VM_EXEC (VM_READ | _VM_EXEC)
#define VM_COW  (VM_READ | VM_WRITE | _VM_COW)

#define VM_RW  (VM_READ | VM_WRITE)
#define VM_RWE (VM_READ | VM_WRITE | _VM_EXEC)

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
void mm_fini(struct mm_struct *mm);
int mm_clone(struct mm_struct *dst, struct mm_struct *src);

struct vma *vma_find(struct mm_struct *mm, void *addr);
struct vma *zmap(struct mm_struct *mm, void *dstp, size_t len, ulong flags);
int vma_grow_up(struct vma *vma, size_t amount, ulong flags);

static inline bool vma_contains(struct vma *vma, void *addr)
{
	return (ulong)addr > vma->start && (ulong)addr < vma->end;
}

#endif
