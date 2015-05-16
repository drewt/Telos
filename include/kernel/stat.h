#ifndef _KERNEL_STAT_H_
#define _KERNEL_STAT_H_

#include <sys/type_macros.h>

#ifndef _BLKCNT_T_DEFINED
#define _BLKCNT_T_DEFINED
typedef _BLKCNT_T_TYPE blkcnt_t;
#endif
#ifndef _BLKSIZE_T_DEFINED
#define _BLKSIZE_T_DEFINED
typedef _BLKSIZE_T_TYPE blksize_t;
#endif
#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
typedef _DEV_T_TYPE dev_t;
#endif
#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
typedef _INO_T_TYPE ino_t;
#endif
#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef _MODE_T_TYPE mode_t;
#endif
#ifndef _NLINK_T_DEFINED
#define _NLINK_T_DEFINED
typedef _NLINK_T_TYPE nlink_t;
#endif
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef _UID_T_TYPE uid_t;
#endif
#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED
typedef _GID_T_TYPE gid_t;
#endif
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef _OFF_T_TYPE off_t;
#endif
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef _TIME_T_TYPE time_t;
#endif

#ifndef _STRUCT_TIMESPEC_DEFINED
#define _STRUCT_TIMESPEC_DEFINED
_STRUCT_TIMESPEC_DEFN
#endif

#define S_IFMT   0xF000
#define S_IFFUN  0xE000
#define S_IFSOCK 0xC000
#define S_IFLNK  0xA000
#define S_IFREG  0x8000
#define S_IFBLK  0x6000
#define S_IFDIR  0x4000
#define S_IFCHR  0x2000
#define S_IFIFO  0x1000
#define S_ISUID  0x0800
#define S_ISGID  0x0400
#define S_ISVTX  0x0200

#define S_ISFUN(m)	(((m) & S_IFMT) == S_IFFUN)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

struct stat {
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	dev_t st_rdev;
	off_t st_size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
	unsigned short st_icount;
};

#ifdef __KERNEL__
#define S_IRWXUGO	(S_IRWXU|S_IRWXG|S_IRWXO)
#define S_IALLUGO	(S_ISUID|S_ISGID|S_ISVTX|S_IRWXUGO)
#define S_IRUGO		(S_IRUSR|S_IRGRP|S_IROTH)
#define S_IWUGO		(S_IWUSR|S_IWGRP|S_IWOTH)
#define S_IXUGO		(S_IXUSR|S_IXGRP|S_IXOTH)
#endif

#endif
