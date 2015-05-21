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

/*
 *  linux/fs/super.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/stat.h>
#include <sys/major.h>
#include <sys/mount.h>
#include <string.h>

#define SBT_SIZE 100

#define ROOT_DEV makedev(MOD_MAJOR, 0)
#define ROOT_FS  "ext2"

static int root_mountflags = 0;

extern struct file_operations * get_blkfops(unsigned int);

struct super_block super_blocks[SBT_SIZE];

struct inode *root_inode;

static struct super_block *get_super(dev_t dev)
{
	if (!dev)
		return NULL;

	for (unsigned i = 0; i < SBT_SIZE; i++)
		if (super_blocks[i].s_dev == dev)
			return &super_blocks[i];
	return NULL;
}

void put_super(dev_t dev)
{
	struct super_block *sb;

	if (dev == ROOT_DEV)
		return;
	if (!(sb = get_super(dev)))
		return;
	if (sb->s_covered)
		return;
	if (sb->s_op && sb->s_op->put_super)
		sb->s_op->put_super(sb);
}

static struct super_block *read_super(dev_t dev, const char *name, int flags,
        void *data, int silent)
{
	struct super_block *sb;
	struct file_system_type *fs_type;

	if (!dev)
		return NULL;

	if ((sb = get_super(dev)))
		return sb;
	if (!(fs_type = get_fs_type(name)))
		return NULL;

	for (sb = super_blocks;; sb++) {
		if (sb >= SBT_SIZE + super_blocks)
			return NULL;
		if (!sb->s_dev)
			break;
	}
	sb->s_dev = dev;
	sb->s_flags = flags;
	if (!fs_type->read_super(sb, data, silent)) {
		sb->s_dev = 0;
		return NULL;
	}
	sb->s_dev = dev;
	sb->s_covered = NULL;
	sb->s_dirt = 0;
	return sb;
}

static char unnamed_dev_in_use[256];

static dev_t get_unnamed_dev(void)
{
	static int first_use = 0;
	unsigned i;

	if (first_use == 0) {
		first_use = 1;
		memset(unnamed_dev_in_use, 0, sizeof(unnamed_dev_in_use));
		unnamed_dev_in_use[0] = 1; /* minor 0 (nodev) is special */
	}
	for (i = 0; i < sizeof unnamed_dev_in_use/sizeof unnamed_dev_in_use[0]; i++) {
		if (unnamed_dev_in_use[i] == 0) {
			unnamed_dev_in_use[i] = 1;
			return makedev(UNNAMED_MAJOR, i);
		}
	}
	return 0;
}

static void put_unnamed_dev(dev_t dev)
{
	if (!dev)
		return;
	unnamed_dev_in_use[dev] = 0;
}

static int do_umount(dev_t dev)
{
	struct super_block * sb;
	//int retval;
	
	if (dev == ROOT_DEV) {
		// Special case for "unmounting" root.  We just try to remount
		//  it readonly, and sync() the device.
		/*if (!(sb=get_super(dev)))
			return -ENOENT;
		if (!(sb->s_flags & MS_RDONLY)) {
			fsync_dev(dev);
			retval = do_remount_sb(sb, MS_RDONLY, 0);
			if (retval)
				return retval;
		}
		return 0;*/
		return -ENOTSUP;
	}
	if (!(sb = get_super(dev)) || !(sb->s_covered))
		return -ENOENT;
	if (!sb->s_covered->i_mount)
		kprintf("VFS: umount: mounted inode has i_mount=NULL\n");
	//if (!fs_may_umount(dev, sb->s_mounted))
	//	return -EBUSY;
	sb->s_covered->i_mount = NULL;
	iput(sb->s_covered);
	sb->s_covered = NULL;
	iput(sb->s_mounted);
	sb->s_mounted = NULL;
	if (sb->s_op && sb->s_op->write_super && sb->s_dirt)
		sb->s_op->write_super(sb);
	put_super(dev);
	return 0;
}

/*
 * Now umount can handle mount points as well as block devices.
 * This is important for filesystems which use unnamed block devices.
 *
 * There is a little kludge here with the dummy_inode.  The current
 * vfs release functions only use the r_dev field in the inode so
 * we give them the info they need without using a real inode.
 * If any other fields are ever needed by any block device release
 * functions, they should be faked here.  -- jrs
 */

long sys_umount(const char *name, size_t name_len)
{
	struct inode * inode;
	dev_t dev;
	int retval, error;
	struct inode dummy_inode;
	struct file_operations * fops;

	error = verify_user_string(name, name_len);
	if (error)
		return error;
	//if (!suser())
	//	return -EPERM;
	retval = namei(name,&inode);
	if (retval) {
		retval = lnamei(name,&inode);
		if (retval)
			return retval;
	}
	if (S_ISBLK(inode->i_mode)) {
		dev = inode->i_rdev;
		if (IS_NODEV(inode)) {
			iput(inode);
			return -EACCES;
		}
	} else {
		if (!inode || !inode->i_sb || inode != inode->i_sb->s_mounted) {
			iput(inode);
			return -EINVAL;
		}
		dev = inode->i_sb->s_dev;
		iput(inode);
		memset(&dummy_inode, 0, sizeof(dummy_inode));
		dummy_inode.i_dev = dev;
		inode = &dummy_inode;
	}
	if (major(dev) >= MAX_BLKDEV) {
		iput(inode);
		return -ENXIO;
	}
	if (!(retval = do_umount(dev)) && dev != ROOT_DEV) {
		fops = get_blkfops(major(dev));
		if (fops && fops->release)
			fops->release(inode,NULL);
		if (major(dev) == UNNAMED_MAJOR)
			put_unnamed_dev(dev);
	}
	if (inode != &dummy_inode)
		iput(inode);
	if (retval)
		return retval;
	//fsync_dev(dev);
	return 0;
}

