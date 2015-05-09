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
#include <kernel/wait.h>
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
 * Wait until a buffer is up to date.
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

static struct request *make_request(int rw, struct buffer *buf)
{
	struct request *req = slab_alloc(request_cachep);
	if (!req)
		panic("No memory for block request");
	// FIXME: block until memory available
	req->buf = buf;
	req->rw = rw;
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
		dev->handle_request(req);
	else
		list_add_tail(&req->chain, &dev->requests);
	return 0;
}

/*
 * Block drivers call this function to signal completion of an I/O request.
 */
void block_request_completed(struct block_device *dev, struct request *req)
{
	struct request *next;
	req->buf->b_flags |= BUF_UPTODATE;
	buffer_unlock(req->buf);
	if (request_queue_empty(dev))
		return;
	next = list_first_entry(&dev->requests, struct request, chain);
	list_del(&next->chain);
	dev->handle_request(next);
}

/*
 * Generic block device file operations.
 */

static ssize_t blkdev_generic_io(struct file *f, char *dst, size_t len,
		unsigned long *pos, void (*do_io)(char*, char*, size_t))
{
	size_t nr_bytes = 0;
	struct block_device *dev = get_device(f->f_rdev);
	len = MIN(len, dev->blksize*dev->blkcount - *pos);

	blkcnt_t blocks = io_block_count(dev->blksize, *pos, len);
	blkcnt_t first_block = io_off_to_block(dev->blksize, *pos);
	off_t start = *pos - io_block_start(dev->blksize, *pos);
	off_t end = MIN(dev->blksize, start + len);

	/*
	 * At each iteration of the loop, we do I/O on a single block.  @start
	 * and @end give the range of data within the block that is part of the
	 * I/O request.  @nr_bytes tracks the number of bytes completed so far.
	 */
	for (blkcnt_t b = first_block; b - first_block < blocks; b++) {
		// do I/O on a single block
		struct buffer *buf = read_block(f->f_rdev, b, dev->blksize);
		if (!buf)
			return -EIO;
		do_io(dst + nr_bytes, buf->b_data + start, end - start);
		release_buffer(buf);

		// update counts
		nr_bytes += end - start;
		start = 0;
		end = MIN(dev->blksize, len - nr_bytes);
	}
	*pos += nr_bytes;
	return nr_bytes;
}

static void do_read(char *dst, char *src, size_t len)
{
	memcpy(dst, src, len);
}

static void do_write(char *src, char *dst, size_t len)
{
	memcpy(dst, src, len);
}

static ssize_t blkdev_generic_read(struct file *f, char *dst, size_t len,
		unsigned long *pos)
{
	return blkdev_generic_io(f, dst, len, pos, do_read);
}

static ssize_t blkdev_generic_write(struct file *f, const char *buf, size_t len,
		unsigned long *pos)
{
	return blkdev_generic_io(f, (char*)buf, len, pos, do_write);
}

struct file_operations blkdev_generic_fops = {
	.read = blkdev_generic_read,
	.write = blkdev_generic_write,
};

void register_block_driver(unsigned int major, const char *name,
		struct block_device *(*get_device)(unsigned int),
		struct file_operations *fops)
{
	blkdev[major].get_device = get_device;
	register_blkdev(major, name, fops ? fops : &blkdev_generic_fops);
}
