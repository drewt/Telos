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

#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/i386.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/slab.h>
#include <kernel/mm/vma.h>
#include <sys/mman.h>
#include <string.h>

#define MMAP_AREA_START ((void*)0x0D000000)
#define MMAP_AREA_END   ((void*)0x0F000000)

struct mmap_private {
	struct file *file;
	unsigned long off;
	unsigned int ref;
};

static DEFINE_SLAB_CACHE(mmap_private_cachep, sizeof(struct mmap_private));

static inline struct mmap_private *alloc_private(struct file *file,
		unsigned long off)
{
	struct mmap_private *private = slab_alloc(mmap_private_cachep);
	if (!private)
		return NULL;
	private->file = file;
	private->off = off;
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
	unsigned long base = page_base((ulong)addr);
	unsigned long pos = private->off + (vma->start - base);

	// FIXME: should wait until memory is available...
	if (!(frame = kalloc_frame(vma->flags)))
		return -ENOMEM;
	if (!(vaddr = kmap_tmp_page(frame->addr))) {
		kfree_frame(frame);
		return -ENOMEM;
	}
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
	unsigned long base = page_base((ulong)addr);
	unsigned long pos = private->off + (vma->start - base);

	len = MIN(len, private->file->f_inode->i_size - pos);
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

struct vma_operations mmap_vma_ops = {
	.map = mmap_map,
	.unmap = mmap_unmap,
	.clone = mmap_clone,
};

struct vma_operations mmap_shared_vma_ops = {
	.map = mmap_map,
	.writeback = mmap_writeback,
	.unmap = mmap_unmap,
	.clone = mmap_clone,
};

int do_mmap(struct file *file, void **addr, size_t len, int prot, int flags,
		unsigned long off)
{
	void *end;
	struct vma *vma;
	struct mmap_private *private;

	private = alloc_private(file, off);
	if (!private)
		return -ENOMEM;

	if (flags & MAP_FIXED) {
		end = (char*)*addr + len;
	} else {
		*addr = MMAP_AREA_START;
		end  = MMAP_AREA_END;
	}

	vma = create_vma(&current->mm, *addr, end, len, prot | VM_CLOEXEC);
	if (!vma) {
		slab_free(mmap_private_cachep, private);
		return -ENOMEM;
	}

	vma->private = private;
	vma->op = (flags & MAP_SHARED) ? &mmap_shared_vma_ops : &mmap_vma_ops;
	return 0;
}

long sys_mmap(struct __mmap_args *args)
{
	struct file *file;
	if (vm_verify(&current->mm, args, sizeof(*args), VM_READ | VM_WRITE))
		return -EFAULT;
	if (!(file = fd_file(current, args->fd)))
		return -EBADF;
	if (args->flags & MAP_FIXED)
		return -ENOTSUP;
	return do_mmap(file, &args->addr, args->len, args->prot & 7,
			args->flags, args->off);
}
