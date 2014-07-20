
#include <kernel/dispatch.h>
#include <kernel/fcntl.h>
#include <kernel/fs.h>
#include <kernel/stat.h>

int permission(struct inode *inode, int mask)
{
	return 1;
}

int lookup(struct inode *dir, const char *name, int len,
		struct inode **result)
{
	struct super_block *sb;
	int perm;

	*result = NULL;
	if (!dir)
		return -ENOENT;

	perm = permission(dir, MAY_EXEC);
	if (len == 2 && name[0] == '.' && name[1] == '.') {
		if (dir == current->root) {
			*result = dir;
			return 0;
		} else if ((sb = dir->i_sb) && (dir == sb->s_mounted)) {
			sb = dir->i_sb;
			iput(dir);
			dir = sb->s_covered;
			if (!dir)
				return -ENOENT;
			dir->i_count++;
		}
	}
	if (!dir->i_op || !dir->i_op->lookup) {
		iput(dir);
		return -ENOTDIR;
	}
	if (!perm) {
		iput(dir);
		return -EACCES;
	}
	if (!len) {
		*result = dir;
		return 0;
	}
	return dir->i_op->lookup(dir, name, len, result);
}

int follow_link(struct inode *dir, struct inode *inode, int flag, int mode,
		struct inode **res_inode)
{
	if (!dir || !inode) {
		iput(dir);
		iput(inode);
		*res_inode = NULL;
		return -ENOENT;
	}
	if (!inode->i_op || !inode->i_op->follow_link) {
		iput(dir);
		*res_inode = inode;
		return 0;
	}
	return inode->i_op->follow_link(dir, inode, flag, mode, res_inode);
}

static int dir_namei(const char *pathname, int *namelen, const char **name,
		struct inode *base, struct inode **res_inode)
{
	char c;
	const char *thisname;
	int len, error;
	struct inode *inode;

	*res_inode = NULL;
	if (!base) {
		base = current->pwd;
		base->i_count++;
	}
	if ((c = *pathname) == '/') {
		iput(base);
		base = current->root;
		pathname++;
		base->i_count++;
	}
	while (1) {
		thisname = pathname;
		for (len = 0; (c = *(pathname++)) && (c != '/'); len++)
			/* nothing */;
		if (!c)
			break;
		base->i_count++;
		error = lookup(base, thisname, len, &inode);
		if (error) {
			iput(base);
			return error;
		}
		error = follow_link(base, inode, 0, 0, &base);
		if (error)
			return error;
	}
	if (!base->i_op || !base->i_op->lookup) {
		iput(base);
		return -ENOTDIR;
	}
	*name = thisname;
	*namelen = len;
	*res_inode = base;
	return 0;
}

static int _namei(const char *pathname, struct inode *base,
		int follow_links, struct inode **res_inode)
{
	const char *basename;
	int namelen, error;
	struct inode *inode;

	*res_inode = NULL;
	error = dir_namei(pathname, &namelen, &basename, base, &base);
	if (error)
		return error;

	base->i_count++; /* lookup uses up base */
	error = lookup(base, basename, namelen, &inode);
	if (error) {
		iput(base);
		return error;
	}
	if (follow_links) {
		error = follow_link(base, inode, 0, 0, &inode);
		if (error)
			return error;
	} else {
		iput(base);
	}

	*res_inode = inode;
	return 0;
}

int lnamei(const char *pathname, struct inode **res_inode)
{
	return _namei(pathname, NULL, 0, res_inode);
}

int namei(const char *pathname, struct inode **res_inode)
{
	return _namei(pathname, NULL, 1, res_inode);
}

int open_namei(const char *pathname, int flag, int mode,
		struct inode **res_inode, struct inode *base)
{
	const char *basename;
	int namelen, error;
	struct inode *dir, *inode;

	error = dir_namei(pathname, &namelen, &basename, base, &dir);
	if (error)
		return error;
	if (!namelen) {
		*res_inode = dir;
		return 0;
	}
	dir->i_count++;
	if (flag & O_CREAT) {
		error = lookup(dir, basename, namelen, &inode);
		if (!error) {
			if (flag & O_EXCL) {
				iput(inode);
				error = -EEXIST;
			}
		} else if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
			error = -EACCES;
		} else if (!dir->i_op || !dir->i_op->create) {
			error = -EACCES;
		} else if (IS_RDONLY(dir)) {
			error = -EROFS;
		} else {
			dir->i_count++;
			error = dir->i_op->create(dir, basename, namelen,
					mode, res_inode);
			iput(dir);
			return error;
		}
	} else {
		error = lookup(dir, basename, namelen, &inode);
	}
	if (error) {
		iput(dir);
		return error;
	}
	// TODO: lots of checks...
	*res_inode = inode;
	return 0;
}

