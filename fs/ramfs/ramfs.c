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

#include <kernel/list.h>
#include <kernel/fs.h>
#include <kernel/ramfs.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <telos/stat.h>

#include <string.h>

struct ramfs_dirent {
	struct list_head chain;
	unsigned long ino;
	char name[256];
};

struct ramfs_superblock {
	unsigned long next_ino;
};

static DEFINE_SLAB_CACHE(ramfs_dirent_cachep, sizeof(struct ramfs_dirent));
static struct inode_operations ramfs_dir_iops;
static struct inode_operations ramfs_reg_iops;

static inline struct ramfs_dirent *__lookup(struct inode *dir,
		const char *name, int len)
{
	struct ramfs_dirent *ent;
	list_for_each_entry(ent, &dir->i_list, chain) {
		if (ent->name[len] == '\0' && !memcmp(name, ent->name, len))
			return ent;
	}
	return NULL;
}

int ramfs_lookup(struct inode *dir, const char *name, int len,
		struct inode **result)
{
	struct ramfs_dirent *ent;

	*result = NULL;
	if (!S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOTDIR;
	}
	if (!(ent = __lookup(dir, name, len))) {
		iput(dir);
		return -ENOENT;
	}

	*result = iget(dir->i_sb, ent->ino);
	iput(dir);
	return 0;
}

static struct inode *ramfs_new_inode(struct inode *dir)
{
	struct inode *inode;
	struct ramfs_superblock *sb = dir->i_sb->s_private;

	if (!(inode = get_empty_inode()))
		return NULL;

	inode->i_ino = sb->next_ino++;
	inode->i_dev = dir->i_sb->s_dev;
	inode->i_count = 1;
	inode->i_dirt = 0;
	inode->i_flags = dir->i_sb->s_flags;
	inode->i_sb = dir->i_sb;
	INIT_LIST_HEAD(&inode->i_list);
	insert_inode_hash(inode);
	return inode;
}

static struct ramfs_dirent *ramfs_new_dirent(const char *name, int len,
		unsigned long ino)
{
	struct ramfs_dirent *fsnode;

	if (!(fsnode = slab_alloc(ramfs_dirent_cachep)))
		return NULL;
	memcpy(fsnode->name, name, len);
	fsnode->ino = ino;

	return fsnode;
}

int ramfs_do_mknod(struct inode *dir, const char *name, int len,
		struct inode **res_inode)
{
	struct inode *inode;
	struct ramfs_dirent *ent = __lookup(dir, name, len);

	*res_inode = NULL;
	if (ent)
		return -EEXIST;
	if (!(inode = ramfs_new_inode(dir)))
		return -ENOSPC;
	if (!(ent = ramfs_new_dirent(name, len, inode->i_ino))) {
		iput(inode);
		return -ENOSPC;
	}

	inode->i_sb = dir->i_sb;
	inode->i_size = 0;

	list_add_tail(&ent->chain, &dir->i_list);
	dir->i_size++;
	*res_inode = inode;
	return 0;
}

static int grow_file(struct inode *inode, size_t amount)
{
	unsigned int nr_pages;

	nr_pages = (((inode->i_size % FRAME_SIZE) + amount) / FRAME_SIZE) + 1;
	if (nr_pages == 0) {
		inode->i_size += amount;
		return 0;
	}

	for (unsigned int i = 0; i < nr_pages; i++) {
		struct pf_info *frame = kalloc_frame(0);
		if (frame == NULL)
			return -ENOSPC; // TODO: free frames!!!
		list_add_tail(&frame->chain, &inode->i_list);
	}
	inode->i_size += amount;
	return 0;
}

