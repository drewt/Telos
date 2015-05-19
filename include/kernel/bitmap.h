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

#ifndef _KERNEL_BITMAP_H_
#define _KERNEL_BITMAP_H_

#include <kernel/bitops.h>

/*
 * Generic bitmap implementation, supporting bitmaps of arbitrary length.
 */

static inline void bitmap_set(unsigned long *bitmap, unsigned idx)
{
	bitmap[idx / BITS_PER_LONG] |= 1UL << (idx % BITS_PER_LONG);
}

static inline void bitmap_clear(unsigned long *bitmap, unsigned idx)
{
	bitmap[idx / BITS_PER_LONG] &=  ~(1UL << (idx % BITS_PER_LONG));
}

static inline void bitmap_change(unsigned long *bitmap, unsigned idx)
{
	bitmap[idx / BITS_PER_LONG] ^= 1UL << (idx % BITS_PER_LONG);
}

static inline long bitmap_ffz(unsigned long *bitmap, unsigned len)
{
	for (unsigned i = 0; i < len; i++) {
		if (bitmap[i] == ~0UL)
			continue;

		return ffz(bitmap[i]) + i * BITS_PER_LONG;
	}
	return -1;
}

static inline long bitmap_ffs(unsigned long *bitmap, unsigned len)
{
	for (unsigned i = 0; i < len; i++) {
		if (bitmap[i] == 0UL)
			continue;
		return ffs(bitmap[i]) + i * BITS_PER_LONG;
	}
	return -1;
}

#endif
