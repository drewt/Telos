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
extern unsigned long _kernel_high_pgt;
extern unsigned long _kstack;

/* Address Space Layout

   +-------------------+
   |   current pgdir   |
   +-------------------+ <-- current_pgdir (0xFFFFF000)
   |     temp pgtab    |
   +-------------------+ <-- tmp_pgtab (0xFFFFE000)
   |                   |
   |   kernel stack    |
   |                   |
   +-------------------+ <-- KSTACK_START
   |                   |
   |     temporary     |
   |      mapping      |
   |       area        |
   |                   |
   +-------------------+ <-- 0xFFC00000
   |                   |
   :    kernel heap    :
   :  (kalloc_pages)   :
   |                   |
   +-------------------+ <-- kheap_end
   |    kernel heap    |
   |     (kmalloc)     |
   +-------------------+ <-- kheap_start
   |                   |
   |    kernel text    |
   |                   |
   +===================+ <-- 0xC0000000
   |                   |
   |    user stack     |
   |                   |
   +-------------------+ <-- STACK_START
   |                   |
   |     mmap area     |
   |    (grows down)   |
   |                   |
   +- - - - - - - - - -+
   |                   |
   :     user heap     :
   :     (grows up)    :
   |                   |
   +-------------------+
   |                   |
   |     user text     | <-- TODO: loading ELF
   |                   |
   +-------------------+
   |                   |
   :                   :
   :                   :
   +-------------------+

*/

/* linker variables */
extern unsigned long KERNEL_PAGE_OFFSET;
extern unsigned long _kstart;   // start of the kernel
extern unsigned long _kend;     // end of the kernel
extern unsigned long _ustart;   // start of user-space
extern unsigned long _uend;     // end of user-space
extern unsigned long _urostart; // start of user-space read-only memory
extern unsigned long _uroend;   // end of user-space read-only memory
extern unsigned long _urwstart; // start of user-space read-write memory
extern unsigned long _urwend;   // end of user-space read-write memory
extern unsigned long _krostart; // start of kernel read-only memory
extern unsigned long _kroend;   // end of kernel read-only memory
extern unsigned long _krwstart; // start of kernel read-write memory
extern unsigned long _krwend;   // end of kernel read-write memory

#define kernel_base ((unsigned long)&KERNEL_PAGE_OFFSET)
#define kstack      ((unsigned long)&_kstack)
#define kstart      ((unsigned long)&_kstart)
#define kend        ((unsigned long)&_kend)
#define ustart      ((unsigned long)&_ustart)
#define uend        ((unsigned long)&_uend)
#define urostart    ((unsigned long)&_urostart)
#define uroend      ((unsigned long)&_uroend)
#define urwstart    ((unsigned long)&_urwstart)
#define urwend      ((unsigned long)&_urwend)
#define krostart    ((unsigned long)&_krostart)
#define kroend      ((unsigned long)&_kroend)
#define krwstart    ((unsigned long)&_krwstart)
#define krwend      ((unsigned long)&_krwend)

/* export sets */
extern unsigned long _kinit_set;
extern unsigned long _kinit_set_end;
extern unsigned long _slab_set;
extern unsigned long _slab_set_end;

#define __set_length(start, end) \
	(((unsigned long)end - (unsigned long)start) / sizeof(unsigned long))

#define kinit_set ((struct kinit_struct**) &_kinit_set)
#define kinit_set_length __set_length(&_kinit_set, &_kinit_set_end)

extern unsigned long kheap_start;
extern unsigned long kheap_end;

#define HEAP_START   0x00100000UL
#define HEAP_SIZE    FRAME_SIZE

#define STACK_SIZE  0x8000
#define STACK_END   kernel_base
#define STACK_START (STACK_END - STACK_SIZE)

#define NR_KSTACK_PAGES 4
#define NR_HIGH_PAGES  (NR_KSTACK_PAGES + 2)
// XXX: 2 pages = 1 for tmp_pgtab + 1 for current_pgdir

#define KSTACK_SIZE  (NR_KSTACK_PAGES * 4096)
#define KSTACK_START (0xFFFFE000UL - KSTACK_SIZE)
#define KSTACK_END   (KSTACK_START + KSTACK_SIZE)

#endif
