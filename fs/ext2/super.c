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
#include "ext2.h"

/*
 * We store the whole ext2 super block and block group descriptor table in
 * memory.
 *
 * FIXME: block group descriptor table may span multiple blocks
 */
struct ext2_sb_private {
	struct ext2_superblock *sb;
	struct ext2_bg_descriptor *bg_table;
	struct buffer *sb_buf;
	struct buffer *bg_buf;
};

/*
 * Get the ext2 super block from the VFS super block.
 */
struct ext2_superblock *ext2_superblock(struct super_block *sb)
{
	return ((struct ext2_sb_private*)sb->s_private)->sb;
}

/*
 * Get the number of inode structures per filesystem block in the inode table.
 */
static int inodes_per_block(struct ext2_superblock *sb)
{
	return (1024 << sb->log_block_size) / sizeof(struct ext2_inode);
}

/*
 * Get the block group number corresponding to an inode number.
 */
static uint32_t inode_block_group_nr(struct ext2_superblock *sb, uint32_t ino)
{
	return (ino - 1) / sb->inodes_per_group;
}

/*
 * Get the local index into the inode table for a given inode number (this is
 * the index into the portion of the inode table stored in the corresponding
 * block group).
 */
static uint32_t inode_local_index(struct ext2_superblock *sb, uint32_t ino)
{
	return (ino - 1) % sb->inodes_per_group;
}

/*
 * Get the filesystem block size.
 */
static blksize_t ext2_block_size(struct ext2_superblock *sb)
{
	return 1024 << sb->log_block_size;
}

/*
 * Get the number of blocks groups for a given filesystem.
 */
static blkcnt_t block_groups_count(struct ext2_superblock *sb)
{
	return sb->blocks_count / sb->blocks_per_group 
		+ (sb->blocks_count % sb->blocks_per_group ? 1 : 0);
}

/*
 * Get the block group descriptor for a given inode number.
 */
static struct ext2_bg_descriptor *ino_to_bg(struct ext2_sb_private *private,
		uint32_t ino)
{
	return &private->bg_table[inode_block_group_nr(private->sb, ino)];
}

struct ext2_inode *ext2_iget(struct super_block *vsb, ino_t ino,
		struct buffer **buf)
{
	// unpack ext2 private data
	struct ext2_sb_private *private = vsb->s_private;
	struct ext2_superblock *sb = private->sb;
	struct ext2_bg_descriptor *bg = ino_to_bg(private, ino);

	// number of inode structures per block of the inode table
	uint32_t inodes_per_block = ext2_block_size(sb) / sizeof(struct ext2_inode);

	// index into the block group's inode table
	uint32_t local_index = inode_local_index(sb, ino);

	// block offset into the inode table
	uint32_t table_block = local_index / inodes_per_block;

	// index into table_block
	uint32_t block_index = local_index % inodes_per_block;

	// read the block containing the inode
	// FIXME: check for IO error
	struct buffer *ibuf = read_block(vsb->s_dev,
			bg->inode_table + table_block,
			ext2_block_size(sb));

	// index into inode table
	struct ext2_inode *inode_table = ibuf->b_data;
	return &inode_table[block_index];
}

struct super_operations ext2_super_ops = {
	.read_inode = ext2_read_inode,
	.put_inode = ext2_put_inode,
};

static struct ext2_sb_private *alloc_sb_private(void)
{
	struct ext2_sb_private *p = kmalloc(sizeof(struct ext2_sb_private));
	if (p) {
		p->sb_buf = NULL;
		p->bg_buf = NULL;
	}
	return p;
}

static void free_sb_private(struct ext2_sb_private *private)
{
	release_buffer(private->sb_buf);
	release_buffer(private->bg_buf);
	kfree(private);
}

static int do_read_super(struct super_block *sb, struct ext2_sb_private *private)
{
	blksize_t blksize = blkdev_blksize(sb->s_dev);
	blkcnt_t sb_block = blksize == 1024 ? 1 : 0;
	blkcnt_t bg_block = blksize == 1024 ? 2 : 1;

	// read the super block and block group descriptor table
	private->sb_buf = read_block(sb->s_dev, sb_block, blksize);
	private->bg_buf = read_block(sb->s_dev, bg_block, blksize);

	if (!private->sb_buf || !private->bg_buf) {
		release_buffer(private->sb_buf);
		release_buffer(private->bg_buf);
		return -EIO;
	}

	private->sb = private->sb_buf->b_data;
	private->bg_table = private->bg_buf->b_data;

	return 0;
}

struct super_block *ext2_read_super(struct super_block *sb, void *data,
		int silent)
{
	struct ext2_superblock tmp;
	struct ext2_sb_private *private;
	if (!(private = alloc_sb_private()))
		goto fail;

	blkdev_read(sb->s_dev, &tmp, sizeof(struct ext2_superblock), 1024);
	if (set_blocksize(sb->s_dev, ext2_block_size(&tmp))) {
		if (!silent)
			kprintf("Unable to set block size on device\n");
		goto fail;
	}

	if (do_read_super(sb, private) < 0) {
		if (!silent)
			kprintf("error reading superblock\n");
		goto fail;
	}

	sb->s_op = &ext2_super_ops;
	sb->s_private = private;
	sb->s_mounted = iget(sb, EXT2_ROOT_INO);
	return sb;
fail:
	free_sb_private(private);
	return NULL;
}

SYSINIT(ext2, SUB_VFS)
{
	register_filesystem(ext2_read_super, "ext2", 1);
}
