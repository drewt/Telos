#ifndef _SYS_MAJOR_H_
#define _SYS_MAJOR_H_

#define MAX_CHRDEV 32
#define MAX_BLKDEV 32

#ifndef makedev
#define makedev _MAKEDEV_DEFN
#endif
#ifndef major
#define major _MAJOR_DEFN
#endif
#ifndef minor
#define minor _MINOR_DEFN
#endif

enum {
	UNNAMED_MAJOR	= 0,
	MEM_MAJOR	= 1,
	FLOPPY_MAJOR	= 2,
	HD_MAJOR	= 3,
	TTY_MAJOR	= 4,
};

#endif
