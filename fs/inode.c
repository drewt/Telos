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
#include <sys/stat.h>
#include <sys/fcntl.h>

static DEFINE_HASHTABLE(inodes, 9);

static DEFINE_SLAB_CACHE(file_cachep, sizeof(struct file));
static DEFINE_SLAB_CACHE(inode_cachep, sizeof(struct inode));

static LIST_HEAD(files);

struct file *get_empty_file(void)
{
	struct file *filp = slab_alloc(file_cachep);
	if (filp) {
		filp->f_count = 1;
		list_add(&filp->chain, &files);
	}
	return filp;
}

void free_file(struct file *filp)
{
	list_del(&filp->chain);
	slab_free(file_cachep, filp);
}

void file_unref(struct file *file)
{
	if (--file->f_count > 0)
		return;
	if (file->f_op && file->f_op->release)
		file->f_op->release(file->f_inode, file);
	iput(file->f_inode);
	free_file(file);
}

struct inode *get_empty_inode(void)
{
	return slab_alloc(inode_cachep);
}

bool fs_may_remount_ro(dev_t dev)
{
	struct file *file;
	list_for_each_entry(file, &files, chain) {
		if (!file->f_count || !file->f_inode || file->f_inode->i_dev != dev)
			continue;
		if (S_ISREG(file->f_inode->i_mode) && file->f_mode & O_WRITE)
			return false;
	}
	return true;
}

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
	inode->i_count = 1;
	inode->i_bio = NULL;
	hash_add(inodes, &inode->i_hash, ino);
	read_inode(inode);
	return inode;
}

void iput(struct inode *inode)
{
	if (!inode)
		return;
	if (--inode->i_count == 0 && !(inode->i_flags & MS_MEMFS)) {
		struct super_operations *ops = inode->i_sb->s_op;
		if (ops && ops->put_inode)
			ops->put_inode(inode);
		hash_del(&inode->i_hash);
		slab_free(inode_cachep, inode);
	}
}
