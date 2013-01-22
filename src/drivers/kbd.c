/* kbd.c : 8042 keyboard driver
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <kernel/common.h>
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/drivers/kbd.h>

#include <klib.h>

#define KBD_CMD 0x64
#define KBD_DAT 0x60

#define KBD_BUF_SIZE 4 /* size of the internal keyboard buffer */

#define DEFAULT_EOF 4

#define NOCHAR 256

static char kbd_eof;

/* internal buffer */
static char kbd_buf[4];       /* the internal keyboard buffer */
static int  kbd_buf_next = 0; /* number of chars in the internal buffer */

/* state */
static bool is_open = false; /* TRUE when keyboard is open */
static bool reading = false; /* TRUE when keyboard is being read */
static bool echo    = false; /* TRUE when echo keyboard is open */
static bool got_eof = false; /* TRUE when EOF has been read */

/* data from user process */
static char *cpy_buf;         /* user read buffer */
static int  cpy_buf_len;      /* total length of read buffer */
static int  cpy_buf_next = 0; /* number of chars in read buffer */
static struct pcb *owner;

/*-----------------------------------------------------------------------------
 * Puts a character in the read buffer, echoing if the echo keyboard is open.
 * Returns TRUE if the read is complete (i.e. if the read buffer is full, or 
 * CR/EOF encountered) */
//-----------------------------------------------------------------------------
static inline bool put_char (char c) {
    if (c == kbd_eof) {
        got_eof = true;
        return true;
    }
    if (echo) kprintf ("%c", c);
    cpy_buf[cpy_buf_next] = c;
    cpy_buf_next++;
    return (cpy_buf_next == cpy_buf_len || c == '\n');
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void kbd_interrupt (void) {
    unsigned int c;

    if (!(inb (KBD_CMD) & 1)) {
        kprintf ("keyboard interrupt with no data\n");
    }
    else if ((c = kbtoa (inb (KBD_DAT))) != NOCHAR) {
        if (reading) {
            if (put_char (c)) {
                reading = false;
                owner->rc = cpy_buf_next;
                ready (owner);
            }
        }
        else if (kbd_buf_next != KBD_BUF_SIZE) {
            kbd_buf[kbd_buf_next] = c;
            kbd_buf_next++;
        }
    }
    pic_eoi ();
}

/*-----------------------------------------------------------------------------
 * Code shared between echo and noecho keyboard reads.
 *-----------------------------------------------------------------------------
 */
bool kbd_common_read (void) {

    cpy_buf_next = 0;

    // if EOF reached, return 0 immediately
    if (got_eof)
        return true;

    int i, j;
    bool read_finished = false;
    // copy the internal buffer into the read buffer
    for (i = 0; i < kbd_buf_next && !read_finished; i++) {
        if (put_char (kbd_buf[i]))
            read_finished = true;
    }
    // if kbd_buf was emptied...
    if (cpy_buf_next == kbd_buf_next) {
        kbd_buf_next = 0;
    }
    // otherwise, shift kbd_buf
    else {
        for (j = 0; j < kbd_buf_next - cpy_buf_len + 1; j++)
            kbd_buf[j] = kbd_buf[j+i];
        kbd_buf_next = j;
    }

    return read_finished;
}

/*-----------------------------------------------------------------------------
 * Initiate a read from the noecho keyboard device
 *-----------------------------------------------------------------------------
 */
int kbd_read (int fd, void *buf, int buf_len) {
    echo = (current->fds[fd] == DEV_KBD_ECHO);
    cpy_buf = buf;
    cpy_buf_len = buf_len;
    if (kbd_common_read ())
        return cpy_buf_next;
    
    reading = true;
    new_process ();
    return 0;
}

/*-----------------------------------------------------------------------------
 * Opens a keyboard device
 *-----------------------------------------------------------------------------
 */
int kbd_open (enum dev_id devno) {
    if (is_open)
        return SYSERR;
    enable_irq (1, 0);
    is_open = true;
    owner   = current;
    kbd_eof = DEFAULT_EOF;
    return 0;
}

/*-----------------------------------------------------------------------------
 * Closes a keyboard device
 *-----------------------------------------------------------------------------
 */
int kbd_close (enum dev_id devno) {
    if (!is_open)
        return SYSERR;
    enable_irq (1, 1);
    is_open = false;
    got_eof = false;
    owner   = NULL;
    kbd_buf_next = 0;
    return 0;
}

/*-----------------------------------------------------------------------------
 * Keyboard ioctl routine.  The only supported command is to change the
 * EOF character.  The command number to request this operation is 49, and the
 * parameter is the integer value of the character that is to become the new
 * EOF indicator
 *-----------------------------------------------------------------------------
 */
int kbd_ioctl (unsigned long command, va_list vargs) {

    if (command != KBD_IOCTL_MOD_EOF)
        return SYSERR;
    
    kbd_eof = va_arg (vargs, int);

    return 0;
}
