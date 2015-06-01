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

#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/fs.h>
#include <kernel/tty.h>
#include <kernel/drivers/console.h>
#include <kernel/drivers/keyboard.h>
#include <telos/major.h>
#include <string.h>

#define NR_CONSOLES 2

#define COL 80
#define ROW 25
#define CHR 2
#define CONSOLE_SIZE (COL*ROW*CHR)

#define TAB_WIDTH 8
#define DEFAULT_ATTR 0x7

#define VGA_COLOR_BASE 0x3D4
#define VGA_COLOR_BUF  ((void*)(0xB8000 + 0xC0000000))

struct console {
	unsigned char mem[CONSOLE_SIZE];
	unsigned int offset;
};

static struct tty *console_tty_table[NR_CONSOLES];
static struct console constab[NR_CONSOLES];
static unsigned int visible = 0;

static void cursor(int pos)
{
	outb(VGA_COLOR_BASE,   14);
	outb(VGA_COLOR_BASE+1, pos >> 8);
	outb(VGA_COLOR_BASE,   15);
	outb(VGA_COLOR_BASE+1, pos & 0xFF);
}

static unsigned char *console_mem(unsigned int index)
{
	if (index == visible)
		return VGA_COLOR_BUF;
	return constab[index].mem;
}

static void console_putc(unsigned char c, unsigned char attr, unsigned int idx)
{
	struct console *cons = &constab[idx];
	unsigned char *base = console_mem(idx);

	// print the character
	switch (c) {
	case '\n':
		cons->offset += COL*CHR;
		// fallthrough
	case '\r':
		cons->offset -= cons->offset % (COL*CHR);
		break;
	case '\t':
		for (int i = 0; i < TAB_WIDTH; i++)
			console_putc(' ', attr, idx);
		break;
	case '\b':
		if (cons->offset <= CHR)
			cons->offset = 0;
		else
			cons->offset -= CHR;
		*(base + cons->offset) = ' ';
		break;
	default:
		*(base + cons->offset) = c;
		*(base + cons->offset + 1) = attr & 0xF;
		cons->offset += CHR;
		break;
	}

	// scroll down
	if (cons->offset >= CONSOLE_SIZE) {
		unsigned char *i;
		memcpy(base, base + (COL*CHR), COL*(ROW-1)*CHR);
		for (i = base + (COL*(ROW-1)*CHR); i < base + CONSOLE_SIZE; i += CHR)
			*i = ' ';
		cons->offset -= COL*CHR;
	}
	if (idx == visible)
		cursor(cons->offset / 2);
}

void console_clear(unsigned int index)
{
	unsigned char *base = console_mem(index);
	for (int i = 0; i < CONSOLE_SIZE; i += CHR) {
		base[i] = ' ';
		base[i+1] = DEFAULT_ATTR;
	}
	cursor(0);
	constab[index].offset = 0;
}

int console_switch(unsigned int index)
{
	if (index >= NR_CONSOLES)
		return -EINVAL;
	if (index == visible)
		return 0;

	memcpy(constab[visible].mem, VGA_COLOR_BUF, CONSOLE_SIZE);
	memcpy(VGA_COLOR_BUF, constab[index].mem, CONSOLE_SIZE);
	cursor(constab[index].offset / 2);
	visible = index;
	return 0;
}

int console_open(struct tty *tty, struct file *file)
{
	struct console *cons = &constab[tty->index];
	if (tty->open)
		return 0;
	cons->offset = 0;
	tty->tty_private = cons;
	console_clear(tty->index);
	return 0;
}

int console_close(struct tty *tty, struct file *file)
{
	if (tty->open)
		return 0;
	console_clear(tty->index);
	return 0;
}

ssize_t console_write(struct tty *tty, const char *buf, size_t len)
{
	for (size_t i = 0; i < len; i++)
		console_putc(buf[i], TXT_CLR, tty->index);
	return len;
}

int console_ioctl(struct tty *tty, unsigned int command, unsigned long arg)
{
	switch (command) {
	case CONSOLE_IOCTL_SWITCH:
		console_switch(tty->index);
		break;
	case CONSOLE_IOCTL_CLEAR:
		console_clear(tty->index);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

void int_keyboard(void)
{
	unsigned s, c;
	struct tty *tty = console_tty_table[visible];

	s = inb(KEYBOARD_DATA_PORT);
	c = kbtoa(s);

	// F1... -- switch virtual console
	if (s >= 59 && s < 59 + NR_CONSOLES) {
		console_switch(s - 59);
		goto end;
	}

	if (tty && c != NOCHAR)
		tty_insert_char(tty, c);
end:
	pic_eoi();
}

/*
 * Prints a string to the visible console
 */
static inline void kputs(char *s, unsigned char attr)
{
	for (; *s != '\0'; s++)
		console_putc(*s, attr, visible);
}

/*
 * Prints a formatted string to the visible console
 */
int _kvprintf(unsigned char attr, const char *fmt, va_list ap)
{
	int ret;
	char buf[1024];

	ret = vsnprintf(buf, 1024, fmt, ap);
	kputs(buf, attr);

	return ret;
}

/*
 * Variadic wrapper for kvprintf
 */
int _kprintf(unsigned char attr, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = _kvprintf(attr, fmt, ap);
	va_end(ap);
	return ret;
}

struct tty_operations console_tty_ops = {
	.open = console_open,
	.close = console_close,
	.write = console_write,
	.ioctl = console_ioctl,
};

struct tty_driver pc_console_driver = {
	.major = TTY_MAJOR,
	.minor_start = 0,
	.num = NR_CONSOLES,
	.name = "console",
	.ttys = console_tty_table,
	.op = &console_tty_ops,
};

SYSINIT(console, SUB_DRIVER)
{
	tty_register_driver(&pc_console_driver);
	enable_irq(1, 0);
}
