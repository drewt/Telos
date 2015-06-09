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

#ifndef _KERNEL_FLEXBUF_H_
#define _KERNEL_FLEXBUF_H_

struct flexbuf {
	size_t size;           // size of the flexbuf in bytes
	size_t pos;            // offset into the first page
	unsigned nr_slots;     // number of slots
	unsigned char *page[]; // slots for pointers to pages
};

struct flexbuf *flexbuf_alloc(size_t size);
void flexbuf_free(struct flexbuf *fb);
struct flexbuf *flexbuf_realloc(struct flexbuf *fb, size_t size);
ssize_t flexbuf_read(struct flexbuf *src, char *dst, size_t length, size_t pos);
ssize_t flexbuf_write(struct flexbuf **dst, const char *src, size_t length,
		size_t pos);
ssize_t flexbuf_enqueue(struct flexbuf **dst, const char *src, size_t length);
ssize_t flexbuf_dequeue(struct flexbuf *src, char *dst, size_t length);

#endif
