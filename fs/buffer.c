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

#include <kernel/fs.h>
#include <kernel/hashtable.h>
#include <kernel/mm/paging.h>
#include <telos/major.h>

/*
 * The buffer cache.
 *
 * This is the half of the glue between the filesystem code and the block
 * device code (the other half is in drivers/block/rw_block.c).
 */

/*
 * Hash table storing all active/cached buffers.
 */
static DEFINE_HASHTABLE(ht_blocks, 9);

static DEFINE_SLAB_CACHE(buffer_cachep, sizeof(struct buffer));

static long buffer_key(dev_t dev, blkcnt_t block, blksize_t size)
{
	return dev+block;
}

/*
 * Find a buffer in the hash table.
 */
static struct buffer *find_buffer(dev_t dev, blkcnt_t block, blksize_t size)
{
	struct buffer *buffer;
	long key = buffer_key(dev, block, size);
	hash_for_each_possible(ht_blocks, buffer, b_hash, key) {
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
	hash_add(ht_blocks, &b->b_hash, buffer_key(dev, block, size));
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

/*
 * Flush any writes to a buffer to disk.
 */
static void flush_buffer(struct buffer *buffer)
{
	if (!(buffer->b_flags & BUF_DIRTY))
		return;
	submit_block(WRITE, buffer);
	buffer_wait(buffer);
}

static void free_buffer(struct buffer *buffer)
{
	if (buffer->b_count)
		panic("tried to free referenced buffer");
	kfree_pages(buffer->b_data, 1);
	slab_free(buffer_cachep, buffer);
	hash_del(&buffer->b_hash);
}

/*
 * Flush and free all buffers associated with a given device.
 */
int free_device_buffers(dev_t dev)
{
	unsigned bkt;
	struct hlist_node *tmp;
	struct buffer *buffer;
	hash_for_each_safe(ht_blocks, bkt, tmp, buffer, b_hash) {
		if (buffer->b_dev != dev)
			continue;
		if (buffer->b_lock)
			buffer_wait(buffer);
		if (buffer->b_count)
			return -EBUSY;
		flush_buffer(buffer);
		free_buffer(buffer);
	}
	return 0;
}

void clear_buffer_cache(void)
{
	unsigned i;
	struct buffer *buffer;
	struct hlist_node *tmp;
	hash_for_each_safe(ht_blocks, i, tmp, buffer, b_hash) {
		if (buffer->b_lock || buffer->b_count > 0)
			continue;
		flush_buffer(buffer);
		free_buffer(buffer);
	}
}
