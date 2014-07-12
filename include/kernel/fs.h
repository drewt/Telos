#ifndef _KERNEL_FS_H_
#define _KERNEL_FS_H_

#include <kernel/list.h>
#include <kernel/mm/slab.h>

/*
 * These are the fs-independent mount-flags: up to 16 flags are supported
 */
enum {
	MS_RDONLY	=  1, /* mount read-only */
	MS_NOSUID	=  2, /* ignore suid and sgid bits */
	MS_NODEV	=  4, /* disallow access to device special files */
	MS_NOEXEC	=  8, /* disallow program execution */
	MS_SYNC		= 16, /* writes are synced at once */
	MS_REMOUNT	= 32, /* alter flags of a mounted FS */
};

/*
 * Flags that can be altered by MS_REMOUNT
 */
#define MS_RMT_MASK (MS_RDONLY)

/*
 * Magic mount flag number. Has to be or-ed to the flag values.
 */
#define MS_MGC_VAL 0xC0ED0000 /* magic flag number to indicate "new" flags */
#define MS_MGC_MSK 0xffff0000 /* magic flag number mask */

/*
 * Note that read-only etc flags are inode-specific: setting some file-system
 * flags just means all the inodes inherit those flags by default. It might be
 * possible to overrride it sevelctively if you really wanted to with some
 * ioctl() that is not currently implemented.
 *
 * Exception: MS_RDONLY is always applied to the entire file system.
 */
#define IS_RDONLY(inode) (((inode)->i_sb) && ((inode)->i_sb->s_flags & MS_RDONLY))
#define IS_NOSUID(inode) ((inode)->i_flags & MS_NOSUID)
#define IS_NODEV(inode) ((inode)->i_flags & MS_NODEV)
#define IS_NOEXEC(inode) ((inode)->i_flags & MS_NOEXEC)
#define IS_SYNC(inode) ((inode)->i_flags & MS_SYNC)

struct inode;
struct file;
struct super_block;

struct file_operations {
	ssize_t (*read)(struct file *, char *, size_t);
	ssize_t (*write)(struct file *, const char *, size_t);
	int (*open)(struct inode *, struct file *);
	int (*close)(struct inode *, struct file *);
	int (*ioctl)(struct inode *, struct file *, unsigned int, unsigned long);
};

struct file {
	struct list_head chain;
	struct inode *f_inode;
	umode_t f_mode;
	dev_t f_dev;
	unsigned short f_flags;
	unsigned short f_count;
	struct file_operations *f_op;
};

struct inode_operations {
	int(*create)(struct inode *, const char *, int, int, struct inode **);
	int(*lookup)(struct inode *, const char *, int, struct inode **);
	int(*mkdir)(struct inode *, const char *, int);
	int(*rmdir)(struct inode *, const char *, int);
	int(*rename)(struct inode *, const char *, int, struct inode *,
			const char *, int);
	struct file_operations *default_file_ops;
};

struct inode {
	unsigned long		i_ino;
	umode_t			i_mode;
	dev_t			i_dev;
	unsigned short		i_count;
	unsigned short		i_flags;
	struct inode		*i_mount;
	struct inode_operations	*i_op;
	struct super_block	*i_sb;
};

struct super_operations {
	void(*read_inode)(struct inode *);
	void(*write_inode)(struct inode *);
	void(*put_inode)(struct inode *);
	void(*put_super)(struct super_block *);
	void(*write_super)(struct super_block *);
	int(*remount_fs)(struct super_block *, int *, char *);
};

struct super_block {
	dev_t s_dev;
	unsigned long s_flags;
	unsigned char s_dirt;
	struct inode *s_mounted;
	struct inode *s_covered;
	struct super_operations *s_op;
};

struct file_system_type {
	struct super_block *(*read_super)(struct super_block *, void *, int);
	char *name;
	int requires_dev;
};

struct slab_cache *file_cachep;
struct slab_cache *inode_cachep;

static inline struct file *get_filp(void)
{
	return slab_alloc(file_cachep);
}

static inline struct inode *iget(void)
{
	return slab_alloc(inode_cachep);
}

static inline void free_filp(struct file *filp)
{
	slab_free(file_cachep, filp);
}

static inline void iput(struct inode *inode)
{
	if (!inode)
		return;
	if (--inode->i_count == 0)
		slab_free(inode_cachep, inode);
}

int fs_may_mount(dev_t dev);
int fs_may_umount(dev_t dev, struct inode * mount_root);
extern int fs_may_remount_ro(dev_t dev);

int fsync_dev(dev_t dev);

extern int namei(const char * pathname, struct inode ** res_inode);
extern int lnamei(const char * pathname, struct inode ** res_inode);

/* devices.c */
struct file_operations * get_blkfops(unsigned int major);
struct file_operations * get_chrfops(unsigned int major);
int register_chrdev(unsigned int major, const char * name, struct file_operations *fops);
int register_blkdev(unsigned int major, const char * name, struct file_operations *fops);
int unregister_chrdev(unsigned int major, const char * name);
int unregister_blkdev(unsigned int major, const char * name);

int devfs_namei(const char *path, struct inode **inode);
struct inode *devfs_devi(dev_t dev);

#endif
