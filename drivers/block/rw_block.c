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
#include <kernel/wait.h>
#include <kernel/mm/kmalloc.h>
#include <sys/major.h>
#include <string.h>
#include "block.h"

/*
 * Block I/O layer.  These functions provide the glue between the high-level
 * interface (read_block) and the block device drivers.
 */

static DEFINE_SLAB_CACHE(request_cachep, sizeof(struct request));

static struct block_driver blkdev[MAX_BLKDEV];

/*
 * Wait until a buffer is unlocked.
 */
void buffer_wait(struct buffer *buf)
{
	while (buf->b_lock) {
		wait_interruptible(&buf->b_wait);
		// TODO: handle signals
	}
}

static void buffer_lock(struct buffer *buf)
{
	buffer_wait(buf);
	buf->b_lock = true;
}

static void buffer_unlock(struct buffer *buf)
{
	buf->b_lock = false;
}

#define NR_SECTORS(buffer) ((buffer)->b_size / SECTOR_SIZE)
#define SECTOR(buffer) ((buffer)->b_blocknr * NR_SECTORS(buffer))

static struct request *make_request(int rw, struct buffer *buf)
{
	struct request *req = slab_alloc(request_cachep);
	if (!req)
		panic("No memory for block request");
	// FIXME: block until memory available
	req->rw = rw;
	req->sector = SECTOR(buf);
	req->nr_sectors = NR_SECTORS(buf);
	req->mem = buf->b_data;
	req->buf = buf;
	buf->b_count++;
	return req;
}

static struct block_device *get_device(dev_t dev)
{
	return blkdev[major(dev)].get_device(minor(dev));
}

/*
 * Submit an I/O request on a buffer.
 */
int submit_block(int rw, struct buffer *buf)
{
	struct block_device *dev = get_device(buf->b_dev);
	struct request *req = make_request(rw, buf);
	buffer_lock(buf);
	if (request_queue_empty(dev))
		dev->handle_request(dev, req);
	else
		list_add_tail(&req->chain, &dev->requests);
	return 0;
}

/*
 * Block drivers call this function to signal completion of an I/O request.
 */
void block_request_completed(struct block_device *dev, struct request *req)
{
	req->buf->b_flags |= BUF_UPTODATE;
	release_buffer(req->buf);
	buffer_unlock(req->buf);
	slab_free(request_cachep, req);

	if (request_queue_empty(dev))
		return;
	req = list_first_entry(&dev->requests, struct request, chain);
	list_del(&req->chain);
	dev->handle_request(dev, req);
}

blksize_t blkdev_blksize(dev_t devno)
{
	struct block_device *dev = get_device(devno);
	if (!dev)
		return -1;
	return dev->blksize;
}

int set_blocksize(dev_t devno, blksize_t size)
{
	int error;
	struct block_device *dev;

	if (size != 512 && size != 1024 && size != 2048 && size != 4096)
		panic("set_blocksize: invalid block size: %ld\n", size);
	dev = get_device(devno);
	if (!dev)
		return -ENODEV;
	if (dev->blksize == size)
		return 0;
	// clear all buffers from the cache, since they have the wrong size
	error = free_device_buffers(devno);
	if (error)
		return error;
	dev->blksize = size;
	return 0;
}

/*
 * Use a slab cache for allocating small bio_vecs (the common case for
 * directory/small file block lists), and fall back on kmalloc for larger
 * allocations.
 */
#define SMALL_BIO_MAX 8
#define BIO_VEC_SIZE(blkcnt) \
	(sizeof(struct bio_vec) + sizeof(blkcnt_t)*blkcnt)

static DEFINE_SLAB_CACHE(small_bio_cachep, BIO_VEC_SIZE(SMALL_BIO_MAX));

struct bio_vec *alloc_bio_vec(dev_t dev, blkcnt_t blkcnt, blksize_t blksize)
{
	struct bio_vec *bio = blkcnt <= SMALL_BIO_MAX
		? slab_alloc(small_bio_cachep)
		: kmalloc(BIO_VEC_SIZE(blkcnt));
	bio->dev = dev;
	bio->blkcnt = blkcnt;
	bio->blksize = blksize;
	return bio;
}

void free_bio_vec(struct bio_vec *bio)
{
	if (bio->blkcnt <= SMALL_BIO_MAX)
		slab_free(small_bio_cachep, bio);
	else
		kfree(bio);
}

/*
 * Scatter/gather block I/O.
 */

