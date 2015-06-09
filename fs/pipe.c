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

#include <kernel/flexbuf.h>
#include <kernel/fs.h>
#include <telos/fcntl.h>
#include <telos/limits.h>

struct pipe_private {
	struct flexbuf *buf;
	struct file *write_end;
	struct file *read_end;
	struct wait_queue read_wait;
	struct wait_queue write_wait;
};

DEFINE_SLAB_CACHE(pipe_cachep, sizeof(struct pipe_private));

off_t pipe_lseek(struct inode *inode, struct file *file, off_t pos, int whence)
{
	return -ESPIPE;
}

ssize_t pipe_read(struct file *file, char *dst, size_t len, unsigned long *pos)
{
	ssize_t nr_bytes;
	struct pipe_private *pipe = file->f_private;

	while (!(nr_bytes = flexbuf_dequeue(pipe->buf, dst, len))) {
		// pipe is empty
		if (!pipe->write_end)
			return 0; // no writers: EOF
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if (wait_interruptible(&pipe->read_wait))
			return -EINTR;
	}
	wake_all(&pipe->write_wait, 0);
	return nr_bytes;
}

static size_t pipe_headroom(struct pipe_private *pipe)
{
	return PIPE_BUF - pipe->buf->size;
}

/*
 * Write @len bytes to @pipe atomically with respect to other writers.
 */
static ssize_t pipe_do_write(struct pipe_private *pipe, const char *src, size_t len)
{
	size_t nr_bytes = 0;
	while (1) {
		// write as much as we can
		size_t this_len = MIN(len-nr_bytes, pipe_headroom(pipe));
		size_t this_bytes = flexbuf_enqueue(&pipe->buf, src+nr_bytes, this_len);
		if (this_bytes)
			wake_all(&pipe->read_wait, 0);

		// termination condition in the middle of the loop...
		nr_bytes += this_bytes;
		if (nr_bytes == len)
			break;

		if (wait_interruptible(&pipe->write_wait))
			return nr_bytes ? (ssize_t)nr_bytes : -EINTR;
	}
	return nr_bytes;
}

ssize_t pipe_write(struct file *file, const char *src, size_t len, unsigned long *pos)
{
	struct pipe_private *pipe = file->f_private;
	size_t nr_bytes = 0;

	if (file->f_flags & O_NONBLOCK) {
		size_t headroom = pipe_headroom(pipe);
		if (len <= PIPE_BUF && len > headroom)
			return -EAGAIN;
		if (len > PIPE_BUF) {
			if (!headroom)
				return -EAGAIN;
			return flexbuf_enqueue(&pipe->buf, src, headroom);
		}
	}

	while (nr_bytes < len) {
		// We don't want writes of <= PIPE_BUF bytes to be interleaved
		// with other writes, so break up the write into blocks of
		// up to PIPE_BUF bytes and write each block atomically.  This
		// prevents processes from hogging the write-end with large
		// writes, while preserving the non-interleaved property.
		size_t this_len = MIN(len-nr_bytes, PIPE_BUF);
		nr_bytes += pipe_do_write(pipe, src+nr_bytes, this_len);
	}
	return nr_bytes;
}

int pipe_release(struct inode *inode, struct file *file)
{
	struct pipe_private *pipe = file->f_private;
	if (file == pipe->read_end) {
		pipe->read_end = NULL;
		wake_all(&pipe->write_wait, 0);
	}
	if (file == pipe->write_end) {
		pipe->write_end = NULL;
		wake_all(&pipe->read_wait, 0);
	}
	if (!pipe->read_end && !pipe->write_end)
		slab_free(pipe_cachep, pipe);
	return 0;
}

struct file_operations pipe_read_operations = {
	.lseek = pipe_lseek,
	.read = pipe_read,
	.release = pipe_release
};

struct file_operations pipe_write_operations = {
	.lseek = pipe_lseek,
	.write = pipe_write,
	.release = pipe_release
};

long sys_pipe(int *read_end, int *write_end, int flags)
{
	if (vm_verify(&current->mm, read_end, sizeof(*read_end), VM_WRITE))
		return -EFAULT;
	if (vm_verify(&current->mm, write_end, sizeof(*write_end), VM_WRITE))
		return -EFAULT;

	int read_fd, write_fd;
	struct file *read_file = get_empty_file();
	struct file *write_file = get_empty_file();
	struct pipe_private *pipe = slab_alloc(pipe_cachep);

	if ((read_fd = get_fd(current, 0)) < 0)
		return read_fd;
	current->filp[read_fd] = read_file;
	if ((write_fd = get_fd(current, 0)) < 0) {
		current->filp[read_fd] = NULL;
		return write_fd;
	}
	current->filp[write_fd] = write_file;

	INIT_WAIT_QUEUE(&pipe->read_wait);
	INIT_WAIT_QUEUE(&pipe->write_wait);
	pipe->read_end = read_file;
	pipe->write_end = write_file;
	pipe->buf = flexbuf_alloc(0);

	read_file->f_inode = write_file->f_inode = NULL;
	read_file->f_flags = write_file->f_flags = flags;
	read_file->f_rdev = write_file->f_rdev = 0;

	read_file->f_mode = O_READ;
	read_file->f_op = &pipe_read_operations;
	read_file->f_private = pipe;

	write_file->f_mode = O_WRITE;
	write_file->f_op = &pipe_write_operations;
	write_file->f_private = pipe;

	current->filp[read_fd] = read_file;
	current->filp[write_fd] = write_file;
	*read_end = read_fd;
	*write_end = write_fd;
	return 0;
}
