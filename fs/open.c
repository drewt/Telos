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
 *  linux/fs/open.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/major.h>
#include <string.h>

long sys_truncate(const char *pathname, size_t name_len, size_t length)
{
	struct inode *inode;
	int error;

	error = verify_user_string(pathname, name_len);
	if (error)
		return error;
	error = namei(pathname, &inode);
	if (error)
		return error;
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -EISDIR;
	}
	if (!permission(inode, MAY_WRITE)) {
		iput(inode);
		return -EACCES;
	}
	if (inode->i_op && inode->i_op->truncate)
		inode->i_op->truncate(inode, length);
	inode->i_size = length;
	return 0;
}

long sys_chdir(const char *pathname, size_t name_len)
{
	struct inode *inode;
	int error;

	error = verify_user_string(pathname, name_len);
	if (error)
		return error;
	error = namei(pathname, &inode);
	if (error)
		return error;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	if (!permission(inode, MAY_EXEC)) {
		iput(inode);
		return -EACCES;
	}
	iput(current->pwd);
	current->pwd = inode;
	return 0;
}

int file_open(struct inode *inode, int flags, int mode, struct file **res)
{
	struct file *file;
	if (!(file = get_empty_file()))
		return -ENFILE;
	file->f_inode = inode;
	file->f_flags = flags;
	file->f_mode = mode;
	file->f_pos = 0;
	file->f_rdev = inode->i_rdev;
	file->f_op = inode->i_op ? inode->i_op->default_file_ops : NULL;
	if (file->f_op && file->f_op->open) {
		int error = file->f_op->open(inode, file);
		if (error) {
			free_file(file);
			return error;
		}
	}
	*res = file;
	return 0;
}

long sys_open(const char *pathname, size_t name_len, int flags, int create_mode)
{
	int fd, error;
	struct inode *inode;
	int file_mode = flags & (O_READ | O_WRITE);
	error = verify_user_string(pathname, name_len);
	if (error)
		return error;
	if ((fd = get_fd(current, 0)) < 0)
		return fd;
	error = open_namei(pathname, flags, create_mode, &inode, NULL);
	if (error)
		return error;
	error = file_open(inode, flags, file_mode, &current->filp[fd]);
	if (error) {
		iput(inode);
		return error;
	}
	return fd;
}

long sys_close(unsigned int fd)
{
	struct file *filp;

	if (fd >= NR_FILES)
		return -EBADF;
	if (!(filp = current->filp[fd]))
		return -EBADF;
	current->filp[fd] = NULL;
	if (filp->f_count == 0)
		kprintf("VFS: close: file count is 0\n");
	else
		file_unref(filp);
	return 0;
}
