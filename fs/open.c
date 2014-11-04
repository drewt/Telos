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
#include <kernel/fcntl.h>
#include <kernel/stat.h>
#include <sys/major.h>
#include <string.h>

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

long sys_open(const char *pathname, size_t name_len, int flags, int mode)
{
	int flag, fd, error;
	struct inode *inode;
	struct file *filp;

	error = verify_user_string(pathname, name_len);
	if (error)
		return error;
	for (fd = 0; fd < NR_FILES; fd++)
		if (!current->filp[fd])
			break;
	if (fd == NR_FILES)
		return -EMFILE;

	if (!(filp = get_filp()))
		return -ENFILE;

	current->filp[fd] = filp;
	filp->f_flags = flag = flags;
	filp->f_mode = (flag+1) & O_ACCMODE;
	if (filp->f_mode)
		flag++;
	if (flag & (O_TRUNC | O_CREAT))
		flag |= 2;
	error = open_namei(pathname, flag, mode, &inode, NULL);
	if (error) {
		current->filp[fd] = NULL;
		free_filp(filp);
		return error;
	}

	filp->f_inode = inode;
	filp->f_pos = 0;
	filp->f_op = NULL;
	filp->f_rdev = inode->i_rdev;
	if (inode->i_op)
		filp->f_op = inode->i_op->default_file_ops;
	if (filp->f_op && filp->f_op->open) {
		error = filp->f_op->open(inode, filp);
		if (error) {
			iput(inode);
			current->filp[fd] = NULL;
			free_filp(filp);
			return error;
		}
	}
	return fd;
}

static int close_fp(struct file *filp, unsigned int fd)
{
	struct inode *inode;

	if (filp->f_count == 0) {
		kprintf("VFS: Close: file count is 0\n");
		return 0;
	}
	inode = filp->f_inode;
	if (filp->f_count > 1) {
		filp->f_count--;
		return 0;
	}
	if (filp->f_op && filp->f_op->release)
		filp->f_op->release(inode, filp);
	free_filp(filp);
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
