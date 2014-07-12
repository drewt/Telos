#ifndef _KERNEL_MAJOR_H_
#define _KERNEL_MAJOR_H_

#define MAX_CHRDEV 32
#define MAX_BLKDEV 32

#define MAJOR(a) ((a) >> 8)
#define MINOR(a) ((a) & 0xFF)

#define DEVICE(major, minor) (((major) << 8) | (minor))

enum {
	UNNAMED_MAJOR	= 0,
	MEM_MAJOR	= 1,
	FLOPPY_MAJOR	= 2,
	HD_MAJOR	= 3,
	TTY_MAJOR	= 4,
};

#endif
