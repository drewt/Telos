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
#include <kernel/ramfs.h>
#include <kernel/multiboot.h>
#include <telos/stat.h>
#include <string.h>

static int modfs_read(struct file *file, char *buf, size_t len,
		unsigned long *pos)
{
	struct multiboot_mod_list *mod = file->f_inode->i_private;

	len = MIN(len, file->f_inode->i_size - *pos);
	memcpy(buf, (void*)(mod->start + *pos), len);
	*pos += len;
	return len;
}

static struct file_operations modfs_dir_fops = {
	.readdir = ramfs_readdir,
};

static struct inode_operations modfs_dir_iops = {
	.lookup = ramfs_lookup,
	.default_file_ops = &modfs_dir_fops,
};

static struct file_operations modfs_reg_fops = {
	.read = modfs_read,
};

static struct inode_operations modfs_reg_iops = {
	.default_file_ops = &modfs_reg_fops,
};

static int mkmod(struct inode *dir, struct multiboot_mod_list *mod,
		unsigned long nr)
{
	int error, len;
	struct inode *inode;
	char name[8];

	len = sprintf(name, "mod%lu", nr);
	if (len < 0)
		return len;

	error = ramfs_do_mknod(dir, name, len, &inode);
	if (error)
		return error;

	inode->i_rdev = 0;
	inode->i_mode = S_IFREG;
	inode->i_nlink = 1;
	inode->i_size = mod->end - mod->start;
	inode->i_op = &modfs_reg_iops;
	inode->i_private = mod;
	return 0;
}

static struct super_block *modfs_read_super(struct super_block *sb, void *data,
		int silent)
{
	int error;
	struct multiboot_mod_list *mod;

	if (!(sb = ramfs_read_super(sb, data, silent)))
		return NULL;
	sb->s_mounted->i_op = &modfs_dir_iops;

	if (!MULTIBOOT_MODS_VALID(mb_info))
		return sb;

	mod = (void*)mb_info->mods_addr;
	for (ulong i = 0; i < mb_info->mods_count; i++, mod++) {
		error = mkmod(sb->s_mounted, mod, i);
		if (error)
			return NULL;
	}

	return sb;
}

SYSINIT(modfs, SUB_VFS)
{
	register_filesystem(modfs_read_super, "modfs", 0);
}
