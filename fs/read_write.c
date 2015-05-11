/*
 *  linux/fs/read_write.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/stat.h>
#include <kernel/mm/vma.h>

long sys_readdir(unsigned int fd, struct dirent *dirent, unsigned int count)
{
	int error;
	struct file *file;
	struct inode *inode;

	if (vm_verify(&current->mm, dirent, sizeof(*dirent), VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd]) ||
			!(inode = file->f_inode))
		return -EBADF;

	error = -ENOTDIR;
	if (file->f_op && file->f_op->readdir) {
		error = file->f_op->readdir(inode, file, dirent, count);
	}
	return error;
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

long sys_read(unsigned int fd, char * buf, size_t count)
{
	struct file *file;

	if (vm_verify(&current->mm, buf, count, VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd]) 
			|| !file->f_inode)
		return -EBADF;
	if (!file->f_op || !file->f_op->read)
		return -EINVAL;
	if (!count)
		return 0;
	return file->f_op->read(file, buf, count, &file->f_pos);
}

long sys_pread(unsigned int fd, char *buf, size_t count, unsigned long pos)
{
	struct file *file;
	unsigned long unused = pos;

	if (vm_verify(&current->mm, buf, count, VM_WRITE))
		return -EFAULT;
	if (fd >= NR_FILES || !(file = current->filp[fd])
			|| !file->f_inode)
		return -EBADF;
	if (!count)
		return 0;
	return file->f_op->read(file, buf, count, &unused);
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
	if (!file->f_op || !file->f_op->write)
		return -EINVAL;
	if (!count)
		return 0;
	return file->f_op->write(file, buf, count, &file->f_pos);
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
	if (!file->f_op || !file->f_op->write)
		return -EINVAL;
	if (!count)
		return 0;
	return file->f_op->write(file, buf, count, &pos);
}
