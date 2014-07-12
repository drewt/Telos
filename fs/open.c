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

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/major.h>
#include <string.h>

struct devfs_inode {
	const char * const path;
	dev_t dev;
	struct inode_operations *iops;
	struct inode *inode;
};

extern struct inode_operations console_iops;
struct devfs_inode devfs[] = {
	{
		.path = "/dev/cons0",
		.dev  = DEVICE(TTY_MAJOR, 0),
		.iops = &console_iops,
	},
	{
		.path = "/dev/cons1",
		.dev  = DEVICE(TTY_MAJOR, 1),
		.iops = &console_iops,
	},
};
#define NR_DEVICES (sizeof(devfs) / sizeof(*devfs))

static struct inode *devfs_get_inode(struct devfs_inode *fsnode)
{
	struct inode *inode;

	if (fsnode->inode)
		return fsnode->inode;

	inode = iget();
	inode->i_op = fsnode->iops;
	inode->i_dev = fsnode->dev;
	return inode;
}

int devfs_namei(const char *path, struct inode **inode)
{
	*inode = NULL;
	for (unsigned devno = 0; devno < NR_DEVICES; devno++) {
		if (!strcmp(path, devfs[devno].path)) {
			*inode = devfs_get_inode(&devfs[devno]);
			return 0;
		}
	}
	return -ENOENT;
}

struct inode *devfs_devi(dev_t dev)
{
	for (unsigned devno = 0; devno < NR_DEVICES; devno++)
		if (devfs[devno].dev == dev)
			return devfs_get_inode(&devfs[devno]);
	return NULL;
}

long sys_open(const char *pathname, int flags, int mode)
{
	int fd, error;
	struct inode *inode;
	struct file *filp;

	for (fd = 0; fd < NR_FILES; fd++)
		if (!current->filp[fd])
			break;
	if (fd == NR_FILES)
		return -EMFILE;

	error = devfs_namei(pathname, &inode);
	if (error)
		return -ENFILE;

	if (!(filp = get_filp())) {
		iput(inode);
		return -ENFILE;
	}

	current->filp[fd] = filp;
	filp->f_flags = flags;
	filp->f_mode = mode;
	filp->f_dev = inode->i_dev;
	filp->f_inode = inode;
	filp->f_op = NULL;
	filp->f_count = 1;
	if (inode->i_op)
		filp->f_op = inode->i_op->default_file_ops;
	if (filp->f_op && filp->f_op->open) {
		error = filp->f_op->open(inode, filp);
		if (error) {
			iput(inode);
			filp->f_count--;
			current->filp[fd] = NULL;
			return error;
		}
	}
	return fd;
}

static int close_fp(struct file *filp, unsigned int fd)
{
	struct inode *inode;

	if (filp->f_count == 0) {
		kprintf("VFS: close: file count is 0\n");
		return 0;
	}
	inode = filp->f_inode;
	if (filp->f_count > 1) {
		filp->f_count--;
		return 0;
	}
	if (filp->f_op && filp->f_op->close)
		filp->f_op->close(inode, filp);
	filp->f_count--;
	filp->f_inode = NULL;
	iput(inode);
	return 0;
}

long sys_close(unsigned int fd)
{
	struct file *filp;

	if (fd >= NR_FILES)
		return -EBADF;
	if (!(filp = current->filp[fd]))
		return -EBADF;
	current->filp[fd] = NULL;
	return close_fp(filp, fd);
}
