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

#include <kernel/list.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <kernel/mm/vma.h>

static DEFINE_SLAB_CACHE(area_cachep, sizeof(struct vma));

#define alloc_vma() slab_alloc(area_cachep)
#define free_vma(p) slab_free(area_cachep, p)

#define HEAP_START  0x00100000UL
#define STACK_START 0x0F000000UL
#define STACK_SIZE  (FRAME_SIZE*8)
#define KSTACK_START (STACK_START+STACK_SIZE)
#define KSTACK_SIZE (FRAME_SIZE*4)

#define HEAP_FLAGS   (VM_WRITE | VM_ZERO)
#define USTACK_FLAGS (VM_WRITE | VM_EXEC | VM_ZERO)
#define KSTACK_FLAGS USTACK_FLAGS

static struct vma *new_vma(ulong start, ulong end, ulong flags)
{
	struct vma *vma;
	if (!(vma = alloc_vma()))
		return NULL;
	vma->start = start;
	vma->end = end;
	vma->flags = flags;
	return vma;
}

int mm_init(struct mm_struct *mm)
{
	if (!(mm->pgdir = new_pgdir()))
		return -ENOMEM;

	mm->brk = HEAP_START + FRAME_SIZE;
	mm->heap = vma_map(mm, (void*)HEAP_START, FRAME_SIZE, HEAP_FLAGS);
	if (!mm->heap)
		goto abort;

	mm->stack = vma_map(mm, (void*)STACK_START, STACK_SIZE, USTACK_FLAGS);
	if (!mm->stack)
		goto abort;

	mm->kernel_stack = vma_map(mm, (void*)KSTACK_START, KSTACK_SIZE, KSTACK_FLAGS);
	if (!mm->kernel_stack)
		goto abort;

	return 0;
abort:
	del_pgdir(mm->pgdir);
	return -ENOMEM;
}

void mm_fini(struct mm_struct *mm)
{
	struct vma *vma, *n;
	list_for_each_entry_safe(vma, n, &mm->map, chain) {
		free_vma(vma);
	}
}

int mm_clone(struct mm_struct *dst, struct mm_struct *src)
{
	struct vma *vma;

	if (!(dst->pgdir = clone_pgdir()))
		return -ENOMEM;

	list_for_each_entry(vma, &src->map, chain) {
		struct vma *new = new_vma(vma->start, vma->end, vma->flags);
		new->mmap = dst;
		list_add_tail(&new->chain, &dst->map);
		if (vma == src->heap)
			dst->heap = new;
		else if (vma == src->stack)
			dst->stack = new;
		else if (vma == src->kernel_stack)
			dst->kernel_stack = new;
	}
	// TODO: dst->kheap
	dst->brk = src->brk;
	return 0;
}

struct vma *vma_find(struct mm_struct *mm, void *addr)
{
	struct vma *area;

	list_for_each_entry(area, &mm->map, chain) {
		if (area->end > (ulong)addr)
			return area;
	}
	return NULL;
}

int vma_grow_up(struct vma *vma, size_t amount, ulong flags)
{
	int error;
	unsigned nr_pages = pages_in_range(vma->end, amount);

	if (flags != vma->flags) {
		if (vma_map(vma->mmap, (void*)vma->end, amount, flags) == NULL)
			return -ENOMEM;
		return 0;
	}

	if (vma->chain.next != &vma->mmap->map) {
		struct vma *next = list_entry(vma->chain.next, struct vma, chain);
		if (next->start < vma->end + amount)
			return -ENOMEM;
	}

	if ((error = map_pages(vma->mmap->pgdir, vma->end, nr_pages, flags)))
		return error;

	vma->end += page_align(amount);
	return 0;
}

static void vma_insert(struct mm_struct *mm, struct vma *vma)
{
	struct list_head *it;

	list_for_each(it, &mm->map) {
		if (list_entry(it, struct vma, chain)->start > vma->start)
			break;
	}
	list_add_tail(&vma->chain, it);
	vma->mmap = mm;
}

static int vma_split_left(struct vma *vma, ulong end, ulong flags)
{
	struct vma *left;

	if ((left = alloc_vma()) == NULL)
		return -ENOMEM;

	left->start = vma->start;
	left->end = end;
	left->flags = flags;
	left->mmap = vma->mmap;

	vma->start = end;

	list_add_tail(&left->chain, &vma->chain);
	return 0;
}

static int vma_split_right(struct vma *vma, ulong start, ulong flags)
{
	struct vma *right;

	if ((right = alloc_vma()) == NULL)
		return -ENOMEM;

	right->start = start;
	right->end = vma->end;
	right->flags = flags;
	right->mmap = vma->mmap;

	vma->end = start;

	list_add(&right->chain, &vma->chain);
	return 0;
}

int vma_split(struct vma *vma, ulong start, ulong end, ulong flags)
{
	struct vma *mid, *right;

	if (vma->start == start)
		return vma_split_left(vma, end, flags);
	else if (vma->end == end)
		return vma_split_right(vma, start, flags);

	/* split middle */
	if ((mid = alloc_vma()) == NULL)
		return -ENOMEM;
	if ((right = alloc_vma()) == NULL) {
		free_vma(mid);
		return -ENOMEM;
	}

	right->start = end;
	right->end = vma->end;
	right->flags = vma->flags;
	right->mmap = vma->mmap;

	mid->start = start;
	mid->end = end;
	mid->flags = flags;
	mid->mmap = vma->mmap;

	vma->end = start;

	list_add(&mid->chain, &vma->chain);
	list_add(&right->chain, &mid->chain);
	return 0;
}

struct vma *vma_map(struct mm_struct *mm, void *dstp, size_t len, ulong flags)
{
	struct vma *vma;
	ulong dst = page_base((ulong)dstp);
	unsigned nr_pages = pages_in_range((ulong)dstp, len);

	if (!(vma = new_vma(dst, dst + nr_pages*FRAME_SIZE, flags)))
		return NULL;

	// TODO: demand paging
	if (map_pages(mm->pgdir, dst, nr_pages, flags)) {
		free_vma(vma);
		return NULL;
	}

	vma_insert(mm, vma);
	return vma;
}