long sys_mknod(const char *filename, int mode, dev_t dev)
{
	const char *basename;
	int namelen, error;
	struct inode *dir;

	if (S_ISDIR(mode))
		return -EPERM;
	switch (mode & S_IFMT) {
	case 0:
		mode |= S_IFREG;
		break;
	case S_IFREG: case S_IFCHR: case S_IFBLK:
		break;
	default:
		return -EINVAL;
	}

	error = dir_namei(filename, &namelen, &basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
		iput(dir);
		return -EACCES;
	}
	if (!dir->i_op || !dir->i_op->mknod) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->mknod(dir, basename, namelen, mode, dev);
}

long sys_mkdir(const char *pathname, int mode)
{
	const char *basename;
	int namelen, error;
	struct inode *dir;

	error = dir_namei(pathname, &namelen, &basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
		iput(dir);
		return -EACCES;
	}
	if (!dir->i_op || !dir->i_op->mkdir) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->mkdir(dir, basename, namelen, mode);
}

long sys_rmdir(const char *pathname)
{
	const char *basename;
	int namelen, error;
	struct inode *dir;

	error = dir_namei(pathname, &namelen, &basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -ENOENT;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
		iput(dir);
		return -EACCES;
	}
	if (!dir->i_op || !dir->i_op->rmdir) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->rmdir(dir, basename, namelen);
}

long sys_unlink(const char *pathname)
{
	const char *basename;
	int namelen, error;
	struct inode *dir;

	error = dir_namei(pathname, &namelen, &basename, NULL, &dir);
	if (error)
		return error;
	if (!namelen) {
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(dir);
		return -EROFS;
	}
	if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
		iput(dir);
		return -EACCES;
	}
	if (!dir->i_op || !dir->i_op->unlink) {
		iput(dir);
		return -EPERM;
	}
	return dir->i_op->unlink(dir, basename, namelen);
}

long sys_link(const char *oldname, const char *newname)
{
	const char *basename;
	int namelen, error;
	struct inode *dir, *oldinode;

	error = namei(oldname, &oldinode);
	if (error)
		return error;

	error = dir_namei(newname, &namelen, &basename, NULL, &dir);
	if (error) {
		iput(oldinode);
		return error;
	}
	if (!namelen) {
		iput(oldinode);
		iput(dir);
		return -EPERM;
	}
	if (IS_RDONLY(dir)) {
		iput(oldinode);
		iput(dir);
		return -EROFS;
	}
	if (dir->i_dev != oldinode->i_dev) {
		iput(dir);
		iput(oldinode);
		return -EXDEV;
	}
	if (!permission(dir, MAY_WRITE | MAY_EXEC)) {
		iput(dir);
		iput(oldinode);
		return -EACCES;
	}
	if (!dir->i_op || !dir->i_op->link) {
		iput(dir);
		iput(oldinode);
		return -EPERM;
	}
	return dir->i_op->link(oldinode, dir, basename, namelen);
}

static inline int is_dotlink(const char *name, int len)
{
	return name[0] == '.' && (len == 1 || (name[1] == '.' && len == 2));
}

long sys_rename(const char *oldname, const char *newname)
{
	const char *old_base, *new_base;
	int old_len, new_len, error;
	struct inode *old_dir, *new_dir;

	error = dir_namei(oldname, &old_len, &old_base, NULL, &old_dir);
	if (error)
		return error;
	if (!permission(old_dir, MAY_WRITE | MAY_EXEC)) {
		iput(old_dir);
		return -EACCES;
	}
	if (!old_len || is_dotlink(old_base, old_len)) {
		iput(old_dir);
		return -EPERM;
	}
	error = dir_namei(newname, &new_len, &new_base, NULL, &new_dir);
	if (error) {
		iput(old_dir);
		return error;
	}
	if (!permission(new_dir, MAY_WRITE | MAY_EXEC)) {
		iput(old_dir);
		iput(new_dir);
		return -EACCES;
	}
	if (!new_len || is_dotlink(new_base, new_len)) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	if (new_dir->i_dev != old_dir->i_dev) {
		iput(old_dir);
		iput(new_dir);
		return -EXDEV;
	}
	if (IS_RDONLY(new_dir) || IS_RDONLY(old_dir)) {
		iput(old_dir);
		iput(new_dir);
		return -EROFS;
	}
	if (!old_dir->i_op || !old_dir->i_op->rename) {
		iput(old_dir);
		iput(new_dir);
		return -EPERM;
	}
	return old_dir->i_op->rename(old_dir, old_base, old_len, new_dir,
			new_base, new_len);
}
