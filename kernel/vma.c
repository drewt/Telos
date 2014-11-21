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
	kernel_mm.pgdir = (uintptr_t)&_kernel_pgd;

	INIT_LIST_HEAD(&kernel_mm.map);
	INIT_LIST_HEAD(&kernel_mm.kheap);
	if (!vma_map(&kernel_mm, krostart, kroend - krostart, 0))
		panic("Failed to map kernel memory");
	if (!vma_map(&kernel_mm, krwstart, krwend - krwstart, VM_WRITE))
		panic("Failed to map kernel memory");

}
EXPORT_KINIT(vm, SUB_LAST, vm_sysinit);

static struct vma *new_vma(uintptr_t start, uintptr_t end, int flags)
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

static uintptr_t vma_find_space_high(struct mm_struct *mm, uintptr_t start,
		uintptr_t end, size_t len)
{
	struct vma *vma;
	len = page_align(len);
	list_for_each_entry_reverse(vma, &mm->map, chain) {
		if (vma->start >= end)
			continue;
		if (start + len > end)
			return 0;
		if (vma->end + len <= end)
			return end - len;
		end = vma->start;
	}
	if (end <= start || end - start < len)
		return 0;
	return end - len;
}

static uintptr_t vma_find_space(struct mm_struct *mm, uintptr_t start,
		uintptr_t end, size_t len)
{
	struct vma *vma;
	list_for_each_entry(vma, &mm->map, chain) {
		if (vma->end <= start)
			continue;
		if (start + len >= end)
			return 0;
		if (start + len < vma->start)
			return start;
		start = vma->end;
	}
	if (start >= end || start + len > end)
		return 0;
	return start;
}

struct vma *vma_create_high(struct mm_struct *mm, uintptr_t start,
		uintptr_t end, size_t len, int flags)
{
	uintptr_t addr;
	struct vma *vma;
	addr = vma_find_space_high(mm, start, end, len);
	if (!addr)
		return NULL;
	vma = new_vma(addr, page_align(addr+len), flags);
	if (!vma)
		return NULL;
	vma_insert(mm, vma);
	return vma;
}

struct vma *vma_create_low(struct mm_struct *mm, uintptr_t start,
		uintptr_t end, size_t len, int flags)
{
	uintptr_t addr;
	struct vma *vma;
	addr = vma_find_space(mm, start, end, len);
	if (!addr)
		return NULL;
	vma = new_vma(addr, page_align(addr+len), flags);
	if (!vma)
		return NULL;
	vma_insert(mm, vma);
	return vma;
}

struct vma *vma_create_fixed(struct mm_struct *mm, uintptr_t start, size_t len,
		int flags)
{
	struct vma *vma = vma_find(mm, (void*)start);
	if (vma && (vma_contains(vma, (void*)start) || vma->start - start < len))
		return NULL;
	vma = new_vma(start, page_align(start+len), flags);
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
	}
	free_pgdir(mm->pgdir);
}

int mm_clone(struct mm_struct *dst, struct mm_struct *src)
{
	struct vma *vma;
	int error = 0;

	if (!(dst->pgdir = clone_pgdir()))
		return -ENOMEM;

	INIT_LIST_HEAD(&dst->map);
	INIT_LIST_HEAD(&dst->kheap);
	list_for_each_entry(vma, &src->map, chain) {
		struct vma *new = new_vma(vma->start, vma->end, vma->flags);
		if (!new) {
			error = -ENOMEM;
			goto abort;
		}
		new->mmap = dst;
		list_add_tail(&new->chain, &dst->map);
		vm_clone(new, vma);
	}
	// TODO: dst->kheap
	dst->brk = src->brk;
	return 0;
abort:
	mm_fini(dst);
	return error;
}

struct vma *vma_find(const struct mm_struct *mm, const void *addr)
{
	struct vma *area;

	list_for_each_entry(area, &mm->map, chain) {
		if (area->end > (uintptr_t)addr)
			return area;
	}
	return NULL;
}

