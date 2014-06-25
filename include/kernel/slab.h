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

#ifndef _KERNEL_SLAB_H_
#define _KERNEL_SLAB_H_

#include <stddef.h>
#include <stdint.h>

struct slab_cache {
	struct list_head full;
	struct list_head partial;
	struct list_head empty;
	unsigned pages_per_slab;
	unsigned objs_per_slab;
	size_t obj_size;
};

struct slab {
	struct list_head chain;
	unsigned in_use;
	void *mem;
	ulong bitmap[];
};

struct slab_cache *slab_cache_create(size_t size);
void *slab_alloc(struct slab_cache *cache);
void slab_free(struct slab_cache *cache, void *mem);

#endif
