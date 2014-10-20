/*
 *  linux/fs/devices.c
 *
 * (C) 1993 Matthias Urlichs -- collected common code and tables.
 * 
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <kernel/fs.h>
#include <kernel/major.h>
#include <string.h>

struct device_struct {
	const char * name;
	struct file_operations * fops;
};

static struct device_struct chrdevs[MAX_CHRDEV] = {
	{ NULL, NULL },
};

static struct device_struct blkdevs[MAX_BLKDEV] = {
	{ NULL, NULL },
};

struct file_operations * get_blkfops(unsigned int major)
{
	if (major >= MAX_BLKDEV)
		return NULL;
	return blkdevs[major].fops;
}

struct file_operations * get_chrfops(unsigned int major)
{
	if (major >= MAX_CHRDEV)
		return NULL;
	return chrdevs[major].fops;
}

int register_chrdev(unsigned int major, const char * name, struct file_operations *fops)
{
	if (major >= MAX_CHRDEV)
		return -EINVAL;
	if (chrdevs[major].fops)
		return -EBUSY;
	chrdevs[major].name = name;
	chrdevs[major].fops = fops;
	return 0;
}

int register_blkdev(unsigned int major, const char * name, struct file_operations *fops)
{
	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (blkdevs[major].fops)
		return -EBUSY;
	blkdevs[major].name = name;
	blkdevs[major].fops = fops;
	return 0;
}

int unregister_chrdev(unsigned int major, const char * name)
{
	if (major >= MAX_CHRDEV)
		return -EINVAL;
	if (!chrdevs[major].fops)
		return -EINVAL;
	if (strcmp(chrdevs[major].name, name))
		return -EINVAL;
	chrdevs[major].name = NULL;
	chrdevs[major].fops = NULL;
	return 0;
}

int unregister_blkdev(unsigned int major, const char * name)
{
	if (major >= MAX_BLKDEV)
		return -EINVAL;
	if (!blkdevs[major].fops)
		return -EINVAL;
	if (strcmp(blkdevs[major].name, name))
		return -EINVAL;
	blkdevs[major].name = NULL;
	blkdevs[major].fops = NULL;
	return 0;
}

/*
 * Called every time a block special file is opened
 */
int blkdev_open(struct inode * inode, struct file * filp)
{
	int i;

	i = MAJOR(inode->i_rdev);
	if (i >= MAX_BLKDEV || !blkdevs[i].fops)
		return -ENODEV;
	filp->f_op = blkdevs[i].fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}	

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
struct file_operations def_blk_fops = {
	.open = blkdev_open,
};

struct inode_operations blkdev_inode_operations = {
	.default_file_ops = &def_blk_fops,
};

/*
 * Called every time a character special file is opened
 */
int chrdev_open(struct inode * inode, struct file * filp)
{
	int i;

	i = MAJOR(inode->i_rdev);
	if (i >= MAX_CHRDEV || !chrdevs[i].fops)
		return -ENODEV;
	filp->f_op = chrdevs[i].fops;
	if (filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}

/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
struct file_operations def_chr_fops = {
	.open = chrdev_open,
};

struct inode_operations chrdev_inode_operations = {
	.default_file_ops = &def_chr_fops,
};

/*
 * "devfs" hack.  This is not a real filesystem, just a means of obtaining
 * inodes for devices.
 */
struct devfs_inode {
	dev_t dev;
	struct inode_operations *iops;
	struct inode *inode;
};

extern struct inode_operations console_iops;
static struct devfs_inode devfs[] = {
	{
		.dev  = DEVICE(TTY_MAJOR, 0),
		.iops = &console_iops,
	},
	{
		.dev  = DEVICE(TTY_MAJOR, 1),
		.iops = &console_iops,
	},
};
#define NR_DEVICES (sizeof(devfs) / sizeof(*devfs))

static struct inode *devfs_get_inode(struct devfs_inode *fsnode)
{
	struct inode *inode;

	if (fsnode->inode)
		return fsnode->inode;

	inode = get_empty_inode();
	inode->i_op = fsnode->iops;
	inode->i_rdev = fsnode->dev;
	return inode;
}

struct inode *devfs_geti(dev_t dev)
{
	for (unsigned devno = 0; devno < NR_DEVICES; devno++)
		if (devfs[devno].dev == dev)
			return devfs_get_inode(&devfs[devno]);
	return NULL;
}