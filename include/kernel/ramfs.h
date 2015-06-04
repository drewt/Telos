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

#ifndef _KERNEL_RAMFS_H_
#define _KERNEL_RAMFS_H_

#include <kernel/fs.h>

ssize_t ramfs_read(struct file *file, char *buf, size_t len, unsigned long *pos);
ssize_t ramfs_write(struct file *file, const char *buf, size_t len, unsigned long *pos);
int ramfs_readdir(struct inode *dir, struct file *file, struct dirent *dirent,
		int count);
int ramfs_create(struct inode *dir, const char *name, int len, int mode,
		struct inode **res_inode);

int ramfs_do_mknod(struct inode *dir, const char *name, int len,
		struct inode **res_inode);
int ramfs_lookup(struct inode *dir, const char *name, int len,
		struct inode **result);
int ramfs_mknod(struct inode *dir, const char *name, int len, int mode,
		dev_t rdev);
int ramfs_mkdir(struct inode *dir, const char *name, int len, int mode);
int ramfs_rmdir(struct inode *dir, const char *name, int len);
int ramfs_unlink(struct inode *dir, const char *name, int len);
int ramfs_link(struct inode *oldinode, struct inode *dir, const char *name,
		int len);
int ramfs_rename(struct inode *old_dir, const char *old_name, int old_len,
		struct inode *new_dir, const char *new_name, int new_len);

struct super_block *ramfs_read_super(struct super_block *sb, void *data,
		int silent);

#endif
