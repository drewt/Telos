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

#ifndef _KERNEL_TTY_H_
#define _KERNEL_TTY_H_

#include <kernel/list.h>
#include <kernel/fs.h>
#include <kernel/wait.h>

#define NR_TTYS 64
#define TTY_BUFFER_SIZE 16

struct tty;

struct tty_buffer {
	struct list_head chain;
	size_t pos;
	size_t read;
	unsigned char data[TTY_BUFFER_SIZE];
};

struct tty_operations {
	int (*open)(struct tty *, struct file *);
	int (*close)(struct tty *, struct file *);
	ssize_t (*write)(struct tty *, const char *buf, size_t len);
	int (*ioctl)(struct tty *, unsigned int, unsigned long);
};

struct tty_driver {
	struct list_head chain;
	int major;
	int minor_start;
	unsigned int num;
	char *name;
	int flags;
	struct tty **ttys;
	struct tty_operations *op;
};

struct tty {
	int flags;
	unsigned int open;
	unsigned int index;
	struct tty_driver *driver;
	struct tty_buffer *buffer;
	struct wait_queue wait;
	struct list_head flushed;
	void *tty_private;
};

void tty_register_driver(struct tty_driver *driver);
int tty_insert_char(struct tty *tty, unsigned char c);

#endif
