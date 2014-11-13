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
#include <kernel/mmap.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <kernel/mm/vma.h>
#include <string.h>

static DEFINE_SLAB_CACHE(area_cachep, sizeof(struct vma));

#define alloc_vma() slab_alloc(area_cachep)
#define free_vma(p) slab_free(area_cachep, p)

struct mm_struct kernel_mm;

static void vm_sysinit(void)
{
	kernel_mm.pgdir = &_kernel_pgd;

	INIT_LIST_HEAD(&kernel_mm.map);
	INIT_LIST_HEAD(&kernel_mm.kheap);
	if (!vma_map(&kernel_mm, krostart, kroend - krostart, 0))
		panic("Failed to map kernel memory");
	if (!vma_map(&kernel_mm, krwstart, krwend - krwstart, VM_WRITE))
		panic("Failed to map kernel memory");

}
EXPORT_KINIT(vm, SUB_LAST, vm_sysinit);

static struct vma *new_vma(ulong start, ulong end, ulong flags)
{
	struct vma *vma;
	if (!(vma = alloc_vma()))
		return NULL;
	vma->start = start;
	vma->end = end;
	vma->flags = flags;
	vma->op = NULL;
	return vma;
}

static unsigned long vma_find_space(struct mm_struct *mm, void *z_start,
		void *z_end, size_t len)
{
	struct vma *vma;
	unsigned long start = (unsigned long) z_start;
	unsigned long end = (unsigned long) z_end;
	list_for_each_entry(vma, &mm->map, chain) {
		if (vma->start < start)
			continue;
		if (start + len >= end)
			return 0;
		if (start + len < vma->start)
			return start;
		start = vma->end;
	}
	// XXX: assumes a VMA exists after end
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

struct vma *create_vma(struct mm_struct *mm, void *z_start, void *z_end,
		size_t len, unsigned long flags)
{
	unsigned long start = vma_find_space(mm, z_start, z_end, len);
	struct vma *vma = new_vma(start, page_align(start+len), flags);
	if (!vma)
		return NULL;
	vma_insert(mm, vma);
	return vma;
}

int mm_init(struct mm_struct *mm)
{
	if (!(mm->pgdir = new_pgdir()))
		return -ENOMEM;
	INIT_LIST_HEAD(&mm->map);
	INIT_LIST_HEAD(&mm->kheap);
	return 0;
}

void mm_fini(struct mm_struct *mm)
{
	struct vma *vma, *n;
	list_for_each_entry_safe(vma, n, &mm->map, chain) {
		vm_unmap(vma);
		free_vma(vma);
	}
	del_pgdir(mm->pgdir);
}

int mm_clone(struct mm_struct *dst, struct mm_struct *src)
{
	struct vma *vma;

	if (!(dst->pgdir = clone_pgdir()))
		return -ENOMEM;

	INIT_LIST_HEAD(&dst->map);
	INIT_LIST_HEAD(&dst->kheap);
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

struct vma *vma_find(const struct mm_struct *mm, const void *addr)
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
	if (flags != vma->flags) {
		if (vma_map(vma->mmap, vma->end, amount, flags) == NULL)
			return -ENOMEM;
		return 0;
	}

	if (vma->chain.next != &vma->mmap->map) {
		struct vma *next = list_entry(vma->chain.next, struct vma, chain);
		if (next->start < vma->end + amount)
			return -ENOMEM;
	}

	vma->end += page_align(amount);
	return 0;
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

struct vma *vma_map(struct mm_struct *mm, ulong dst, size_t len, ulong flags)
{
	struct vma *vma;
	unsigned nr_pages = pages_in_range(dst, len);
	dst = page_base(dst);

	if (!(vma = new_vma(dst, dst + nr_pages*FRAME_SIZE, flags)))
		return NULL;

	if (flags & VM_ALLOC && map_pages(mm->pgdir, dst, nr_pages, flags)) {
		free_vma(vma);
		return NULL;
	}

	vma_insert(mm, vma);
	return vma;
}

int vm_map_page(struct vma *vma, void *addr)
{
	// default: demand paging
	if (!vma->op || !vma->op->map)
		return map_page(addr, vma->flags);
	return vma->op->map(vma, addr);
}

int vm_writeback(struct vma *vma, void *addr, size_t len)
{
	// default: no writeback
	if (!vma->op || !vma->op->writeback)
		return 0;
	return vma->op->writeback(vma, addr, len);
}

int vm_read_perm(struct vma *vma, void *addr)
{
	if (!vma->op || !vma->op->read_perm)
		return -EFAULT;
	return vma->op->read_perm(vma, addr);
}

int vm_write_perm(struct vma *vma, void *addr)
{
	if (!vma->op || !vma->op->write_perm)
		return -EFAULT;
	return vma->op->write_perm(vma, addr);
}

int vm_unmap(struct vma *vma)
{
	if (!vma->op || !vma->op->unmap)
		return 0;
	return vma->op->unmap(vma);
}

int vm_verify(const struct mm_struct *mm, const void *start, size_t len,
		ulong flags)
{
	struct vma *vma;

	if (!(vma = vma_get(mm, start))) {
		kprintf("%p not mapped!\n", start);
		return -1; // not mapped
	}
	if ((vma->flags & flags) != flags) {
		kprintf("%p bad access!\n", start);
		return -1; // bad access
	}
	if ((ulong)start - vma->start < len) {
		kprintf("%p bad length!\n", start);
		return -1; // invalid length
	}
	return 0;
}

int vm_copy_from(const struct mm_struct *mm, void *dst, const void *src,
		size_t len)
{
	void *addr;
	struct vma *vma;
	if (!(vma = vma_get(mm, src)))
		return -EFAULT;
	if (!(addr = kmap_tmp_range(mm->pgdir, (ulong)src, len, vma->flags)))
		return -ENOMEM;
	memcpy(dst, addr, len);
	kunmap_tmp_range(addr, len);
	return 0;
}

int vm_copy_to(const struct mm_struct *mm, void *dst, const void *src,
		size_t len)
{
	void *addr;
	struct vma *vma;
	if (!(vma = vma_get(mm, dst)))
		return -EFAULT;
	if (!(addr = kmap_tmp_range(mm->pgdir, (ulong)dst, len, vma->flags)))
		return -ENOMEM;
	memcpy(addr, src, len);
	kunmap_tmp_range(addr, len);
	return 0;
}
