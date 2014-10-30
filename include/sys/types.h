/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#include <sys/type_macros.h>

#ifndef NULL
#define NULL _NULL_DEFN
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef _SIZE_T_TYPE size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef _SSIZE_T_TYPE ssize_t;
#endif

#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _CLOCKID_T_DEFINED
#define _CLOCKID_T_DEFINED
typedef _CLOCKID_T_TYPE clockid_t;
#endif
#ifndef _TIMER_T_DEFINED
#define _TIMER_T_DEFINED
typedef _TIMER_T_TYPE timer_t;
#endif
#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
typedef _TIME_T_TYPE time_t;
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
#ifndef _BLKSIZE_T_DEFINED
#define _BLKSIZE_T_DEFINED
typedef _BLKSIZE_T_TYPE blksize_t;
#endif
#ifndef _BLKCNT_T_DEFINED
#define _BLKCNT_T_DEFINED
typedef _BLKCNT_T_TYPE blkcnt_t;
#endif
#ifndef _DEV_T_DEFINED
#define _DEV_T_DEFINED
typedef _DEV_T_TYPE dev_t;
#endif

#ifdef __KERNEL__

#define BITS_PER_LONG 32

typedef unsigned short port_t;

typedef unsigned long pte_t;
typedef unsigned long* pmap_t;

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;

// XXX: arch/implementation dependent
typedef signed   char		s8;
typedef unsigned char		u8;
typedef unsigned long		__s8;
typedef unsigned long		__u8;

typedef signed   short		s16;
typedef unsigned short		u16;
typedef unsigned long		__s16;
typedef unsigned long		__u16;

typedef signed   long		s32;
typedef unsigned long		u32;
typedef unsigned long		__s32;
typedef unsigned long		__u32;

typedef signed   long long	s64;
typedef unsigned long long	u64;
typedef unsigned long long	__s64;
typedef unsigned long long	__u64;

typedef signed   long long	int64_t;
typedef unsigned long long	uint64_t;

typedef long			ptrdiff_t;

#define bool	_Bool
#define true	1
#define false	0

#endif /* __KERNEL__ */
#endif /* _KERNEL_TYPES_H_ */
