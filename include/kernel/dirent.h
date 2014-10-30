

#ifndef _KERNEL_DIRENT_H_
#define _KERNEL_DIRENT_H_

#include <sys/type_macros.h>

#ifndef _INO_T_DEFINED
#define _INO_T_DEFINED
typedef _INO_T_TYPE ino_t;
#endif

#define NAME_MAX 255

struct dirent {
	ino_t d_ino;
	unsigned long d_off;
	char d_name[NAME_MAX+1];
};

#endif
