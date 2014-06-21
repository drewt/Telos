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

#define READ_BUF_SIZE 512

struct console {
	/* output */
	unsigned char mem[CBUF_SIZE];
	unsigned int offset;
	unsigned int opened;

	/* input */
	unsigned char buf[READ_BUF_SIZE];
	size_t buffered;
	size_t buf_pos;
	bool reading;
	bool flush;
	bool got_eof;

	struct list_head work_q;
	struct pcb *reader;
	char *usr_buf;
	size_t usr_buf_len;
	size_t usr_buf_next;
};

static struct console constab[N_CONSOLES];

static unsigned int visible = 0; /* currently visible console */

static void console_putc(unsigned char c, unsigned char attr,
		unsigned int cno);
static void cursor(int pos);

/*
 * KEYBOARD
 */

#define KBD_CMD 0x64
#define KBD_DAT 0x60

#define NOCHAR 256

static char kbd_eof = 4;

/* push console buffer to user */
static size_t send_to_reader(struct console *con)
{
	void *to = con->usr_buf + con->usr_buf_next;
	void *from = con->buf + con->buf_pos;
	size_t buffered = con->buffered - con->buf_pos;
	size_t len = buffered < con->usr_buf_len ? buffered : con->usr_buf_len;

	copy_to_user(con->reader, to, from, len);

	con->buf_pos += len;
	if (con->buf_pos == READ_BUF_SIZE || con->buf_pos == con->buffered) {
		con->buf_pos = con->buffered = 0;
		con->flush = false;
	}

	return len;
}

/* returns TRUE if current read should return (EOF, full buffer, or newline) */
static bool put_char(struct console *con, char c)
{
	if (c == kbd_eof)
		return con->got_eof = true;

	console_putc(c, TXT_CLR, visible);

	if (c == '\b' && con->buffered > con->buf_pos)
		con->buffered--;
	else if (c != '\b') {
		con->buf[con->buffered++] = c;
		if (con->buffered == READ_BUF_SIZE || c == '\n')
			return true;
	}
	return false;
}

static void console_iint(void)
{
	unsigned c, s;
	struct console *con = &constab[visible];

	s = inb(KBD_DAT);
	c = kbtoa(s);

	/* F1... -- switch virtual console */
	if (s >= 59 && s <= 60) {
		console_switch(s - 59);
		goto end;
	}

	if (c == NOCHAR)
		goto end;

	if ((con->flush = put_char(con, c)) && con->reading) {
		con->reader->rc = send_to_reader(con);
		ready(con->reader);

		con->reader = list_dequeue(&con->work_q, struct pcb, chain);
		if (con->reader) {
			con->usr_buf = con->reader->pbuf.buf;
			con->usr_buf_len = con->reader->pbuf.len;
			con->usr_buf_next = 0;
		} else {
			con->reading = false;
		}
	}
end:
	pic_eoi();
}

static int console_read(int fd, void *buf, int buf_len)
{
	struct console *con = &constab[current->fds[fd] - DEV_CONSOLE_0];

	/* read in progress: queue up work request and block */
	if (con->reading) {
		current->pbuf = (struct pbuf)
			{ .buf = buf, .len = buf_len, .id = fd };
		list_add_tail(&current->chain, &con->work_q);
		new_process();
		return 0;
	}

	con->reader = current;
	con->usr_buf = buf;
	con->usr_buf_len = buf_len;
	con->usr_buf_next = 0;

	/* data ready: copy to user */
	if (con->flush)
		return send_to_reader(con);

	/* data not ready: block */
	con->reading = true;
	new_process();

	return 0;
}

/*
 * TEXT-MODE VGA
 */

static int console_write(int fd, void *buf, int buf_len)
{
	int i;
	unsigned int cno = current->fds[fd] - DEV_CONSOLE_0;

	char *s = buf;
	for (i = 0; s[i] != '\0' && i < buf_len; i++) {
		console_putc(s[i], TXT_CLR, cno);
	}

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

int console_early_init(void)
{
	// TODO: probe for colour/monochrome display
	unsigned short cpos;

	for (int i = 0; i < N_CONSOLES; i++) {
		constab[i].offset = 0;
		constab[i].buffered = 0;
		constab[i].buf_pos = 0;
		constab[i].flush = false;
		constab[i].reading = false;
		constab[i].got_eof = false;
		INIT_LIST_HEAD(&constab[i].work_q);
	}

	/* get cursor position */
	outb(CLR_BASE, 14);
	cpos = inb(CLR_BASE+1) << 8;
	outb(CLR_BASE, 15);
	cpos |= inb(CLR_BASE+1);

	constab[0].offset = (cpos > COL * ROW) ? 0 : cpos * 2;

	return 0;
}

int console_init(void)
{
	enable_irq(1, 0);
	return 0;
}

/*
 * Switches to the given console
 */
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

/*
 * Clears the visible console and sets the cursor position to 0
 */
void clear_console(void)
{
	for (int i = 0; i < ROW; i++)
		console_putc('\n', TXT_CLR, visible);
	cursor(0);
	constab[visible].offset = 0;
}

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

/*
 * Update the cursor position
 */
static void cursor(int pos)
{
	outb(CLR_BASE,   14);
	outb(CLR_BASE+1, pos >> 8);
	outb(CLR_BASE,   15);
	outb(CLR_BASE+1, pos & 0xFF);
}

/*
 * Prints a character to the given console
 */
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

/*
 * KERNEL INTERFACE
 */

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
int kvprintf(unsigned char clr, const char *fmt, va_list ap) {
	int ret;
	char buf[1024];

	ret = vsnprintf(buf, 1024, fmt, ap);
	kputs(buf, clr);

	return ret;
}

/*
 * Variadic wrapper for kvprintf
 */
int kprintf_clr(unsigned char clr, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = kvprintf(clr, fmt, ap);
	va_end(ap);
	return ret;
}

/*
 * Variadic wrapper for kvprintf which prints in the default text colour
 */
int kprintf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = kvprintf(TXT_CLR, fmt, ap);
	va_end(ap);
	return ret;
}

struct device_operations console_operations = {
	.dvinit = console_init,
	.dvopen = console_open,
	.dvclose = console_close,
	.dvread = console_read,
	.dvwrite = console_write,
	.dvioctl = console_ioctl,
	.dviint = console_iint,
	.dvoint = NULL
};
