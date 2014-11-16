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
#include <kernel/mm/paging.h>
#include <kernel/mm/vma.h>

long sys_sbrk(long inc, ulong *oldbrk)
{
	int rc;
	struct vma *heap;
	unsigned long new = current->mm.brk + inc;

	if (vm_verify(&current->mm, oldbrk, sizeof(*oldbrk), VM_WRITE))
		return -EFAULT;
	*oldbrk = current->mm.brk;

	if (!(heap = vma_get(&current->mm, (void*)(current->mm.brk-1))))
		panic("Process %d has no heap!\n", current->pid);

	if (new < heap->start)
		return -EINVAL;

	if (inc <= 0 || new < heap->end)
		goto skip;

	if ((rc = vma_grow_up(heap, inc, heap->flags)))
		return rc;

skip:
	current->mm.brk = new;
	return 0;
}
