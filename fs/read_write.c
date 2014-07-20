/*
 *  linux/fs/read_write.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/stat.h>

long sys_readdir(unsigned int fd, struct dirent *dirent, unsigned int count)
{
	int error;
	struct file *file;
	struct inode *inode;

	if (fd >= NR_FILES || !(file = current->filp[fd]) ||
			!(inode = file->f_inode))
		return -EBADF;

	error = -ENOTDIR;
	if (file->f_op && file->f_op->readdir) {
		// TODO: check pointer
		error = file->f_op->readdir(inode, file, dirent, count);
	}
	return error;
}

long sys_read(unsigned int fd, char * buf, size_t count)
{
	struct file *file;
	struct inode *inode;

	if (fd >= NR_FILES || !(file = current->filp[fd]) 
			|| !(inode = file->f_inode))
		return -EBADF;
	if (!file->f_op || !file->f_op->read)
		return -EINVAL;
	if (!count)
		return 0;
	// TODO: check pointer
	return file->f_op->read(file, buf, count);
}

long sys_write(unsigned int fd, char * buf, size_t count)
{
	struct file *file;
	struct inode *inode;

	if (fd >= NR_FILES || !(file = current->filp[fd])
			|| !(inode = file->f_inode))
		return -EBADF;
	if (!file->f_op || !file->f_op->write)
		return -EINVAL;
	if (!count)
		return 0;
	// TODO: check pointer
	return file->f_op->write(file, buf, count);
}
