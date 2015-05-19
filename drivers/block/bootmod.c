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
#include <kernel/multiboot.h>
#include <sys/major.h>
#include <string.h>
#include "block.h"

/*
 * Multiboot module block device driver.
 */

#define MAX_MODDEV 16

#define MOD_BLKSIZE 1024

struct mod_device {
	struct block_device blkdev;
	unsigned char *mem;
	off_t len;
};

struct mod_device moddev[MAX_MODDEV];

static void handle_request(struct request *req)
{
	struct mod_device *dev = &moddev[minor(req->buf->b_dev)];
	off_t off = req->buf->b_blocknr * req->buf->b_size;
	off_t len = MIN(req->buf->b_size, dev->len - off);
	if (req->rw == READ) {
		memcpy(req->buf->b_data, dev->mem+off, len);
		if (len != req->buf->b_size)
			memset(req->buf->b_data, 0, req->buf->b_size - len);
	} else {
		memcpy(dev->mem+off, req->buf->b_data, len);
	}
	block_request_completed(&dev->blkdev, req);
}

static struct block_device *get_device(unsigned int minor)
{
	return &moddev[minor].blkdev;
}

/*
 * Create a boot module device.  Returns the minor number of the created
 * device, or -1 if MAX_MODDEV devices are already registered.
 */
int register_moddev(struct multiboot_mod_list *mod)
{
	int i;
	for (i = 0; i < MAX_MODDEV; i++) {
		if (!moddev[i].mem)
			break;
	}
	if (i == MAX_MODDEV)
		return -1;

	blkcnt_t blocks = (mod->end - mod->start) / MOD_BLKSIZE;
	if ((mod->end - mod->start) % MOD_BLKSIZE)
		blocks++;
	INIT_BLOCK_DEVICE(&moddev[i].blkdev, handle_request, MOD_BLKSIZE, blocks);
	moddev[i].mem = (void*)mod->start;;
	moddev[i].len = mod->end - mod->start;
	return i;
}

SYSINIT(modblk, SUB_DRIVER)
{
	register_block_driver(MOD_MAJOR, "mod", get_device, NULL);

	struct multiboot_mod_list *mod = (void*)mb_info->mods_addr;
	for (unsigned long i = 0; i < mb_info->mods_count; i++, mod++) {
		if (register_moddev(mod) < 0)
			break;
	}
}
