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

static struct slab_cache *area_cachep;

static void mmap_sysinit(void)
{
	area_cachep = slab_cache_create(sizeof(struct vma));
}
EXPORT_KINIT(mmap, SUB_LAST, mmap_sysinit);

#define alloc_vma() slab_alloc(area_cachep)
#define free_vma(p) slab_free(area_cachep, p)

static inline ulong vma_to_page_flags(ulong flags)
{
	return  ((flags & VM_READ) ? PE_U : 0) |
		((flags & VM_WRITE) ? PE_RW : 0);
}

struct vma *vma_find(struct mm_struct *mm, void *addr)
{
	struct vma *area;

	list_for_each_entry(area, &mm->map, chain) {
		if (area->end < (ulong)addr)
			return area;
	}
	return NULL;
}

int vma_grow_up(struct vma *vma, size_t amount)
{
	int rc;
	unsigned nr_pages = pages_in_range(vma->end, amount);
	ulong pflags = vma_to_page_flags(vma->flags);

	if (vma->chain.next != &vma->mmap->map) {
		struct vma *next = list_entry(vma->chain.next, struct vma, chain);
		if (next->start < vma->end + amount)
			return -1;
	}

	if ((rc = map_zpages(vma->mmap->pgdir, vma->end, nr_pages, pflags)))
		return rc;

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

struct vma *zmap(struct mm_struct *mm, void *dstp, size_t len, ulong flags)
{
	struct vma *vma;
	ulong dst = page_base((ulong)dstp);
	unsigned nr_pages = pages_in_range((ulong)dstp, len);
	ulong pflags = vma_to_page_flags(flags);

	if ((vma = alloc_vma()) == NULL)
		return NULL;

	if (map_zpages(mm->pgdir, dst, nr_pages, pflags)) {
		free_vma(vma);
		return NULL;
	}

	vma->start = dst;
	vma->end = dst + nr_pages*FRAME_SIZE;
	vma->flags = flags;
	vma_insert(mm, vma);

	return vma;
}
