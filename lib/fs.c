
#include <dirent.h>
#include <syscall.h>
#include <unistd.h>
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
	return syscall2(SYS_MKDIR, path, mode);
}

int mknod(const char *path, mode_t mode, dev_t dev)
{
	return syscall3(SYS_MKNOD, path, mode, dev);
}

int mount(const char *dev_name, const char *dir_name, const char *type,
		ulong new_flags, const void *data)
{
	return syscall5(SYS_MOUNT, dev_name, dir_name, type, new_flags, data);
}

int umount(const char *target)
{
	return syscall1(SYS_UMOUNT, target);
}

int rmdir(const char *path)
{
	return syscall1(SYS_RMDIR, path);
}

int chdir(const char *path)
{
	return syscall1(SYS_CHDIR, path);
}

int unlink(const char *path)
{
	return syscall1(SYS_UNLINK, path);
}

int link(const char *old, const char *new)
{
	return syscall2(SYS_LINK, old, new);
}

int rename(const char *old, const char *new)
{
	return syscall2(SYS_RENAME, old, new);
}

int stat(const char *restrict path, struct stat *restrict stat)
{
	return syscall2(SYS_STAT, path, stat);
}
