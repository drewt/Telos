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

#ifndef _KERNEL_MEM_H_
#define _KERNEL_MEM_H_

#include <kernel/process.h>
#include <kernel/list.h>

struct multiboot_info;

/*
 * Defines page-at-a-time allocation functions for the given type.
 *
 * @name the name of the allocation function
 * @type the type of object to allocate
 * @list the &struct list_head to add newly allocated objects to
 * @member the name of the struct list_head within the struct
 */
#define DEFINE_ALLOCATOR(name, type, list, member)	\
static int __ ## name(void)				\
{							\
	type *__tmp;					\
	int __i;					\
							\
	if ((__tmp = kmalloc(FRAME_SIZE)) == NULL)	\
		return -1;				\
							\
	__i = FRAME_SIZE / sizeof(type);		\
	for (; __i > 0; __i--, __tmp++)			\
		list_enqueue(&__tmp->member, list);	\
							\
	return 0;					\
}							\
							\
type *name(void)					\
{							\
	if (list_empty(list) && __ ## name() == -1)	\
		return NULL;				\
	return list_dequeue(list, type, member);	\
}

#endif
