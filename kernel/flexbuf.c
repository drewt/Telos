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

#include <kernel/flexbuf.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <string.h>

/*
 * flexbuf.c: flexible buffers.
 *
 * A flexbuf is an array of pages.  Flexbufs may be read from/written to
 * at arbitrary offsets.  Memory is allocated on-demand.
 *
 * A flexbuf may also be used as a queue.
 */

/*
 * Offset into the page for a given buffer offset.
 */
static inline size_t page_offset(size_t off)
{
	return off % FRAME_SIZE;
}

/*
 * Amount of free space in the last allocated page.
 */
static inline size_t flexbuf_headroom(struct flexbuf *fb)
{
	return FRAME_SIZE - page_offset(fb->pos + fb->size);
}

/*
 * Index into the page list for a given buffer offset.
 */
static inline unsigned off_to_page(size_t off)
{
	return off / FRAME_SIZE;
}

/*
 * Number of pages allocated for a flexbuf.
 */
static inline unsigned flexbuf_nr_pages(struct flexbuf *fb)
{
	return fb->size ? pages_in_range(fb->pos, fb->size) : 0;
}

/*
 * Size in bytes needed for a flexbuf descriptor describing a flexbuf of a
 * given size.
 */
static inline size_t descriptor_size(unsigned slots)
{
	return sizeof(struct flexbuf) + sizeof(unsigned char*) * slots;
}

/*
 * Allocate a flexbuf with a given initial size.
 */
struct flexbuf *flexbuf_alloc(size_t size)
{
	unsigned pages = off_to_page(size);
	unsigned slots = MAX(pages, 8);
	struct flexbuf *fb = kmalloc(descriptor_size(slots));
	fb->size = size;
	fb->pos = 0;
	fb->nr_slots = slots;
	for (unsigned i = 0; i < pages; i++)
		fb->page[i] = kalloc_pages(1);
	return fb;
}

void flexbuf_free(struct flexbuf *fb)
{
	unsigned pages = flexbuf_nr_pages(fb);
	for (unsigned i = 0; i < pages; i++)
		kfree_pages(fb->page[i], 1);
	kfree(fb);
}

/*
 * Reallocate a flexbuf descriptor for a given flexbuf size.
 */
static struct flexbuf *realloc_descriptor(struct flexbuf *old, size_t size)
{
	struct flexbuf *new = kmalloc(descriptor_size(old->pos + size));
	new->size = size;
	new->pos = old->pos;
	new->nr_slots = off_to_page(old->pos + size);

	const unsigned old_pages = flexbuf_nr_pages(old);
	const unsigned new_pages = flexbuf_nr_pages(new);
	for (unsigned page = 0; page < MIN(new_pages, old_pages); page++)
		new->page[page] = old->page[page];
	return new;
}

/*
 * Grow or shrink a flexbuf to a given size.
 */
struct flexbuf *flexbuf_realloc(struct flexbuf *fb, size_t size)
{
	unsigned old_pages = flexbuf_nr_pages(fb);
	unsigned new_pages = size ? pages_in_range(fb->pos, size) : 0;

	// reallocate descriptor if we're out of slots
	if (new_pages > fb->nr_slots) {
		struct flexbuf *new = kmalloc(descriptor_size(new_pages));
		new->size = size;
		new->pos = fb->pos;
		new->nr_slots = off_to_page(fb->pos + size);
		for (unsigned page = 0; page < old_pages; page++)
			new->page[page] = fb->page[page];
		fb = new;
	}

	// if we're growing...
	for (unsigned page = old_pages; page < new_pages; page++) {
		fb->page[page] = kalloc_pages(1);
	}
	// if we're shrinking...
	for (unsigned page = new_pages; page < old_pages; page++) {
		kfree_pages(fb->page[page], 1);
	}

	fb->size = size;
	return fb;
}

/*
   There are four variables to take into account when doing I/O on a flexbuf:
   the offset into the first page (pos), the flexbuf size (size), the I/O
   request offset (off), and the I/O request length (length).

   Since the pages are not contiguous in memory, I/O is performed by iterating
   over the range of pages involved in the I/O request.  At each iteration, we
   keep track of the 'start' and 'end' offsets into the current page.  Most of
   the time these are 0 and FRAME_SIZE, respectively; on the first and last
   iteration, they will be something else.

                                     size
       ________________________________|_______________________________
      (                                                                )
                off                   length
       __________|__________  __________|_________
      (                     )(                    )
   [_____page[0]_____][_____page[1]_____][_____page[2]_____][_____page[3]_____]
      ^                                                                ^
     pos                                                            pos+size

*/

enum { FB_READ, FB_WRITE };

/*
 * Perform an I/O operation on a flexbuf.  Valid values for @rw are FB_READ
 * and FB_WRITE.
 */
static ssize_t flexbuf_do_io(struct flexbuf *fb, char *ub, size_t length,
		size_t off, int rw)
{
	length = MIN(length, fb->size - off);

	size_t nr_bytes = 0;
	size_t start = page_offset(fb->pos + off);
	size_t end = MIN(FRAME_SIZE, start + length);
	unsigned first_page = off_to_page(fb->pos + off);
	unsigned nr_pages = pages_in_range(fb->pos + off, length);

	for (unsigned page = first_page; page - first_page < nr_pages; page++) {
		if (rw == FB_READ) {
			memcpy(ub+nr_bytes, fb->page[page]+start, end-start);
		} else {
			memcpy(fb->page[page]+start, ub+nr_bytes, end-start);
		}
		nr_bytes += end - start;
		start = 0;
		end = MIN(FRAME_SIZE, length - nr_bytes);
	}
	return nr_bytes;
}

/*
 * Read @length bytes from @src, at the offset given by @pos, into @dst.  If
 * the flexbuf isn't large enough to accomodate the entire read, this function
 * reads as much data as is available.
 *
 * @return the number of bytes read.
 */
ssize_t flexbuf_read(struct flexbuf *src, char *dst, size_t length, size_t pos)
{
	return flexbuf_do_io(src, dst, length, pos, FB_READ);
}

/*
 * Write @length bytes from @src into @dst at the offset given by @pos.  If the
 * flexbuf isn't large enough to accomodate the write, it is reallocated.
 *
 * @return the number of bytes written, which should always be equal to @length.
 */
ssize_t flexbuf_write(struct flexbuf **dst, const char *src, size_t length,
		size_t pos)
{
	if (pos + length > (*dst)->size) {
		if (!(*dst = flexbuf_realloc(*dst, pos + length)))
			return -ENOMEM;
	}

	return flexbuf_do_io(*dst, (char*)src, length, pos, FB_WRITE);
}

/*
 * Append @length bytes from @src to the flexbuf @dst.
 *
 * @return the number of bytes enqueued, which should always be equal to @length.
 */
ssize_t flexbuf_enqueue(struct flexbuf **dst, const char *src, size_t length)
{
	return flexbuf_write(dst, src, length, (*dst)->size);
}

/*
 * Read @length bytes from @src into @dst, removing them from the flexbuf.  As
 * with flexbuf_read, this function may not read the amount of data requested.
 *
 * @return the number of bytes read.
 */
ssize_t flexbuf_dequeue(struct flexbuf *src, char *dst, size_t length)
{
	ssize_t nr_bytes = flexbuf_read(src, dst, length, 0);
	unsigned shift = (src->pos + nr_bytes) / FRAME_SIZE;

	for (unsigned page = 0; page < shift; page++)
		src->page[page] = src->page[shift+page];
	src->size -= nr_bytes;
	src->pos = page_offset(src->pos + nr_bytes);
	return nr_bytes;
}