int ramfs_read(struct file *file, char *buf, size_t len, unsigned long *pos)
{
	char *fbuf;
	struct pf_info *frame;
	unsigned long offset = *pos % FRAME_SIZE;
	unsigned int frame_nr = *pos / FRAME_SIZE;
	unsigned int i = 0;

	if (!len || *pos >= file->f_inode->i_size)
		return 0;
	len = MIN(len, file->f_inode->i_size - *pos);

	list_for_each_entry(frame, &file->f_inode->i_list, chain) {
		if (i++ < frame_nr)
			continue;

		fbuf = kmap_tmp_page(frame->addr);
		memcpy(buf, fbuf+offset, MIN(FRAME_SIZE-offset, len));
		kunmap_page(fbuf);

		if (len <= FRAME_SIZE)
			break;
		buf += FRAME_SIZE - offset;
		len -= FRAME_SIZE - offset;
		offset = 0;
	}
	*pos += len;
	return len;
}

static int ramfs_do_write(struct file *f, const char *buf, size_t len,
		unsigned long pos)
{
	char *fbuf;
	struct pf_info *frame;
	unsigned long offset = pos % FRAME_SIZE;
	unsigned int frame_nr = pos / FRAME_SIZE;
	unsigned int i = 0;

	list_for_each_entry(frame, &f->f_inode->i_list, chain) {
		if (i++ < frame_nr)
			continue;

		fbuf = kmap_tmp_page(frame->addr);
		memcpy(fbuf+offset, buf, MIN(FRAME_SIZE-offset, len));
		kunmap_page(fbuf);

		if (len <= FRAME_SIZE)
			break;
		buf += FRAME_SIZE - offset;
		len -= FRAME_SIZE - offset;
		offset = 0;
	}
	return len;
}

static void zero_fill(struct inode *inode, unsigned long start, size_t len)
{
	char *fbuf;
	struct pf_info *frame;
	unsigned long offset = start % FRAME_SIZE;
	unsigned long frame_nr = start / FRAME_SIZE;
	unsigned int i = 0;

	list_for_each_entry(frame, &inode->i_list, chain) {
		if (i++ < frame_nr)
			continue;

		fbuf = kmap_tmp_page(frame->addr);
		memset(fbuf+offset, 0, MIN(FRAME_SIZE-offset, len));
		kunmap_page(fbuf);

		if (len <= FRAME_SIZE)
			break;
		len -= FRAME_SIZE - offset;
		offset = 0;
	}
}

int ramfs_write(struct file *file, const char *buf, size_t len,
		unsigned long *pos)
{
	int error, rc;
	unsigned long next_pos = *pos + len;
	unsigned long prev_size = file->f_inode->i_size;

	if (next_pos > file->f_inode->i_size) {
		error = grow_file(file->f_inode, next_pos - file->f_inode->i_size);
		if (error)
			return error;
	}

	// fill gap with zeros if we've seeked past EOF
	if (*pos > prev_size)
		zero_fill(file->f_inode, prev_size, *pos - prev_size);

	rc = ramfs_do_write(file, buf, len, *pos);
	if (rc > 0)
		*pos += rc;
	return rc;
}

int ramfs_truncate(struct inode *inode, size_t len)
{
	unsigned int nr_pages = pages_in_range(0, len);
	unsigned int old_nr_pages = pages_in_range(0, inode->i_size);
	int page_diff = nr_pages - old_nr_pages;
	ssize_t diff = len - inode->i_size;

	if (diff > 0) {
		unsigned long prev_size = inode->i_size;
		int error = grow_file(inode, diff);
		if (error)
			return error;
		zero_fill(inode, prev_size, len);
		return 0;
	} else for (; page_diff < 0; page_diff++) {
		struct pf_info *frame;
		if (list_empty(&inode->i_list)) {
			warn("ramfs: expected more frames in truncate");
			break;
		}
		frame = list_entry(inode->i_list.prev, struct pf_info, chain);
		list_del(&frame->chain);
		kfree_frame(frame);
	}
	return 0;

}

int ramfs_readdir(struct inode *dir, struct file *file, struct dirent *dirent,
		int count)
{
	struct ramfs_dirent *ent;
	unsigned long i = 0;

	if (dirent->d_off >= dir->i_size)
		return -ENOENT; // ???

	list_for_each_entry(ent, &dir->i_list, chain) {
		if (i == dirent->d_off)
			break;
		i++;
	}

	dirent->d_off++;
	dirent->d_ino = ent->ino;
	memcpy(dirent->d_name, ent->name, 256);
	return 0;
}

