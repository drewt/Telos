
#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <kernel/dirent.h>

typedef struct {
	int fd;
	struct dirent dirent;
} DIR;

DIR *opendir(const char *pathname);
DIR *fdopendir(int fd);
int closedir(DIR *dirp);
int dirfd(DIR *dirp);
void rewinddir(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);
struct dirent *readdir(DIR *dirp);

#endif
