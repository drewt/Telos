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

#ifndef _KERNEL_FS_H_
#define _KERNEL_FS_H_

#include <kernel/list.h>
#include <kernel/dirent.h>
#include <kernel/mm/slab.h>
#include <kernel/wait.h>
#include <sys/mount.h>

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
	_SEEK_MAX
};

/*
 * Flags for the permission function.
 */
enum {
	MAY_EXEC  = 1,
	MAY_WRITE = 2,
	MAY_READ  = 4,
};

/*
 * Symbolic constants for submit_buffer, etc.
 */
enum {
	READ  = 1,
	WRITE = 2,
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

enum {
	BUF_UPTODATE = 1,
	BUF_DIRTY    = 2,
};

struct buffer {
	struct list_head b_chain;
	struct hlist_node b_hash;
	struct wait_queue b_wait;
	void *b_data;
	dev_t b_dev;
	blksize_t b_size;
	blkcnt_t b_blocknr;
	unsigned long b_flags;
	unsigned short b_count;
	bool b_lock;
};

/*
 * Block I/O vectors: scatter/gather I/O on block devices.
 */

struct bio_vec {
	dev_t dev;
	blkcnt_t blkcnt;
	blksize_t blksize;
	blkcnt_t block[];
};

struct file_operations {
	off_t (*lseek)(struct inode *, struct file *, off_t, int);
	ssize_t (*read)(struct file *, char *, size_t, unsigned long *pos);
	ssize_t (*write)(struct file *, const char *, size_t, unsigned long *pos);
	int (*readdir)(struct inode *, struct file *, struct dirent *, int);
	int (*open)(struct inode *, struct file *);
	int (*release)(struct inode *, struct file *);
	int (*ioctl)(struct inode *, struct file *, unsigned int, unsigned long);
};

struct file {
	struct list_head chain;
	struct inode *f_inode;
	mode_t f_mode;
	dev_t f_rdev;
	unsigned long f_pos;
	unsigned short f_flags;
	unsigned short f_count;
	void *f_private;
	struct file_operations *f_op;
};

struct inode_operations {
	int(*create)(struct inode *, const char *, int, int, struct inode **);
	int(*lookup)(struct inode *, const char *, int, struct inode **);
	int(*link)(struct inode *, struct inode *, const char *, int);
	int(*unlink)(struct inode *, const char *, int);
	int(*mkdir)(struct inode *, const char *, int, int);
	int(*rmdir)(struct inode *, const char *, int);
	int(*mknod)(struct inode *, const char *, int, int, dev_t);
	int(*rename)(struct inode *, const char *, int, struct inode *,
			const char *, int);
	int(*follow_link)(struct inode *, struct inode *, int, int, struct inode**);
	int(*truncate)(struct inode *, size_t len);
	struct file_operations *default_file_ops;
};

struct inode {
	struct list_head	i_list;
	struct hlist_node	i_hash;
	unsigned long		i_ino;
	mode_t			i_mode;
	nlink_t			i_nlink;
	dev_t			i_dev;
	dev_t			i_rdev;
	unsigned long		i_size;
	unsigned short		i_count;
	unsigned short		i_flags;
	bool			i_dirt;
	struct bio_vec          *i_bio;
	struct inode		*i_mount;
	struct inode_operations	*i_op;
	struct super_block	*i_sb;
	void			*i_private;
};

struct super_operations {
	void(*read_inode)(struct inode *);
	void(*write_inode)(struct inode *);
	void(*put_inode)(struct inode *);
	void(*put_super)(struct super_block *);
	void(*write_super)(struct super_block *);
	int(*remount_fs)(struct super_block *, int *, const void *);
};

struct super_block {
	dev_t s_dev;
	unsigned long s_flags;
	unsigned char s_dirt;
	struct inode *s_mounted;
	struct inode *s_covered;
	struct super_operations *s_op;
	void *s_private;
};

struct file_system_type {
	struct super_block *(*read_super)(struct super_block *, void *, int);
	char *name;
	int requires_dev;
};

extern struct inode_operations chrdev_inode_operations;
extern struct inode_operations blkdev_inode_operations;

extern struct inode *root_inode;

void iput(struct inode *inode);

struct file *get_empty_file(void);
void free_file(struct file *filp);
void file_unref(struct file *file);
struct inode *get_empty_inode(void);

int file_open(struct inode *inode, int flags, int mode, struct file **res);
void mount_root(void);

struct inode *iget(struct super_block *sb, unsigned long ino);
struct inode *__iget(struct super_block *sb, ino_t ino, bool crossmntp);
void insert_inode_hash(struct inode *inode);

static void ifree(struct inode *inode)
{
	inode->i_flags &= ~MS_MEMFS;
	iput(inode);
}

int permission(struct inode *inode, int mask);
bool fs_may_mount(dev_t dev);
bool fs_may_umount(dev_t dev, struct inode * mount_root);
bool fs_may_remount_ro(dev_t dev);

int register_filesystem(
	struct super_block *(*read_super)(struct super_block*, void*, int),
	char *name, int requires_dev);
struct file_system_type *get_fs_type(const char *name);

int fsync_dev(dev_t dev);

extern int namei(const char * pathname, struct inode ** res_inode);
extern int lnamei(const char * pathname, struct inode ** res_inode);
extern int open_namei(const char *pathname, int flag, int mode,
		struct inode **res_inode, struct inode *base);

/* devices.c */
struct file_operations *get_blkfops(unsigned int major);
struct file_operations *get_chrfops(unsigned int major);
int register_chrdev(unsigned int major, const char * name, struct file_operations *fops);
int register_blkdev(unsigned int major, const char * name, struct file_operations *fops);
int unregister_chrdev(unsigned int major, const char * name);
int unregister_blkdev(unsigned int major, const char * name);

struct buffer *read_block(dev_t dev, blkcnt_t block, blksize_t size);
int free_device_buffers(dev_t dev);
void clear_buffer_cache(void);
int submit_block(int rw, struct buffer *buf);
void buffer_wait(struct buffer *buf);
ssize_t blkdev_read(dev_t dev, void *dst, size_t len, unsigned long pos);
blksize_t blkdev_blksize(dev_t devno);
int set_blocksize(dev_t devno, blksize_t size);

struct bio_vec *alloc_bio_vec(dev_t dev, blkcnt_t blkcnt, blksize_t blksize);
void free_bio_vec(struct bio_vec *bio);
ssize_t bio_read(struct bio_vec *vec, char *iobuf, size_t len,
		unsigned long *pos);
ssize_t bio_write(struct bio_vec *vec, const char *iobuf, size_t len,
		unsigned long *pos);
ssize_t bio_file_read(struct file *file, char *buf, size_t len,
		unsigned long *pos);

static inline void release_buffer(struct buffer *buf)
{
	if (buf)
		buf->b_count--;
}

// FIXME: doesn't belong here...
int do_mmap(struct file *file, void **addr, size_t len, int prot, int flags,
		unsigned long off);

#endif