int ramfs_create(struct inode *dir, const char *name, int len, int mode,
		struct inode **res_inode)
{
	struct inode *inode;
	int error;

	*res_inode = NULL;
	error = ramfs_do_mknod(dir, name, len, &inode);
	if (error)
		return error;

	inode->i_rdev = 0;
	inode->i_mode = S_IFREG | mode;
	inode->i_nlink = 1;
	inode->i_op = &ramfs_reg_iops;
	iput(dir);
	*res_inode = inode;
	return 0;
}

int ramfs_mknod(struct inode *dir, const char *name, int len, int mode,
		dev_t rdev)
{
	struct inode *inode;
	int error;

	error = ramfs_do_mknod(dir, name, len, &inode);
	if (error)
		return error;

	inode->i_rdev = rdev;
	inode->i_mode = mode;
	inode->i_nlink = 1;
	if (S_ISREG(mode))
		inode->i_op = &ramfs_reg_iops;
	else if (S_ISCHR(mode))
		inode->i_op = &chrdev_inode_operations;
	else if (S_ISBLK(mode))
		inode->i_op = &blkdev_inode_operations;
	iput(dir);
	iput(inode);
	return 0;
}

static int mkdir_links(struct inode *parent_dir, struct inode *dir)
{
	struct ramfs_dirent *this, *parent;

	if (!(this = ramfs_new_dirent(".", 2, dir->i_ino)))
		return -ENOSPC;
	if (!(parent = ramfs_new_dirent("..", 3, parent_dir->i_ino))) {
		slab_free(ramfs_dirent_cachep, this);
		return -ENOSPC;
	}
	list_add(&parent->chain, &dir->i_list);
	list_add(&this->chain, &dir->i_list);
	dir->i_size += 2;
	return 0;
}

int ramfs_mkdir(struct inode *dir, const char *name, int len, int mode)
{
	int error;
	struct inode *inode;

	error = ramfs_do_mknod(dir, name, len, &inode);
	if (error)
		return error;
	error = mkdir_links(dir, inode);
	if (error) {
		iput(inode);
		return error;
	}

	inode->i_mode = S_IFDIR | mode;
	inode->i_op = &ramfs_dir_iops;
	inode->i_nlink = 2;
	dir->i_nlink++;
	iput(dir);
	iput(inode);
	return 0;
}

int ramfs_rmdir(struct inode *dir, const char *name, int len)
{
	int retval;
	struct inode *inode = NULL;
	struct ramfs_dirent *ent = __lookup(dir, name, len);

	if (!ent) {
		retval = -ENOENT;
		goto end;
	}
	retval = -EPERM;
	if (!(inode = iget(dir->i_sb, ent->ino)))
		goto end;
	if (inode->i_dev != dir->i_dev)
		goto end;
	if (inode == dir)
		goto end;
	if (!S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
		goto end;
	}
	if (inode->i_size > 2) {
		retval = -ENOTEMPTY;
		goto end;
	}
	if (inode->i_count > 2) {
		retval = -EBUSY;
		goto end;
	}

	list_del(&ent->chain);
	slab_free(ramfs_dirent_cachep, ent);
	dir->i_size--;
	inode->i_count--;
	retval = 0;
end:
	iput(dir);
	iput(inode);
	return retval;
}

int ramfs_unlink(struct inode *dir, const char *name, int len)
{
	int retval;
	struct inode *inode = NULL;
	struct ramfs_dirent *ent = __lookup(dir, name, len);

	retval = -ENOENT;
	if (!ent)
		goto end;
	if (!(inode = iget(dir->i_sb, ent->ino)))
		goto end;
	retval = -EPERM;
	if (S_ISDIR(inode->i_mode))
		goto end;
	if (--inode->i_nlink == 0)
		inode->i_flags &= ~MS_MEMFS; // inode may be freed
	dir->i_size--;
	list_del(&ent->chain);
	slab_free(ramfs_dirent_cachep, ent);
	// FIXME-MAYBE: should dirent free wait for iput()?
	retval = 0;
end:
	iput(inode);
	iput(dir);
	return retval;
}

