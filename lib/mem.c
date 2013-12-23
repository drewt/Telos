/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <syscall.h>

#include <kernel/list.h> /* FIXME: shouldn't include kernel header */

#define STEP_SIZE  1024

#define MAGIC_FREE 0xF2EEB10C
#define MAGIC_OK   0x600DC0DE

#define next_hdr(hdr) ((struct mem_header*) ((hdr)->data + (hdr)->size))

#define round_up(v, size) \
	((v) % (size) == 0 ? (v) : (v) + ((size) - (v) % (size)))

#define round_down(v, size) \
	((v) % (size) == 0 ? (v) : (v) - ((v) % (size)))

int errno;

struct mem_header {
	struct list_head chain;
	unsigned long    size;
	unsigned long    magic;
	unsigned char    data[];
};

_Static_assert(STEP_SIZE % sizeof(struct mem_header) == 0,
		"STEP_SIZE must be a multiple of sizeof(struct mem_header)");

static LIST_HEAD(free_list);

/* Increases brk and returns a mem_header for a block of at least 'size'
 * bytes.
 */
static struct mem_header *__grow_heap(size_t size)
{
	struct mem_header *r;

	if ((long) (r = sbrk(size)) == -1)
		return NULL;
	return (void*) round_down((unsigned long)r, sizeof(*r));
}

/* Increases brk and returns a mem_header for a block of at least 'size'
 * bytes, extending the last free block if possible.
 */
static struct mem_header *grow_heap(size_t size)
{
	struct mem_header *new, *last;
	size_t step = round_up(size + sizeof(*new), STEP_SIZE);

	if ((new = __grow_heap(step)) == NULL)
		return NULL;

	if (list_empty(&free_list))
		goto skip;

	last = list_entry(free_list.prev, struct mem_header, chain);
	if (next_hdr(last) == new) {
		last->size += step;
		list_del(&last->chain);
		return last;
	}

skip:
	new->size = step - sizeof(*new);
	new->magic = MAGIC_FREE;

	return new;
}

void print_free_list(void)
{
	struct mem_header *it;

	list_for_each_entry(it, &free_list, chain) {
		printf("%x -> %x\n", it->data, it->data + it->size);
	}
}

/* This is necessary because libc isn't "fresh" when a program is run a
 * second time.
 */
void malloc_init(void)
{
	INIT_LIST_HEAD(&free_list);
}

void free(void *ptr)
{
	struct mem_header *it;
	struct mem_header *h = (struct mem_header*) ptr - 1;

	if (h->magic != MAGIC_OK) {
		printf("libc: detected double free or corruption\n");
		return;
	}

	list_for_each_entry(it, &free_list, chain) {
		if (it > h)
			break;
	}

	list_add_tail(&h->chain, &it->chain);
	/* TODO: coalesce */
}

void *malloc(size_t size)
{
	size_t blksize;
	struct mem_header *it, *rem, *h = NULL;

	if (size == 0)
		return NULL;

	list_for_each_entry(it, &free_list, chain) {
		if (it->size >= size) {
			h = it;
			break;
		}
	}

	if (h != NULL)
		list_del(&h->chain);
	else if ((h = grow_heap(size)) == NULL)
		return NULL;

	h->magic = MAGIC_OK;

	if (h->size < size + sizeof(*h)*2)
		return h->data;

	blksize = round_up(size, sizeof(*h));
	rem = (struct mem_header*) (h->data + blksize);
	rem->size = h->size - blksize - sizeof(*rem);
	rem->magic = MAGIC_FREE;
	list_add_tail(&rem->chain, &free_list);

	h->size = blksize;

	return h->data;
}

void *calloc(size_t nmemb, size_t size)
{
	void *ptr;
	size_t total = nmemb * size;

	if ((ptr = malloc(total)) == NULL)
		return NULL;

	memset(ptr, 0, total);
	return ptr;
}

void *realloc(void *ptr, size_t size)
{
	size_t old;
	void *newptr;

	if (ptr != NULL && size == 0) {
		free(ptr);
		return NULL;
	} else if (ptr == NULL) {
		return malloc(size);
	}

	/* TODO: check neighbouring blocks, shrink, etc. */
	if ((newptr = malloc(size)) == NULL)
		return NULL;

	old = ((struct mem_header*)ptr - 1)->size;
	memcpy(newptr, ptr, old < size ? old : size);
	free(ptr);
	return newptr;
}

void *sbrk(long increment)
{
	unsigned long old;
	long rc;

	if ((rc = syscall2(SYS_SBRK, (void*) increment, &old)) < 0) {
		errno = -rc;
		return (void*) -1;
	}

	return (void*) old;
}
