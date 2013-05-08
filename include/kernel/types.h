#ifndef _KERNEL_TYPES_H_
#define _KERNEL_TYPES_H_

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

#ifdef __KERNEL__

// XXX: arch/implementation dependent
typedef signed   char      s8;
typedef unsigned char      u8;

typedef signed   short     s16;
typedef unsigned short     u16;

typedef signed   long      s32;
typedef unsigned long      u32;

typedef signed   long long s64;
typedef unsigned long long u64;

#define bool  _Bool
#define true  1
#define false 0

#endif /* __KERNEL__ */
#endif /* _KERNEL_TYPES_H_ */