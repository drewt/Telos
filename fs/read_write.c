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
 *  linux/fs/read_write.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/mm/vma.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

long sys_readdir(unsigned int fd, struct dirent *dirent, unsigned int count)
{
	struct file *file;
	struct inode *inode;

	if (vm_verify(&current->mm, dirent, sizeof(*dirent), VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd]) ||
			!(inode = file->f_inode))
		return -EBADF;

	if (file->f_op && file->f_op->readdir)
		return file->f_op->readdir(inode, file, dirent, count);
	return -ENOTDIR;
}

long sys_lseek(unsigned int fd, off_t offset, unsigned int whence)
{
	struct file *filp;
	int tmp = -1;

	if (fd >= NR_FILES || !(filp=current->filp[fd]) || !(filp->f_inode))
		return -EBADF;
	if (whence >= _SEEK_MAX)
		return -EINVAL;
	if (filp->f_op && filp->f_op->lseek)
		return filp->f_op->lseek(filp->f_inode, filp, offset, whence);

	switch (whence) {
	case SEEK_SET:
		tmp = offset;
		break;
	case SEEK_CUR:
		tmp = filp->f_pos + offset;
		break;
	case SEEK_END:
		if (!filp->f_inode)
			return -EINVAL;
		tmp = filp->f_inode->i_size + offset;
		break;
	}
	if (tmp < 0)
		return -EINVAL;
	filp->f_pos = tmp;
	return filp->f_pos;
}

static long do_read(struct file *file, char *buf, size_t count,
		unsigned long *pos)
{
	if (!(file->f_mode & O_READ))
		return -EBADF;
	if (!count)
		return 0;
	if (file->f_op && file->f_op->read)
		return file->f_op->read(file, buf, count, pos);
	return -EINVAL;
}

long sys_read(unsigned int fd, char *buf, size_t count)
{
	struct file *file;

	if (vm_verify(&current->mm, buf, count, VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd]) 
			|| !file->f_inode)
		return -EBADF;
	return do_read(file, buf, count, &file->f_pos);
}

long sys_pread(unsigned int fd, char *buf, size_t count, unsigned long pos)
{
	struct file *file;

	if (vm_verify(&current->mm, buf, count, VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd])
			|| !file->f_inode)
		return -EBADF;
	return do_read(file, buf, count, &pos);
}

static long do_write(struct file *file, char *buf, size_t count,
		unsigned long *pos)
{
	if (!(file->f_mode & O_WRITE))
		return -EBADF;
	if (file->f_inode && IS_RDONLY(file->f_inode))
		return -ENOSPC;
	if (!count)
		return 0;
	if (file->f_op && file->f_op->write)
		return file->f_op->write(file, buf, count, pos);
	return -EINVAL;
}

long sys_write(unsigned int fd, char * buf, size_t count)
{
	struct file *file;
	struct inode *inode;

	if (vm_verify(&current->mm, buf, count, VM_READ))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd])
			|| !(inode = file->f_inode))
		return -EBADF;
	return do_write(file, buf, count, &file->f_pos);
}

long sys_pwrite(unsigned int fd, char *buf, size_t count, unsigned long pos)
{
	struct file *file;
	struct inode *inode;

	if (vm_verify(&current->mm, buf, count, VM_READ))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd])
			|| !(inode = file->f_inode))
		return -EBADF;
	return do_write(file, buf, count, &pos);
}
