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

#include <kernel/fs.h>
#include <kernel/hashtable.h>
#include <kernel/mm/paging.h>
#include <sys/major.h>

/*
 * Hash table storing all active/cached buffers.
 */
static DEFINE_HASHTABLE(ht_blocks, 9);

static DEFINE_SLAB_CACHE(buffer_cachep, sizeof(struct buffer));

/*
 * Find a buffer in the hash table.
 */
static struct buffer *find_buffer(dev_t dev, blkcnt_t block, blksize_t size)
{
	struct buffer *buffer;
	hash_for_each_possible(ht_blocks, buffer, b_hash, dev+block) {
		if (buffer->b_dev == dev && buffer->b_blocknr == block) {
			if (buffer->b_size == size)
				return buffer;
			else
				kprintf("Wrong blocksize on device %lu/%lu\n",
						major(dev), minor(dev));
		}
	}
	return NULL;
}

/*
 * Allocate and initialize a buffer.
 */
static struct buffer *make_buffer(dev_t dev, blkcnt_t block, blksize_t size)
{
	struct buffer *b;
	if (!(b = slab_alloc(buffer_cachep)))
		return NULL;
	if (!(b->b_data = kalloc_pages(1))) {
		slab_free(buffer_cachep, b);
		return NULL;
	}
	b->b_flags = 0;
	b->b_dev = dev;
	b->b_blocknr = block;
	b->b_size = size;
	b->b_count = 1;
	b->b_lock = false;
	INIT_WAIT_QUEUE(&b->b_wait);
	hash_add(ht_blocks, &b->b_hash, dev+block);
	return b;
}

/*
 * Get a buffer for a given device/block number, allocating one if the buffer
 * is not in the hash table.  The returned buffer may not be up to date.
 */
static struct buffer *get_buffer(dev_t dev, blkcnt_t block, blksize_t size)
{
	struct buffer *b;
	if ((b = find_buffer(dev, block, size)))
		return b;
	return make_buffer(dev, block, size);
}

/*
 * Bring a buffer up-to-date.
 */
static struct buffer *read_buffer(struct buffer *buffer)
{
	if (!(buffer->b_flags & BUF_UPTODATE)) {
		submit_block(READ, buffer);
		buffer_wait(buffer);
	}
	return buffer;
}

/*
 * Read a block.
 */
struct buffer *read_block(dev_t dev, blkcnt_t block, blksize_t size)
{
	struct buffer *b;
	if (!(b = get_buffer(dev, block, size)))
		return NULL;
	return read_buffer(b);
}

static void flush_buffer(struct buffer *buf)
{
	if (!(buf->b_flags & BUF_DIRTY))
		return;
	submit_block(WRITE, buf);
	buffer_wait(buf);
}

void clear_buffer_cache(void)
{
	unsigned i;
	struct buffer *b;
	struct hlist_node *tmp;
	hash_for_each_safe(ht_blocks, i, tmp, b, b_hash) {
		if (b->b_lock || b->b_count > 0)
			continue;
		flush_buffer(b);
		kfree_pages(b->b_data, 1);
		slab_free(buffer_cachep, b);
	}
}
