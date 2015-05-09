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

#ifndef _DRIVERS_BLOCK_H_
#define _DRIVERS_BLOCK_H_

#include <kernel/fs.h>
#include <kernel/list.h>
#include <kernel/log2.h>

// TODO: allow multi-block requests
struct request {
	struct list_head chain;
	struct buffer *buf;
	int rw;
};

struct block_device {
	struct list_head requests;
	void (*handle_request)(struct request*);
	unsigned long blksize;
	unsigned long blkcount;
};

struct block_driver {
	struct block_device *(*get_device)(unsigned int);
};

extern struct file_operations blkdev_generic_fops;

static inline void INIT_BLOCK_DEVICE(struct block_device *dev,
		void (*handle_request)(struct request*),
		unsigned long blksize, unsigned long blkcount)
{
	INIT_LIST_HEAD(&dev->requests);
	dev->handle_request = handle_request;
	dev->blksize = blksize;
	dev->blkcount = blkcount;
}

static inline bool request_queue_empty(struct block_device *dev)
{
	return list_empty(&dev->requests);
}

static inline off_t io_block_start(blksize_t blksize, unsigned long off)
{
	return off & ~(blksize-1);
}

static inline off_t io_block_to_off(blksize_t blksize, blkcnt_t blocknr)
{
	return blocknr << ilog2(blksize);
}

static inline blkcnt_t io_off_to_block(blksize_t blksize, off_t off)
{
	return off >> ilog2(blksize);
}

static inline blkcnt_t io_block_count(blksize_t blksize, size_t off, size_t len)
{
	len = (off + len) - io_block_start(blksize, off);
	return (len + blksize - 1) >> ilog2(blksize);
}

void block_request_completed(struct block_device *dev, struct request *req);
void register_block_driver(unsigned int major, const char *name,
		struct block_device *(*get_device)(unsigned int),
		struct file_operations *fops);

#endif