/*
 * do_mount() does the actual mounting after sys_mount has done the ugly
 * parameter parsing. When enough time has gone by, and everything uses the
 * new mount() parameters, sys_mount() can then be cleaned up.
 *
 * We cannot mount a filesystem if it has active, used, or dirty inodes.
 * We also have to flush all inode-data for this device, as the new mount
 * might need new info.
 */
static int do_mount(dev_t dev, const char * dir, const char * type, int flags,
		void * data)
{
	struct inode *dir_i;
	struct super_block *sb;
	int error;

	error = namei(dir, &dir_i);
	if (error)
		return error;
	if (dir_i->i_count != 1 || dir_i->i_mount) {
		iput(dir_i);
		return -EBUSY;
	}
	if (!S_ISDIR(dir_i->i_mode)) {
		iput(dir_i);
		return -EPERM;
	}
	//if (!fs_may_mount(dev)) {
	//	iput(dir_i);
	//	return -EBUSY;
	//}
	sb = read_super(dev, type, flags, data, 0);
	if (!sb || sb->s_covered) {
		iput(dir_i);
		return -EBUSY;
	}
	sb->s_covered = dir_i;
	dir_i->i_mount = sb->s_mounted;
	return 0; // we don't iput(dir_i) - see umount
}

/*
 * Alters the mount flags of a mounted file system. Only the mount point
 * is used as a reference - file system type and the device are ignored.
 * FS-specific mount options can't be altered by remounting.
 */
static int do_remount_sb(struct super_block *sb, int flags, const void *data)
{
	int error;
	
	// If we are remounting RDONLY, make sure there are no rw files open
	if ((flags & MS_RDONLY) && !(sb->s_flags & MS_RDONLY))
		if (!fs_may_remount_ro(sb->s_dev))
			return -EBUSY;
	if (sb->s_op && sb->s_op->remount_fs) {
		error = sb->s_op->remount_fs(sb, &flags, data);
		if (error)
			return error;
	}
	sb->s_flags = (sb->s_flags & ~MS_RMT_MASK) |
		(flags & MS_RMT_MASK);
	return 0;
}

static int do_remount(const char *dir, int flags, const void *data)
{
	struct inode *dir_i;
	int error;

	error = namei(dir,&dir_i);
	if (error)
		return error;
	if (dir_i != dir_i->i_sb->s_mounted) {
		iput(dir_i);
		return -EINVAL;
	}
	error = do_remount_sb(dir_i->i_sb, flags, data);
	iput(dir_i);
	return error;
}

static long unpacked_mount(const char *dev_name, const char *dir_name,
		const char *type, unsigned long flags, const void *data)
{
	struct file_system_type *fstype;
	struct inode *inode;
	struct file_operations *fops;
	dev_t dev;
	int error;

	if (flags & MS_REMOUNT) {
		return do_remount(dir_name, flags, data);
	}
	flags &= MS_MOUNT_MASK;
	if (!type || !(fstype = get_fs_type(type)))
		return -ENODEV;

	if (fstype->requires_dev) {
		if (!dev_name)
			return -ENOTBLK;
		error = namei(dev_name, &inode);
		if (error)
			return error;
		if (!S_ISBLK(inode->i_mode))
			return -ENOTBLK;
		dev = inode->i_rdev;
	} else {
		if (!(dev = get_unnamed_dev()))
			return -EMFILE;
		inode = NULL;
	}
	fops = get_blkfops(major(dev));
	if (fops && fops->open) {
		if ((error = fops->open(inode, NULL))) {
			iput(inode);
			return error;
		}
	}
	error = do_mount(dev, dir_name, type, flags, NULL);
	if (error && fops && fops->release)
		fops->release(inode, NULL);
	iput(inode);
	return error;
}

long sys_mount(const struct mount *mount)
{
	int error;
	if (vm_verify(&current->mm, mount, sizeof(*mount), VM_READ))
		return -EFAULT;
	// data may be null, or else point to 4096 bytes of readable memory
	if (mount->data && vm_verify(&current->mm, mount->data, 4096, VM_READ))
		return -EFAULT;
	// target is always required
	error = verify_user_string(mount->dir.str, mount->dir.len);
	if (error)
		return error;
	// source may be null (e.g. ramfs)
	if (mount->dev.str) {
		error = verify_user_string(mount->dev.str, mount->dev.len);
		if (error)
			return error;
	}
	// filesystemtype may be null (e.g. remounting a filesytem)
	if (mount->type.str) {
		error = verify_user_string(mount->type.str, mount->type.len);
		if (error)
			return error;
	}
	return unpacked_mount(mount->dev.str, mount->dir.str, mount->type.str,
			mount->flags, mount->data);
}

void mount_root(void)
{
	struct super_block *sb;
	struct inode *inode;

	memset(super_blocks, 0, sizeof(super_blocks));
	if (!(sb = read_super(ROOT_DEV, ROOT_FS, root_mountflags, NULL, 1)))
		panic("VFS: unable to mount root");

	root_inode = inode = sb->s_mounted;
	inode->i_count += 3;
	sb->s_covered = inode;
}
