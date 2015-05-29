/*  Copyright 2013-2015 Drew Thoreson
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

#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#define __need_size_t
#include <stddef.h>
#include <sys/type_defs.h>

#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef _MODE_T_TYPE mode_t;
#endif
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef _OFF_T_TYPE off_t;
#endif

#define PROT_NONE  0
#define PROT_EXEC  1
#define PROT_WRITE 2
#define PROT_READ  4

#define MAP_PRIVATE   0
#define MAP_SHARED    1
#define MAP_FIXED     2
#define MAP_ANONYMOUS 4

struct __mmap_args {
	void *addr;
	size_t len;
	int prot;
	int flags;
	int fd;
	off_t off;
};

#define MAP_FAILED ((void*)-1)

#ifndef __KERNEL__
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);
#endif
#endif
