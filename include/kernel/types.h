#ifndef _TELOS_TYPES_H_
#define _TELOS_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef int ssize_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef int pid_t;

#endif
