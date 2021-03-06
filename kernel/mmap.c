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

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/i386.h>
#include <kernel/mmap.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <kernel/mm/vma.h>
#include <telos/mman.h>
#include <string.h>

struct mmap_private {
	struct file *file;
	unsigned long off;
	unsigned long len;
	unsigned int ref;
};

static DEFINE_SLAB_CACHE(mmap_private_cachep, sizeof(struct mmap_private));

static inline struct mmap_private *alloc_private(struct file *file,
		unsigned long off, unsigned long len)
{
	struct mmap_private *private = slab_alloc(mmap_private_cachep);
	if (!private)
		return NULL;
	private->file = file;
	private->off = off;
	private->len = len;
	private->ref = 1;
	file->f_count++;
	return private;
}

static inline void private_unref(struct mmap_private *private)
{
	file_unref(private->file);
	if (--private->ref == 0)
		slab_free(mmap_private_cachep, private);
}

static inline void private_ref(struct mmap_private *private)
{
	private->file->f_count++;
	private->ref++;
}

static int mmap_map(struct vma *vma, void *addr)
{
	int error;
	ssize_t bytes;
	struct pf_info *frame;
	char *vaddr;
	struct mmap_private *private = vma->private;
	uintptr_t base = page_base((uintptr_t)addr);
	unsigned long pos = private->off + (base - vma->start);

	// FIXME: should wait until memory is available...
	if (!(frame = kalloc_frame(vma->flags)))
		return -ENOMEM;
	vaddr = kmap_tmp_page(frame->addr);
	bytes = private->file->f_op->read(private->file, vaddr, FRAME_SIZE, &pos);
	if (bytes < 0) {
		error = bytes;
		goto abort;
	}
	if (bytes < FRAME_SIZE)
		memset(vaddr + bytes, 0, FRAME_SIZE - bytes);
	if ((error = map_frame(frame, addr, vma->flags)))
		goto abort;
	kunmap_tmp_page(vaddr);
	return 0;
abort:
	kunmap_tmp_page(vaddr);
	kfree_frame(frame);
	return error;
}

static int mmap_writeback(struct vma *vma, void *addr, size_t len)
{
	ssize_t bytes;
	struct mmap_private *private = vma->private;
	uintptr_t base = page_base((uintptr_t)addr);
	unsigned long pos = private->off + (vma->start - base);

	len = MIN(len, (vma->start + private->len) - base);
	bytes = private->file->f_op->write(private->file, addr, len, &pos);
	if (bytes < 0)
		return bytes;
	return 0;
}

static int mmap_unmap(struct vma *vma)
{
	private_unref(vma->private);
	return 0;
}

static int mmap_clone(struct vma *dst, struct vma *src)
{
	private_ref((dst->private = src->private));
	return 0;
}

static int mmap_split(struct vma *new, struct vma *old)
{
	struct mmap_private *private;
	struct mmap_private *old_private = old->private;
	private = alloc_private(old_private->file, old_private->off,
			old_private->len);
	if (!private)
		return -ENOMEM;
	private->off += new->start - old->start;
	private->len -= old->end - old->start;
	old_private->len -= private->len;
	new->private = private;
	return 0;
}

struct vma_operations mmap_vma_ops = {
	.map = mmap_map,
	.unmap = mmap_unmap,
	.clone = mmap_clone,
	.split = mmap_split,
};

struct vma_operations mmap_shared_vma_ops = {
	.map = mmap_map,
	.writeback = mmap_writeback,
	.unmap = mmap_unmap,
	.clone = mmap_clone,
	.split = mmap_split,
};

static struct vma *mmap_create_vma(void *addr, size_t len, int prot, int flags)
{
	if (flags & MAP_FIXED)
		return vma_create_fixed(&current->mm, (uintptr_t)addr, len, prot);
	return vma_create_high(&current->mm, user_base, kernel_base, len, prot);
}

int do_mmap(struct file *file, void **addr, size_t len, int prot, int flags,
		unsigned long off)
{
	struct vma *vma;
	struct mmap_private *private;
	private = alloc_private(file, off, len);
	if (!private)
		return -ENOMEM;
	vma = mmap_create_vma(*addr, len, prot, flags);
	if (!vma) {
		slab_free(mmap_private_cachep, private);
		return -ENOMEM;
	}
	vma->op = (flags & MAP_SHARED) ? &mmap_shared_vma_ops : &mmap_vma_ops;
	vma->private = private;
	*addr = (void*) vma->start;
	return 0;
}

static int do_mmap_anon(void **addr, size_t len, int prot, int flags)
{
	struct vma *vma = mmap_create_vma(*addr, len, prot | VM_ZERO, flags);
	if (!vma)
		return -ENOMEM;
	*addr = (void*) vma->start;
	return 0;
}

long sys_mmap(struct __mmap_args *args)
{
	struct file *file;
	if (vm_verify(&current->mm, args, sizeof(*args), VM_READ | VM_WRITE))
		return -EFAULT;
	if (args->flags & MAP_FIXED && !page_aligned(args->addr))
		return -EINVAL;
	if (args->flags & MAP_ANONYMOUS)
		return do_mmap_anon(&args->addr, args->len, args->prot & 7,
				args->flags);
	if (!(file = fd_file(current, args->fd)))
		return -EBADF;
	return do_mmap(file, &args->addr, args->len, args->prot & 7,
			args->flags, args->off);
}

long sys_munmap(void *addr, size_t len)
{
	struct vma *vma, *n;
	uintptr_t start = (uintptr_t)addr;
	uintptr_t end = start + page_align(len);

	if (end > kernel_base)
		return -EINVAL;
	if (!len || !page_aligned(addr))
		return -EINVAL;
	// ensure no VMAs in range have VM_KEEP
	list_for_each_entry(vma, &current->mm.map, chain) {
		if (vma->end < start)
			continue;
		if (vma->start >= end)
			break;
		if (vma->flags & VM_KEEP)
			return -EINVAL;
	}
	list_for_each_entry_safe(vma, n, &current->mm.map, chain) {
		if (vma->end < start)
			continue;
		if (vma->start >= end)
			break;
		if (vma->start < start) {
			vma_bisect(vma, start, vma->flags, vma->flags);
			vma = list_entry(vma->chain.next, struct vma, chain);
		}
		if (vma->end > end)
			vma_bisect(vma, end, vma->flags, vma->flags);
		vm_unmap(vma);
	}
	return 0;
}
