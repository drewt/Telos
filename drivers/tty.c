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
#include <kernel/list.h>
#include <kernel/tty.h>
#include <kernel/mm/kmalloc.h>
#include <sys/major.h>
#include <string.h>

struct tty tty_table[NR_TTYS];
LIST_HEAD(tty_drivers);
LIST_HEAD(tty_buffers);

static struct tty *init_tty(struct tty *tty)
{
	tty->open = 0;
	tty->flags = 1;
	tty->driver = NULL;
	tty->buffer = NULL;
	INIT_LIST_HEAD(&tty->readers);
	INIT_LIST_HEAD(&tty->flushed);
	return tty;
}

static struct tty *alloc_tty(void)
{
	for (int i = 0; i < NR_TTYS; i++)
		if (!tty_table[i].flags)
			return init_tty(&tty_table[i]);
	return NULL;
}

static void free_tty(struct tty *tty)
{
	tty->flags = 0;
}

static struct tty_buffer *alloc_tty_buffer(void)
{
	struct tty_buffer *buffer;
	if (!list_empty(&tty_buffers))
		buffer = list_pop(&tty_buffers, struct tty_buffer, chain);
	else if (!(buffer = kmalloc(sizeof(*buffer))))
		return NULL;
	buffer->pos = 0;
	buffer->read = 0;
	return buffer;
}

static void free_tty_buffer(struct tty_buffer *buffer)
{
	list_push(&buffer->chain, &tty_buffers);
}

static struct tty_driver *get_tty_driver(dev_t device)
{
	struct tty_driver *d;
	list_for_each_entry(d, &tty_drivers, chain) {
		if (d->major == (int)major(device))
			return d;
	}
	return NULL;
}

static struct tty *tty_driver_lookup_tty(struct tty_driver *driver, int index)
{
	return driver->ttys[index];
}

static struct tty *file_tty(struct file *f)
{
	return f->f_private;
}

static void tty_init_file(struct tty *tty, struct file *f)
{
	f->f_private = tty;
}

static size_t tty_buffer_copy(struct tty_buffer *src, char *dst, size_t len)
{
	size_t bytes = MIN(len, src->pos - src->read);
	memcpy(dst, src->data + src->read, bytes);
	src->read += bytes;
	return bytes;
}

static int tty_buffer_empty(struct tty_buffer *buf)
{
	return buf->read == buf->pos;
}

static ssize_t tty_read(struct file *f, char *buf, size_t len)
{
	size_t bytes = 0;
	struct tty *tty = file_tty(f);

	while (bytes < len) {
		struct tty_buffer *tty_buf;
		list_add_tail(&current->chain, &tty->readers);
		while (list_empty(&tty->flushed)) {
			if (schedule())
				return -EINTR;
		}

		tty_buf = list_first_entry(&tty->flushed, struct tty_buffer, chain);
		bytes += tty_buffer_copy(tty_buf, buf + bytes, len - bytes);

		if (tty_buffer_empty(tty_buf)) {
			list_del(&tty_buf->chain);
			free_tty_buffer(tty_buf);
		}
	}
	return bytes;
}

static ssize_t tty_write(struct file *f, const char *buf, size_t len)
{
	struct tty *tty = file_tty(f);

	if (!tty->driver->op->write)
		return -EIO;
	return tty->driver->op->write(tty, buf, len);
}

static int tty_open(struct inode *inode, struct file *file)
{
	int rc;
	unsigned int index;
	struct tty *tty;
	struct tty_driver *driver;

	if (!(driver = get_tty_driver(inode->i_rdev)))
		return -ENXIO;
	index = minor(inode->i_rdev) - driver->minor_start;
	if (index >= driver->num)
		return -ENXIO;
	if (!(tty = tty_driver_lookup_tty(driver, index))) {
		if (!(tty = alloc_tty()))
			return -ENOMEM;
		tty->driver = driver;
	}
	tty_init_file(tty, file);
	if (tty->driver->op->open)
		if ((rc = tty->driver->op->open(tty, file)))
			return rc;
	tty->driver->ttys[index] = tty;
	tty->open++;
	return 0;
}

static int tty_release(struct inode *inode, struct file *file)
{
	int rc;
	struct tty *tty;
	struct tty_driver *driver;

	if (!(driver = get_tty_driver(inode->i_rdev)))
		return -ENXIO;
	if (!(tty = tty_driver_lookup_tty(driver, minor(inode->i_rdev))))
		return -ENXIO;
	if (tty->driver->op->close)
		if ((rc = tty->driver->op->close(tty, file)))
			return rc;
	if (--tty->open == 0)
		free_tty(tty);
	return 0;
}

static int tty_ioctl(struct inode *inode, struct file *file,
		unsigned int command, unsigned long arg)
{
	struct tty *tty = file_tty(file);
	switch (command) {
	// TODO: generic tty ioctls
	default:
		if (tty->driver->op->ioctl)
			return tty->driver->op->ioctl(tty, command, arg);
	}
	return -EINVAL;
}

struct file_operations tty_fops = {
	.read = tty_read,
	.write = tty_write,
	.open = tty_open,
	.release = tty_release,
	.ioctl = tty_ioctl,
};

struct inode_operations tty_iops = {
	.default_file_ops = &tty_fops,
};

void tty_register_driver(struct tty_driver *driver)
{
	register_chrdev(driver->major, driver->name, &tty_fops);
	list_add_tail(&driver->chain, &tty_drivers);
}

static void tty_buffer_flush(struct tty *tty)
{
	list_add_tail(&tty->buffer->chain, &tty->flushed);
	if (!list_empty(&tty->readers))
		wake(list_pop(&tty->readers, struct pcb, chain), 0);
	tty->buffer = alloc_tty_buffer();
}

static inline void _tty_insert_char(struct tty *tty, unsigned char c)
{
	tty->buffer->data[tty->buffer->pos++] = c;
}

int tty_insert_char(struct tty *tty, unsigned char c)
{
	if (!tty->buffer && !(tty->buffer = alloc_tty_buffer()))
		return -ENOMEM;
	switch (c) {
	case '\b':
		if (tty->buffer->pos > 0)
			tty->buffer->pos--;
		break;
	case '\n':
		_tty_insert_char(tty, c);
		tty_buffer_flush(tty);
		return 0;
	default:
		_tty_insert_char(tty, c);
	}
	if (tty->buffer->pos >= TTY_BUFFER_SIZE)
		tty_buffer_flush(tty);
	return 0;
}

static void tty_sysinit(void)
{
}
