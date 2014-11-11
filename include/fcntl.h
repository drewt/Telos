#ifndef _FNCTL_H_
#define _FNCTL_H_

#include <sys/fcntl.h>

//int creat(const char *, mode_t);
//int fcntl(int, int, ...);
int open(const char *, int, ...);
//int openat(int, const char *, int, ...);
//int posix_fadvise(int, off_t, off_t, int);
//int posix_fallocate(int, off_t, off_t);

#endif
