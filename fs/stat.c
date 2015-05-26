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

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/mm/vma.h>
#include <sys/stat.h>

long sys_stat(const char *pathname, size_t name_len, struct stat *s)
{
	int error;
	struct inode *inode;

	error = verify_user_string(pathname, name_len);
	if (error)
		return error;
	if (vm_verify(&current->mm, s, sizeof(*s), VM_WRITE))
		return -EFAULT;
	error = namei(pathname, &inode);
	if (error)
		return error;

	s->st_dev = inode->i_dev;
	s->st_ino = inode->i_ino;
	s->st_mode = inode->i_mode;
	s->st_nlink = inode->i_nlink;
	s->st_uid = 0;
	s->st_gid = 0;
	s->st_rdev = inode->i_rdev;
	s->st_size = inode->i_size;
	//s->st_atim = inode->i_atim;
	//s->st_mtim = inode->i_mtim;
	//s->st_ctim = inode->i_ctim;
	s->st_blksize = 1024;
	s->st_blocks = 0;
	s->st_icount = inode->i_count - 1;
	iput(inode);
	return 0;
}
