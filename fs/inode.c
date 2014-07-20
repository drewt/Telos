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
#include <kernel/mm/slab.h>

static DEFINE_HASHTABLE(inodes, 9);

static void write_inode(struct inode *inode)
{
	if (!inode->i_dirt)
		return;
	if (!inode->i_sb || !inode->i_sb->s_op || !inode->i_sb->s_op->write_inode) {
		inode->i_dirt = 0;
		return;
	}
	inode->i_sb->s_op->write_inode(inode);
}

static void read_inode(struct inode *inode)
{
	if (inode->i_sb && inode->i_sb->s_op && inode->i_sb->s_op->read_inode)
		inode->i_sb->s_op->read_inode(inode);
}

void insert_inode_hash(struct inode *inode)
{
	hash_add(inodes, &inode->i_hash, inode->i_ino);
}

struct inode *iget(struct super_block *sb, ino_t ino)
{
	return __iget(sb, ino, true);
}

struct inode *__iget(struct super_block *sb, ino_t ino, bool crossmntp)
{
	struct inode *inode;

	if (!sb)
		panic("VFS: iget with sb==NULL");

	hash_for_each_possible(inodes, inode, i_hash, ino) {
		if (inode->i_ino == ino && inode->i_sb == sb) {
			inode->i_count++;
			if (crossmntp && inode->i_mount) {
				struct inode *tmp = inode->i_mount;
				tmp->i_count++;
				iput(inode);
				inode = tmp;
			}
			return inode;
		}
	}

	if ((inode = slab_alloc(inode_cachep)) == NULL)
		return NULL;

	inode->i_sb = sb;
	inode->i_dev = sb->s_dev;
	inode->i_ino = ino;
	inode->i_flags = sb->s_flags;
	hash_add(inodes, &inode->i_hash, ino);
	read_inode(inode);
	return inode;
}

void iput(struct inode *inode)
{
	if (!inode)
		return;
	if (--inode->i_count == 0 && !(inode->i_flags & MS_MEMFS)) {
		kprintf("FREEING INODE: %lu\n", inode->i_ino);
		slab_free(inode_cachep, inode);
	}
}

static void fs_file_sysinit(void)
{
	file_cachep = slab_cache_create(sizeof(struct file));
	inode_cachep = slab_cache_create(sizeof(struct inode));
}
EXPORT_KINIT(fs_file, SUB_VFS, fs_file_sysinit);
