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

#ifndef _SYS_TYPE_MACROS_H_
#define _SYS_TYPE_MACROS_H_

#define _NULL_DEFN ((void*)0)
#define _EOF_DEFN (-1)

#define _SIZE_T_TYPE unsigned int
#define _SSIZE_T_TYPE int
#define _INTPTR_T_TYPE long
#define _UINTPTR_T_TYPE unsigned long
#define _FPOS_T_TYPE unsigned long
#define _PID_T_TYPE int
#define _CLOCK_T_TYPE unsigned long
#define _CLOCKID_T_TYPE int
#define _TIMER_T_TYPE unsigned long
#define _TIME_T_TYPE unsigned long
#define _LOCALE_T_TYPE unsigned long
#define _INO_T_TYPE unsigned long
#define _MODE_T_TYPE unsigned long
#define _NLINK_T_TYPE unsigned long
#define _UID_T_TYPE unsigned long
#define _GID_T_TYPE unsigned long
#define _ID_T_TYPE unsigned long
#define _OFF_T_TYPE long
#define _BLKSIZE_T_TYPE unsigned long
#define _BLKCNT_T_TYPE unsigned long
#define _DEV_T_TYPE unsigned long
#define _SIG_ATOMIC_T_TYPE int
#define _SIGSET_T_TYPE unsigned long

#define _STRUCT_TIMESPEC_DEFN \
	struct timespec { \
		time_t tv_sec; \
		long tv_nsec; \
	};

#define _MAKEDEV_DEFN(maj, min) (((maj) << 8) | ((min) & 0xFF))
#define _MAJOR_DEFN(dev) ((dev) >> 8)
#define _MINOR_DEFN(dev) ((dev) & 0xFF)

#endif
