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

#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/drivers/console.h>

#include <string.h>
#include <klib.h>

#define N_CONSOLES 2

#define COL  80
#define ROW  25
#define CHR  2

#define CBUF_SIZE (COL*ROW*CHR)

#define CLR_BASE 0x3D4
#define CLR_BUF  (0xB8000 + 0xC0000000)

#define TXT_CLR   0x7
#define NUM_CLR   0xF
#define TAB_WIDTH 8

static int console_write(int fd, void *buf, int buf_len);
static int console_open(dev_t devno);
static int console_close(dev_t devno);
static int console_ioctl(int fd, unsigned long command, va_list vargs);

struct device_operations console_operations = {
	.dvinit = console_init,
	.dvopen = console_open,
	.dvclose = console_close,
	.dvread = NULL,
	.dvwrite = console_write,
	.dvioctl = console_ioctl,
	.dviint = NULL,
	.dvoint = NULL
};

struct console {
	unsigned char mem[CBUF_SIZE];
	unsigned int offset;
	unsigned int opened;
};

static struct console constab[N_CONSOLES];

static unsigned int visible = 0; /* currently visible console */

static void console_putc(unsigned char c, unsigned char attr,
		unsigned int cno);
static void cursor(int pos);

static int console_write(int fd, void *buf, int buf_len)
{
	int i;
	unsigned int cno = current->fds[fd] - DEV_CONSOLE_0;

	char *s = (char*) kmap_tmp_range(current->pgdir, (ulong) buf, buf_len);
	for (i = 0; s[i] != '\0' && i < buf_len; i++) {
		console_putc(s[i], TXT_CLR, cno);
	}
	kunmap_range((ulong) s, buf_len);

	return i;
}

static int console_open(dev_t devno)
{
	unsigned int cno = devno - DEV_CONSOLE_0;
	constab[cno].opened++;
	return 0;
}

static int console_close(dev_t devno)
{
	unsigned int cno = devno - DEV_CONSOLE_0;
	constab[cno].opened--;
	return 0;
}

int console_init(void)
{
	// TODO: probe for colour/monochrome display
	unsigned short cpos;

	/* get cursor position */
	outb(CLR_BASE, 14);
	cpos = inb(CLR_BASE+1) << 8;
	outb(CLR_BASE, 15);
	cpos |= inb(CLR_BASE+1);

	/* set initial cursor positions */
	constab[0].offset = (cpos > COL * ROW) ? 0 : cpos * 2;

	return 0;
}

/*-----------------------------------------------------------------------------
 * Switches to the given console */
//-----------------------------------------------------------------------------
int console_switch(unsigned int to)
{
	if (to >= N_CONSOLES)
		return -EINVAL;
	if (to == visible)
		return 0;

	/* swap current console to driver memory and load new console to vga memory */
	memcpy(constab[visible].mem, (void*) CLR_BUF, ROW*COL*CHR);
	memcpy((void*) CLR_BUF, constab[to].mem, ROW*COL*CHR);

	visible = to;
	return 0;
}

/*-----------------------------------------------------------------------------
 * Clears the visible console and sets the cursor position to 0 */
//-----------------------------------------------------------------------------
void clear_console(void)
{
	for (int i = 0; i < ROW; i++)
		console_putc('\n', TXT_CLR, visible);
	cursor(0);
	constab[visible].offset = 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
static int console_ioctl(int fd, unsigned long command, va_list vargs)
{
	switch (command) {
	case CONSOLE_IOCTL_SWITCH:
		return console_switch(va_arg(vargs, unsigned int));
	case CONSOLE_IOCTL_CLEAR:
		clear_console();
		return 0;
	default:
		return -EINVAL;
	}
	return -EIO;
}

/*-----------------------------------------------------------------------------
 * Update the cursor position */
//-----------------------------------------------------------------------------
static void cursor(int pos)
{
	outb(CLR_BASE,   14);
	outb(CLR_BASE+1, pos >> 8);
	outb(CLR_BASE,   15);
	outb(CLR_BASE+1, pos & 0xFF);
}

/*-----------------------------------------------------------------------------
 * Prints a character to the given console */
//-----------------------------------------------------------------------------
static void console_putc(unsigned char c, unsigned char attr,
		unsigned int cno)
{
	unsigned char *base = (cno==visible) ? (unsigned char*) CLR_BUF :
			constab[cno].mem;
	// print the character
	switch (c) {
	case '\n':
		constab[cno].offset += COL * CHR;
		/* fallthrough */
	case '\r':
		constab[cno].offset -= constab[cno].offset % (COL * CHR);
		break;
	case '\t':
		for (int i = 0; i < TAB_WIDTH; i++)
			console_putc(' ', attr, cno);
		break;
	case '\b':
		if (constab[cno].offset <= CHR)
			constab[cno].offset = 0;
		else
			constab[cno].offset -= CHR;
		*(base + constab[cno].offset) = ' ';
		break;
	default:
		*(base + constab[cno].offset) = c;
		*(base + constab[cno].offset + 1) = attr & 0xF;
		constab[cno].offset += CHR;
		break;
	}

	// scroll down
	if (constab[cno].offset >= CBUF_SIZE) {
		unsigned char *i;
		memcpy(base, base + (COL*CHR), COL*(ROW-1)*CHR);
		for (i = base + (COL*(ROW-1)*CHR); i < base + CBUF_SIZE; i += CHR)
			*i = ' ';
		constab[cno].offset -= COL*CHR;
	}
	if (cno == visible)
		cursor(constab[cno].offset / 2);
}

/* KERNEL INTERFACE */

/*-----------------------------------------------------------------------------
 * Prints a string to the visible console */
//-----------------------------------------------------------------------------
static inline void kputs(char *s, unsigned char attr)
{
	for (; *s != '\0'; s++)
		console_putc(*s, attr, visible);
}

/*-----------------------------------------------------------------------------
 * Prints a formatted string to the visible console */
//-----------------------------------------------------------------------------
int kvprintf(unsigned char clr, const char *fmt, va_list ap) {
	int ret;
	char buf[1024];

	ret = vsnprintf(buf, 1024, fmt, ap);
	kputs(buf, clr);

	return ret;
}

/*-----------------------------------------------------------------------------
 * Variadic wrapper for kvprintf */
//-----------------------------------------------------------------------------
int kprintf_clr(unsigned char clr, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = kvprintf(clr, fmt, ap);
	va_end(ap);
	return ret;
}

/*-----------------------------------------------------------------------------
 * Variadic wrapper for kvprintf which prints in the default text colour */
//-----------------------------------------------------------------------------
int kprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = kvprintf(TXT_CLR, fmt, ap);
	va_end(ap);
	return ret;
}
