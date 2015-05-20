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
#include <string.h>
#include "ext2.h"

/*
 * If @dst is NULL, get the first dirent in @dir.  Otherwise get the dirent
 * after @dst.  If @dst is the last dirent, NULL is returned.
 */
static struct ext2_dirent *next_dirent(struct inode *dir, unsigned long *off,
		struct ext2_dirent *dst)
{
	do {
		if (*off >= dir->i_size)
			return NULL;
		unsigned long unused = *off;
		bio_read(dir->i_bio, (char*)dst, sizeof(struct ext2_dirent), &unused);
		*off += dst->rec_len;
	} while (!dst->ino);
	return dst;
}

int ext2_lookup(struct inode *dir, const char *name, int len,
		struct inode **result)
{
	struct ext2_dirent dirent;
	unsigned long pos = 0;
	while (next_dirent(dir, &pos, &dirent)) {
		if (dirent.name_len != len)
			continue;
		if (!memcmp(name, dirent.name, len)) {
			*result = iget(dir->i_sb, dirent.ino);
			iput(dir);
			return 0;
		}
	}
	iput(dir);
	return -ENOENT;
}

int ext2_readdir(struct inode *dir, struct file *file, struct dirent *dirent,
		int count)
{
	struct ext2_dirent fs_dirent;
	if (!next_dirent(dir, &file->f_pos, &fs_dirent))
		return -ENOENT;
	dirent->d_ino = fs_dirent.ino;
	memcpy(dirent->d_name, fs_dirent.name, fs_dirent.name_len);
	dirent->d_name[fs_dirent.name_len] = '\0';
	return 0;
}

struct file_operations ext2_dir_fops = {
	.readdir = ext2_readdir,
};

struct inode_operations ext2_dir_iops = {
	.lookup = ext2_lookup,
	.default_file_ops = &ext2_dir_fops,
};