static ssize_t bio_do_io(struct bio_vec *vec, char *iobuf, size_t len,
		size_t pos, int rw)
{
	size_t nr_bytes = 0;
	blkcnt_t blocks = io_block_count(vec->blksize, pos, len);
	blkcnt_t first_block = io_off_to_block(vec->blksize, pos);
	off_t start = io_block_off(vec->blksize, pos);
	off_t end = MIN((unsigned long)vec->blksize, start + len);

	for (blkcnt_t b = first_block; b - first_block < blocks; b++) {
		// do I/O within a single block
		struct buffer *buffer = read_block(vec->dev, vec->block[b],
				vec->blksize);
		if (rw == READ) {
			memcpy(iobuf+nr_bytes, buffer->b_data+start, end-start);
		} else {
			memcpy(buffer->b_data+start, iobuf+nr_bytes, end-start);
			buffer->b_flags |= BUF_DIRTY;
		}
		release_buffer(buffer);

		// update counts
		nr_bytes += end - start;
		start = 0;
		end = MIN((unsigned long)vec->blksize, len - nr_bytes);
	}
	return nr_bytes;
}

ssize_t bio_read(struct bio_vec *vec, char *iobuf, size_t len,
		unsigned long *pos)
{
	ssize_t r = bio_do_io(vec, iobuf, len, *pos, READ);
	if (r < 0)
		return r;
	*pos += r;
	return r;
}

ssize_t bio_write(struct bio_vec *vec, const char *iobuf, size_t len,
		unsigned long *pos)
{
	ssize_t r = bio_do_io(vec, (char*)iobuf, len, *pos, WRITE);
	if (r < 0)
		return r;
	*pos += r;
	return r;
}

/*
 * Generic VFS read function for filesystems that provide bio_vec block lists
 * to the VFS.
 */
ssize_t bio_file_read(struct file *file, char *buf, size_t len,
		unsigned long *pos)
{
	if (*pos > file->f_inode->i_size)
		return 0;
	len = MIN(len, file->f_inode->i_size - *pos);
	return bio_read(file->f_inode->i_bio, buf, len, pos);
}

/*
 * Sequential block I/O
 */

static ssize_t blkdev_generic_io(dev_t rdev, char *iobuf, size_t len,
		unsigned long *pos, int rw)
{
	struct block_device *dev = get_device(rdev);
	len = MIN(len, dev->sectors*SECTOR_SIZE - *pos);
	blkcnt_t count = io_block_count(dev->blksize, *pos, len);
	blkcnt_t first = io_off_to_block(dev->blksize, *pos);
	unsigned long io_off = io_block_off(dev->blksize, *pos);

	// populate bio_vec with sequential blocks
	struct bio_vec *bio = alloc_bio_vec(rdev, count, dev->blksize);
	for (blkcnt_t i = 0; i < count; i++)
		bio->block[i] = first + i;

	ssize_t r = bio_do_io(bio, iobuf, len, io_off, rw);
	if (r > 0)
		*pos += r;

	free_bio_vec(bio);
	return r;
}

ssize_t blkdev_read(dev_t dev, void *dst, size_t len, unsigned long pos)
{
	return blkdev_generic_io(dev, dst, len, &pos, READ);
}

ssize_t blkdev_write(dev_t dev, void *src, size_t len, unsigned long pos)
{
	return blkdev_generic_io(dev, src, len, &pos, WRITE);
}

static ssize_t blkdev_generic_read(struct file *f, char *dst, size_t len,
		unsigned long *pos)
{
	return blkdev_generic_io(f->f_rdev, dst, len, pos, READ);
}

static ssize_t blkdev_generic_write(struct file *f, const char *src, size_t len,
		unsigned long *pos)
{
	return blkdev_generic_io(f->f_rdev, (char*)src, len, pos, WRITE);
}

int blkdev_generic_open(struct inode *inode, struct file *file)
{
	if (!get_device(inode->i_rdev))
		return -ENXIO;
	return 0;
}

struct file_operations blkdev_generic_fops = {
	.read = blkdev_generic_read,
	.write = blkdev_generic_write,
	.open = blkdev_generic_open,
};

void register_block_driver(unsigned int major, const char *name,
		struct block_device *(*get_device)(unsigned int),
		struct file_operations *fops)
{
	blkdev[major].get_device = get_device;
	register_blkdev(major, name, fops ? fops : &blkdev_generic_fops);
}
