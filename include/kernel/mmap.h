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

#ifndef _KERNEL_MMAP_H_
#define _KERNEL_MMAP_H_

extern pte_t _kernel_pgd;
extern ulong _kernel_high_pgt;
extern ulong _kstack;

#define kstack (&_kstack)

/* linker variables */
extern ulong KERNEL_PAGE_OFFSET;
extern ulong _kstart;   // start of the kernel
extern ulong _kend;     // end of the kernel
extern ulong _ustart;   // start of user-space
extern ulong _uend;     // end of user-space
extern ulong _urostart; // start of user-space read-only memory
extern ulong _uroend;   // end of user-space read-only memory
extern ulong _urwstart; // start of user-space read-write memory
extern ulong _urwend;   // end of user-space read-write memory
extern ulong _krostart; // start of kernel read-only memory
extern ulong _kroend;   // end of kernel read-only memory
extern ulong _krwstart; // start of kernel read-write memory
extern ulong _krwend;   // end of kernel read-write memory

#define kstart   ((ulong)&_kstart)
#define kend     ((ulong)&_kend)
#define ustart   ((ulong)&_ustart)
#define uend     ((ulong)&_uend)
#define urostart ((ulong)&_urostart)
#define uroend   ((ulong)&_uroend)
#define urwstart ((ulong)&_urwstart)
#define urwend   ((ulong)&_urwend)
#define krostart ((ulong)&_krostart)
#define kroend   ((ulong)&_kroend)
#define krwstart ((ulong)&_krwstart)
#define krwend   ((ulong)&_krwend)

/* export sets */
extern ulong _kinit_set;
extern ulong _kinit_set_end;
extern ulong _slab_set;
extern ulong _slab_set_end;

#define __set_length(start, end) \
	(((ulong)end - (ulong)start) / sizeof(ulong))

#define kinit_set ((struct kinit_struct**) &_kinit_set)
#define kinit_set_length __set_length(&_kinit_set, &_kinit_set_end)

extern ulong kheap_start;
extern ulong kheap_end;

#endif