int ramfs_link(struct inode *oldinode, struct inode *dir, const char *name,
		int len)
{
	struct ramfs_dirent *dirent;

	if (__lookup(dir, name, len)) {
		iput(dir);
		iput(oldinode);
		return -EEXIST;
	}

	if (!(dirent = ramfs_new_dirent(name, len, oldinode->i_ino))) {
		iput(dir);
		iput(oldinode);
		return -ENOSPC;
	}

	list_add_tail(&dirent->chain, &dir->i_list);
	dir->i_size++;
	oldinode->i_nlink++;
	iput(dir);
	iput(oldinode);
	return 0;
}

int ramfs_rename(struct inode *old_dir, const char *old_name, int old_len,
		struct inode *new_dir, const char *new_name, int new_len)
{
	struct ramfs_dirent *fsnode;

	if (!(fsnode = __lookup(old_dir, old_name, old_len)))
		return -ENOENT;
	if (__lookup(new_dir, new_name, new_len))
		return -EEXIST;

	list_del(&fsnode->chain);
	list_add_tail(&fsnode->chain, &new_dir->i_list);
	memcpy(fsnode->name, new_name, new_len);
	old_dir->i_size--;
	new_dir->i_size++;
	iput(old_dir);
	iput(new_dir);
	return 0;
}

struct file_operations ramfs_dir_fops = {
	.readdir = ramfs_readdir,
};

static struct inode_operations ramfs_dir_iops = {
	.create = ramfs_create,
	.lookup = ramfs_lookup,
	.link = ramfs_link,
	.unlink = ramfs_unlink,
	.mkdir = ramfs_mkdir,
	.rmdir = ramfs_rmdir,
	.mknod = ramfs_mknod,
	.rename = ramfs_rename,
	.default_file_ops = &ramfs_dir_fops,
};

struct file_operations ramfs_reg_fops = {
	.read = ramfs_read,
	.write = ramfs_write,
};

static struct inode_operations ramfs_reg_iops = {
	.truncate = ramfs_truncate,
	.default_file_ops = &ramfs_reg_fops,
};

struct super_operations ramfs_super_ops = {
	.read_inode = NULL,
};

struct super_block *ramfs_read_super(struct super_block *sb, void *data,
		int silent)
{
	struct inode *root;
	struct ramfs_dirent *fsroot;
	struct ramfs_superblock *fsblock;

	if (!(fsblock = kmalloc(sizeof(struct ramfs_superblock))))
		return NULL;

	if (!(root = get_empty_inode())) {
		kfree(fsblock);
		return NULL;
	}

	if (!(fsroot = slab_alloc(ramfs_dirent_cachep))) {
		kfree(fsblock);
		iput(root);
		return NULL;
	}

	fsblock->next_ino = 1;

	sb->s_flags |= MS_MEMFS;
	root->i_flags = sb->s_flags;
	root->i_ino = 0;
	root->i_mode = S_IFDIR;
	root->i_nlink = 2;
	root->i_dev = sb->s_dev;
	root->i_count = 1;
	root->i_dirt = 0;
	root->i_sb = sb;
	root->i_op = &ramfs_dir_iops;
	INIT_LIST_HEAD(&root->i_list);
	insert_inode_hash(root);

	fsroot->name[0] = '/';
	fsroot->name[1] = '\0';
	fsroot->ino = root->i_ino;

	if (mkdir_links(root, root))
		return NULL;

	sb->s_mounted = root;
	sb->s_op = &ramfs_super_ops;
	sb->s_private = fsblock;
	return sb;
}

SYSINIT(ramfs, SUB_VFS)
{
	register_filesystem(ramfs_read_super, "ramfs", 0);
}
