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

typedef int clockid_t;
typedef unsigned long timer_t;

typedef unsigned long time_t;

#ifdef __KERNEL__

#define BITS_PER_LONG 32

typedef unsigned short port_t;

typedef unsigned short dev_t;

typedef unsigned long pte_t;
typedef unsigned long* pmap_t;

typedef unsigned char	uchar;
typedef unsigned short	ushort;
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
