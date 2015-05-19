/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <string.h>
#include <dirent.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>

#define NR_DIRS 8

static DIR dirs[NR_DIRS];

static inline DIR *get_free_dir(void)
{
	for (int i = 0; i < NR_DIRS; i++)
		if (!dirs[i].fd)
			return &dirs[i];
	return NULL;
}

DIR *opendir(const char *pathname)
{
	int rc;
	DIR *dir;

	if ((dir = get_free_dir()) == NULL)
		return NULL;

	if ((rc = open(pathname, 0)) < 0)
		return NULL;

	dir->fd = rc;
	dir->dirent.d_off = 0;
	return dir;
}

DIR *fdopendir(int fd)
{
	DIR *dir;

	if ((dir = get_free_dir()) == NULL)
		return NULL;

	dir->fd = fd;
	dir->dirent.d_off = 0;
	return dir;
}

int closedir(DIR *dirp)
{
	if (close(dirp->fd) < 0)
		return -1;

	dirp->fd = 0;
	return 0;
}

int dirfd(DIR *dirp)
{
	return dirp->fd;
}

void rewinddir(DIR *dirp)
{
	dirp->dirent.d_off = 0;
}

void seekdir(DIR *dirp, long loc)
{
	dirp->dirent.d_off = loc;
}

long telldir(DIR *dirp)
{
	return dirp->dirent.d_off;
}

struct dirent *readdir(DIR *dirp)
{
	int rc = syscall3(SYS_READDIR, dirp->fd, &dirp->dirent, 1);
	if (rc)
		return NULL;
	return &dirp->dirent;
}

int mkdir(const char *path, mode_t mode)
{
	return syscall3(SYS_MKDIR, path, strlen(path), mode);
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
	return syscall4(SYS_MKNOD, path, strlen(path), mode, dev);
}

int mount(const char *dev_name, const char *dir_name, const char *type,
		unsigned long flags, const void *data)
{
	struct mount mnt = {
		.dev = {
			.str = dev_name,
			.len = dev_name ? strlen(dev_name) : 0,
		},
		.dir = {
			.str = dir_name,
			.len = dir_name ? strlen(dir_name) : 0,
		},
		.type = {
			.str = type,
			.len = type ? strlen(type) : 0,
		},
		.flags = flags,
		.data = data,
	};
	return syscall1(SYS_MOUNT, &mnt);
}

int umount(const char *target)
{
	return syscall2(SYS_UMOUNT, target, strlen(target));
}

int rmdir(const char *path)
{
	return syscall2(SYS_RMDIR, path, strlen(path));
}

int chdir(const char *path)
{
	return syscall2(SYS_CHDIR, path, strlen(path));
}

int unlink(const char *path)
{
	return syscall2(SYS_UNLINK, path, strlen(path));
}

int link(const char *old, const char *new)
{
	return syscall4(SYS_LINK, old, strlen(old), new, strlen(new));
}

int rename(const char *old, const char *new)
{
	return syscall4(SYS_RENAME, old, strlen(old), new, strlen(new));
}

int stat(const char *restrict path, struct stat *restrict stat)
{
	return syscall3(SYS_STAT, path, strlen(path), stat);
}
