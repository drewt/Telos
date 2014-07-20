
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#include <sys/types.h>
#include <kernel/stat.h>

int stat(const char *restrict path, struct stat *restrict stat);
int mkdir(const char *path, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);

#endif
