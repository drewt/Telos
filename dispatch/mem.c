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

#include <kernel/dispatch.h>
#include <kernel/list.h>
#include <kernel/mem.h>

long sys_sbrk(long inc, ulong *oldbrk)
{
	unsigned long new = current->brk + inc;
	unsigned long pages = (inc / FRAME_SIZE) + 1;

	/* FIXME: check address */
	copy_to_current(oldbrk, &current->brk, sizeof(current->brk));

	if (new < current->heap_start)
		return -EINVAL;

	if (inc <= 0 || new < current->heap_end)
		goto skip;

	if (map_pages_user(current, current->heap_end, pages, PE_U | PE_RW))
		return -ENOMEM;
	current->heap_end += pages * FRAME_SIZE;

skip:
	current->brk = new;
	return 0;
}
