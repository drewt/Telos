/*  Copyright 2013-2015 Drew Thoreson
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

#include <kernel/bitmap.h>
#include <kernel/list.h>
#include <kernel/mmap.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>

#define bitmap_length(cache) (cache->pages_per_slab * (256 / BITS_PER_LONG))
#define bitmap_size(cache) (bitmap_length(cache) * sizeof(unsigned long))
#define slab_size(cache) (cache->pages_per_slab * FRAME_SIZE)
#define slab_desc_size(cache) (sizeof(struct slab) + bitmap_size(cache))
#define slab_mem_size(cache) (slab_size(cache) - slab_desc_size(cache))

#define slab_set ((struct slab_init_struct **) &_slab_set)
#define slab_set_length __set_length(&_slab_set, &_slab_set_end)

SYSINIT(slab, SUB_SLAB)
{
	for (unsigned i = 0; i < slab_set_length; i++)
		*(slab_set[i]->cache) = slab_cache_create(slab_set[i]->object_size);
}

struct slab {
	struct list_head chain;
	unsigned int in_use;
	void *mem;
	unsigned long bitmap[];
};

struct slab_cache *slab_cache_create(size_t size)
{
	struct slab_cache *cache = kmalloc(sizeof(struct slab_cache));

	if (cache == NULL)
		return NULL;

	INIT_LIST_HEAD(&cache->full);
	INIT_LIST_HEAD(&cache->partial);
	INIT_LIST_HEAD(&cache->empty);
	cache->obj_size = (size < 16) ? 16 : size;
	cache->pages_per_slab = 1;

	cache->objs_per_slab = slab_mem_size(cache) / cache->obj_size;

	return cache;
}

/* Allocates and initializes a new slab for the given cache */
static struct slab *new_slab(struct slab_cache *cache)
{
	struct slab *slab;

	if ((slab = kalloc_pages(cache->pages_per_slab)) == NULL)
		return NULL;

	slab->mem = slab->bitmap + bitmap_length(cache);
	slab->in_use = 0;
	for (uint i = 0; i < bitmap_length(cache); i++)
		slab->bitmap[i] = 0;
	return slab;
}

static struct slab *get_slab(struct slab_cache *cache)
{
	struct slab *slab;

	if (list_empty(&cache->partial) && list_empty(&cache->empty)) {
		if ((slab = new_slab(cache)) == NULL)
			return NULL;
		list_add(&slab->chain, &cache->partial);
		return slab;
	}

	if (list_empty(&cache->partial))
		return list_first_entry(&cache->empty, struct slab, chain);

	return list_first_entry(&cache->partial, struct slab, chain);
}

void *slab_alloc(struct slab_cache *cache)
{
	unsigned long zero;
	struct slab *slab = get_slab(cache);

	/* find first free object and update bitmap */
	zero = bitmap_ffz(slab->bitmap, bitmap_length(cache));
	bitmap_set(slab->bitmap, zero);

	/* move slab to full/partial list, as appropriate */
	if (++slab->in_use == cache->objs_per_slab) {
		list_del(&slab->chain);
		list_add(&slab->chain, &cache->full);
	} else if (slab->in_use == 1) {
		list_del(&slab->chain);
		list_add(&slab->chain, &cache->partial);
	}

	return (void*) ((uintptr_t)slab->mem + zero * cache->obj_size);
}

static inline void *slab_end(struct slab_cache *cache, struct slab *slab)
{
	return (void*) ((uintptr_t)slab->mem + slab_mem_size(cache));
}

static struct slab *find_slab(struct slab_cache *cache, void *mem)
{
	struct slab *slab;

	list_for_each_entry(slab, &cache->full, chain) {
		if (mem >= slab->mem && mem < slab_end(cache, slab))
			return slab;
	}

	list_for_each_entry(slab, &cache->partial, chain) {
		if (mem >= slab->mem && mem < slab_end(cache, slab))
			return slab;
	}

	return NULL;
}

void slab_free(struct slab_cache *cache, void *mem)
{
	unsigned idx;
	struct slab *slab;

	if ((slab = find_slab(cache, mem)) == NULL) {
		kprintf("slab_free: failed to locate slab!\n");
		return;
	}

	idx = ((uintptr_t)mem - (uintptr_t)slab->mem) / cache->obj_size;
	bitmap_clear(slab->bitmap, idx);

	if (--slab->in_use == 0) {
		list_del(&slab->chain);
		list_add(&slab->chain, &cache->empty);
	} else if (slab->in_use + 1 == cache->objs_per_slab) {
		list_del(&slab->chain);
		list_add(&slab->chain, &cache->partial);
	}
}
