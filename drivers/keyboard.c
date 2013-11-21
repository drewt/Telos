
#include <kernel/common.h>
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/drivers/kbd.h>
#include <kernel/drivers/console.h>
#include <kernel/list.h>

#include <klib.h>

#define KBD_CMD 0x64
#define KBD_DAT 0x60

#define NOCHAR 256

#define CPY_BUF_LEN 64

static int kbd_init(void);
static void kbd_interrupt(void);
static int kbd_open(dev_t devno);
static int kbd_close(dev_t devno);
static int kbd_read(int fd, void *buf, int nbytes);
static int kbd_ioctl(int fd, unsigned long command, va_list vargs);

struct device_operations kbd_operations  = {
	.dvinit = kbd_init,
	.dvopen = kbd_open,
	.dvclose = kbd_close,
	.dvread = kbd_read,
	.dvwrite = NULL,
	.dvioctl = kbd_ioctl,
	.dviint = kbd_interrupt,
	.dvoint = NULL
};

static LIST_HEAD(work_q);

static char kbd_eof = 4;

/* state */
static bool reading = false;
static bool echo    = false;
static bool got_eof = false;

/* user buffer data */
static char   *usr_buf;
static size_t usr_buf_len;
static size_t usr_buf_next;

static char   cpy_buf[CPY_BUF_LEN];
static size_t cpy_buf_next;

static struct pcb *reader;

static inline void send_to_reader(void)
{
	copy_to_userspace(reader->pgdir, usr_buf+usr_buf_next, cpy_buf,
			cpy_buf_next);
	usr_buf_next += cpy_buf_next;
}

static bool put_char(char c)
{
	if (c == kbd_eof) {
		send_to_reader();
		return got_eof = true;
	}

	if (echo)
		kprintf("%c", c);

	cpy_buf[cpy_buf_next++] = c;
	if (usr_buf_next + cpy_buf_next == usr_buf_len || c == '\n') {
		send_to_reader();
		return true;
	}

	if (cpy_buf_next == CPY_BUF_LEN)
		send_to_reader();
	return false;
}

static void kbd_interrupt(void)
{
	unsigned c, s;

	s = inb(KBD_DAT);

	if (s >= 59 && s <= 60) {
		console_switch(s - 59);
	} else if (reading && (c = kbtoa(s)) != NOCHAR) {
		if (put_char(c)) {
			reader->rc = usr_buf_next;
			ready(reader);

			if ((reader = (struct pcb*) dequeue(&work_q))) {
				echo = (reader->fds[reader->pbuf.id] == DEV_KBD_ECHO);
				usr_buf = reader->pbuf.buf;
				usr_buf_len = reader->pbuf.len;
				usr_buf_next = cpy_buf_next = 0;
			} else {
				reading = false;
			}
		}
	}
	pic_eoi();
}

static int kbd_read(int fd, void *buf, int buf_len)
{
	if (reading) {
		current->pbuf = (struct pbuf)
			{ .buf = buf, .len = buf_len, .id = fd };
		enqueue(&work_q, (list_entry_t) current);
		new_process();
		return 0;
	}

	if (got_eof)
		return true;

	reader = current;
	echo = (current->fds[fd] == DEV_KBD_ECHO);
	usr_buf = buf;
	usr_buf_len = buf_len;
	usr_buf_next = cpy_buf_next = 0;

	reading = true;
	new_process();

	return 0;
}

static int kbd_open(dev_t devno)
{
	return 0;
}

static int kbd_close(dev_t devno)
{
	return 0;
}

static int kbd_init(void)
{
	enable_irq(1, 0);
	return 0;
}

static int kbd_ioctl(int fd, ulong command, va_list vargs)
{
	if (command != KBD_IOCTL_MOD_EOF)
		return -1;

	kbd_eof = va_arg(vargs, int);

	return 0;
}