int vma_grow_up(struct vma *vma, size_t amount, int flags)
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

int vma_bisect(struct vma *vma, uintptr_t split, int lflags, int rflags)
{
	int error;
	struct vma *right;
	int orig_flags = vma->flags;

	if (!(right = alloc_vma()))
		return -ENOMEM;

	right->start = split;
	right->end = vma->end;
	right->flags = rflags;
	right->mmap = vma->mmap;
	right->op = vma->op;

	vma->end = split;
	vma->flags = lflags;

	if ((error = vm_split(right, vma))) {
		vma->flags = orig_flags;
		vma->end = right->end;
		free_vma(right);
		return error;
	}

	list_add(&right->chain, &vma->chain);
	return 0;
}

int vma_split(struct vma *vma, uintptr_t start, uintptr_t end, int flags)
{
	int error;
	struct vma *mid, *right;

	if (vma->start == start)
		return vma_bisect(vma, end, flags, vma->flags);
	else if (vma->end == end)
		return vma_bisect(vma, start, vma->flags, flags);

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

	// XXX: the order matters here!  Note that the second call uses mid
	//      as the "old" VMA -- this is so split() always gets adjacent
	//      VMAs.
	if ((error = vm_split(mid, vma)) || (error = vm_split(right, mid))) {
		free_vma(mid);
		free_vma(right);
		return error;
	}

	vma->end = start;
	list_add(&mid->chain, &vma->chain);
	list_add(&right->chain, &mid->chain);
	return 0;
}

struct vma *vma_map(struct mm_struct *mm, uintptr_t dst, size_t len, int flags)
{
	struct vma *vma;
	unsigned nr_pages = pages_in_range(dst, len);
	dst = page_base(dst);

	if (!(vma = new_vma(dst, dst + nr_pages*FRAME_SIZE, flags)))
		return NULL;

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
	if (vma->flags & VM_WRITE)
		return copy_page(addr, vma->flags);
	if (!vma->op || !vma->op->write_perm)
		return -EFAULT;
	return vma->op->write_perm(vma, addr);
}

int vm_unmap(struct vma *vma)
{
	int error;
	if (vma->flags & VM_KEEP)
		return 0;
	if ((error = pm_unmap(vma)))
		return error;
	if (vma->op && vma->op->unmap)
		vma->op->unmap(vma);
	list_del(&vma->chain);
	free_vma(vma);
	return 0;
}

int vm_clone(struct vma *dst, struct vma *src)
{
	dst->op = src->op;
	if (src->flags & VM_WRITE && !(src->flags & VM_SHARE)) {
		pm_disable_write(src);
		pm_disable_write(dst);
	}
	if (!src->op || !src->op->clone)
		return 0;
	return src->op->clone(dst, src);
}

int vm_split(struct vma *new, struct vma *old)
{
	new->op = old->op;
	if (old->op && old->op->split)
		return old->op->split(new, old);
	return 0;
}

int vm_verify(const struct mm_struct *mm, const void *start, size_t len,
		int flags)
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
	if ((uintptr_t)start - vma->start < len) {
		kprintf("%p bad length!\n", start);
		return -1; // invalid length
	}
	return 0;
}

int vm_verify_unmapped(const struct mm_struct *mm, const void *start,
		size_t len)
{
	struct vma *vma = vma_find(mm, start);
	if (!vma || vma->start > (uintptr_t)start + len)
		return 0;
	return -1;
}

int vm_copy_from(const struct mm_struct *mm, void *dst, const void *src,
		size_t len)
{
	void *addr;
	struct vma *vma;
	if (!(vma = vma_get(mm, src)))
		return -EFAULT;
	if (!(addr = kmap_tmp_range(mm->pgdir, (uintptr_t)src, len, vma->flags)))
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
	if (!(addr = kmap_tmp_range(mm->pgdir, (uintptr_t)dst, len, vma->flags)))
		return -ENOMEM;
	memcpy(addr, src, len);
	kunmap_tmp_range(addr, len);
	return 0;
}
