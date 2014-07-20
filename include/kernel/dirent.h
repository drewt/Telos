

#ifndef _KERNEL_DIRENT_H_
#define _KERNEL_DIRENT_H_

#define NAME_MAX 255

struct dirent {
	unsigned long d_ino;
	unsigned long d_off;
	char d_name[NAME_MAX+1];
};

#endif
