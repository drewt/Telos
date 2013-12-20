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

/*
 * dispatch/mem.c
 *
 * System calls for memory allocation.  Currently these allocate memory in a
 * shared heap.  Eventually these should be replaced by a brk syscall and
 * user-space malloc routines.
 */

#include <kernel/common.h>
#include <kernel/dispatch.h>
#include <kernel/list.h>
#include <kernel/mem.h>

#define MAX_ALLOC 0x4000

long sys_malloc(unsigned int size, void **p)
{
	struct mem_header *h;

	if (size == 0)
		return -EINVAL;

	// TODO: limit memory use in a more `global' way
	if (size > MAX_ALLOC)
		return -ENOMEM;

	if (!(*p = hmalloc(size, &h)))
		return -ENOMEM;

	/* add h to list of allocated memory for current process */
	list_add_tail(&h->chain, &current->heap_mem);

	return 0;
}

long sys_free(void *ptr)
{
	struct mem_header *h = mem_ptoh(ptr);

	// XXX: unsafe!  Memory might be unallocated or allocated to another process
	list_del(&h->chain);
	hfree(h);
	return 0;
}

long sys_palloc(void **p)
{
	struct pf_info *page;

	if ((page = kalloc_page()) == NULL)
		return -ENOMEM;

	list_add_tail(&page->chain, &current->page_mem);

	*p = (void*) page->addr;
	return 0;
}
