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
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/slab.h>
#include <telos/stat.h>
#include "ext2.h"

/*
 * We store the whole ext2 inode in i_private.  This wastes a bit of memory,
 * but it's likely that the buffer will remain cached anyways.
 */
struct ext2_inode_private {
	struct ext2_inode *inode;
	struct buffer *buf;
};
static DEFINE_SLAB_CACHE(inode_private_cachep, sizeof(struct ext2_inode_private));

/*
 * Get the ext2 inode from a VFS inode.
 */
struct ext2_inode *ext2_inode(struct inode *vnode)
{
	return ((struct ext2_inode_private*)vnode->i_private)->inode;
}

/*
 * Get the inode_operations corresponding to a file mode.
 */
static struct inode_operations *ext2_mode_iops(mode_t mode)
{
	if (S_ISREG(mode))
		return &ext2_reg_iops;
	if (S_ISDIR(mode))
		return &ext2_dir_iops;
	if (S_ISCHR(mode))
		return &chrdev_inode_operations;
	if (S_ISBLK(mode))
		return &blkdev_inode_operations;
	panic("ext2: bad mode: %lx", mode); // FIXME
}

void ext2_read_inode(struct inode *vnode)
{
	// TODO: handle ENOMEM
	struct ext2_inode_private *private = slab_alloc(inode_private_cachep);
	private->inode = ext2_iget(vnode->i_sb, vnode->i_ino,
			&private->buf);

	// fill out VFS inode from ext2 inode
	vnode->i_mode = private->inode->mode; // TODO: check these
	if (S_ISCHR(vnode->i_mode) || S_ISBLK(vnode->i_mode))
		vnode->i_rdev = private->inode->block[0];
	vnode->i_size = private->inode->size;
	vnode->i_nlink = private->inode->links_count;
	vnode->i_op = ext2_mode_iops(vnode->i_mode);
	vnode->i_private = private;
	vnode->i_bio = ext2_inode_to_bio_vec(vnode);
}

void ext2_put_inode(struct inode *vnode)
{
	struct ext2_inode_private *private = vnode->i_private;
	release_buffer(private->buf);
	slab_free(inode_private_cachep, private);
	free_bio_vec(vnode->i_bio);
}

/*
 * Read block numbers from an indirect block (given by @block) into the flat
 * array @blocks.
 *
 */
static unsigned buffers_from_indirect(dev_t dev, uint32_t block,
		unsigned blksize, blkcnt_t *blocks, unsigned len)
{
	len = MIN(len, blksize / sizeof(uint32_t));

	struct buffer *ibuf = read_block(dev, block, blksize); 
	for (unsigned i = 0; i < len; i++) {
		uint32_t block = ((uint32_t*)ibuf->b_data)[i];
		blocks[i] = block;
	}
	release_buffer(ibuf);
	return len;
}

/*
 * Read block numbers from a doubly-indirect block (given by @block) into the
 * flat array @blocks.
 */
static unsigned buffers_from_indirect2(dev_t dev, uint32_t block,
		unsigned blksize, blkcnt_t *blocks, unsigned len)
{
	const unsigned max_indirect = blksize / sizeof(uint32_t);
	len = MIN(len, max_indirect*max_indirect);

	struct buffer *i2buf = read_block(dev, block, blksize);
	for (unsigned i = 0; i*max_indirect < len; i++) {
		uint32_t block = ((uint32_t*)i2buf->b_data)[i];
		buffers_from_indirect(dev, block, blksize,
				blocks + i*max_indirect, len - i*max_indirect);
	}
	release_buffer(i2buf);
	return len;
}

/*
 * Read block numbers from a triply-indirect block (given by @block) into the
 * flat array @blocks.
 */
static unsigned buffers_from_indirect3(dev_t dev, uint32_t block,
		unsigned blksize, blkcnt_t *blocks, unsigned len)
{
	const unsigned max_indirect = blksize / sizeof(uint32_t);
	const unsigned max_indirect2 = max_indirect*max_indirect;
	len = MIN(len, max_indirect*max_indirect2);

	struct buffer *i3buf = read_block(dev, block, blksize);
	for (unsigned i = 0; i*max_indirect2 < len; i++) {
		uint32_t block = ((uint32_t*)i3buf->b_data)[i];
		buffers_from_indirect2(dev, block, blksize,
				blocks + i*max_indirect2, len - i*max_indirect2);
	}
	release_buffer(i3buf);
	return len;
}

/*
 * Flatten an ext2 inode block list into a bio_vec.
 */
struct bio_vec *ext2_inode_to_bio_vec(struct inode *vnode)
{
	struct ext2_superblock *sb = ext2_superblock(vnode->i_sb);
	struct ext2_inode *inode = ext2_inode(vnode);
	const blksize_t blksize = blkdev_blksize(vnode->i_dev);
	const uint32_t nr_blocks = ext2_inode_blocks(sb, inode);
	struct bio_vec *bio = alloc_bio_vec(vnode->i_dev, nr_blocks, blksize);
	unsigned i = 0;

	// direct blocks
	for (; i < 12 && i < nr_blocks; i++) {
		bio->block[i] = inode->block[i];
	}
	if (i == nr_blocks)
		return bio;

	// indirect blocks
	i += buffers_from_indirect(vnode->i_dev, inode->block[12], blksize,
			&bio->block[i], nr_blocks - i);
	if (i == nr_blocks)
		return bio;

	// doubly-indirect blocks
	i += buffers_from_indirect2(vnode->i_dev, inode->block[13], blksize,
			&bio->block[i], nr_blocks - i);
	if (i == nr_blocks)
		return bio;

	// triply-indirect blocks
	i += buffers_from_indirect3(vnode->i_dev, inode->block[14], blksize,
			&bio->block[i], nr_blocks - i);
	return bio;
}
