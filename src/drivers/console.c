/* console.c : vga console driver
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
#include <kernel/drivers/console.h>

#include <errnodefs.h>
#include <string.h>
#include <klib.h>

#define N_CONSOLES 2

#define COL  80
#define ROW  25
#define CHR  2

#define CBUF_SIZE (COL*ROW*CHR)

#define CLR_BASE 0x3D4
#define CLR_BUF  0xB8000

#define TXT_CLR   0x7
#define NUM_CLR   0xF
#define TAB_WIDTH 8

struct console {
    unsigned char mem[CBUF_SIZE];
    volatile unsigned char *pos;
    unsigned int opened;
};

static struct console constab[N_CONSOLES];

static unsigned int visible = 0; /* currently visible console */

static void console_putc (unsigned char c, unsigned char attr,
        unsigned int cno);
static void cursor (int pos);

int console_write (int fd, void *buf, int buf_len) {
    int i;
    unsigned char *s = buf;
    unsigned int cno = current->fds[fd] - DEV_CONSOLE_0;
    for (i = 0; s[i] != '\0' && i < buf_len; i++) {
        console_putc (s[i], TXT_CLR, cno);
    }
    return i;
}

int console_open (enum dev_id devno) {
    unsigned int cno = devno - DEV_CONSOLE_0;
    constab[cno].opened++;
    return 0;
}

int console_close (enum dev_id devno) {
    unsigned int cno = devno - DEV_CONSOLE_0;
    constab[cno].opened--;
    return 0;
}

int console_init (void) {
    // TODO: probe for colour/monochrome display
    unsigned char *buf = (unsigned char*) CLR_BUF;
    u16 cpos;

    // get cursor position
    outb (CLR_BASE, 14);
    cpos = inb (CLR_BASE+1) << 8;
    outb (CLR_BASE, 15);
    cpos |= inb (CLR_BASE+1);

    // set initial cursor positions
    constab[0].pos = (cpos > COL * ROW) ? buf : buf + cpos*2;
    for (int i = 1; i < N_CONSOLES; i++)
        constab[i].pos  = constab[i].mem;

    return 0;
}

/*-----------------------------------------------------------------------------
 * Switches to the given console */
//-----------------------------------------------------------------------------
int console_switch (unsigned int to) {

    if (to >= N_CONSOLES)
        return -EINVAL;

    // swap current console to driver memory and load new console to vga memory
    memcpy (constab[visible].mem, (void*) CLR_BUF, ROW*COL*CHR);
    memcpy ((void*) CLR_BUF, constab[to].mem, ROW*COL*CHR);

    visible = to;
    return 0;
}

/*-----------------------------------------------------------------------------
 * Clears the visible console and sets the cursor position to 0 */
//-----------------------------------------------------------------------------
void clear_console (void) {
    for (int i = 0; i < ROW; i++)
        console_putc ('\n', TXT_CLR, visible);
    cursor (0);
    constab[visible].pos = (unsigned char*) CLR_BUF;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int console_ioctl (int fd, unsigned long command, va_list vargs) {
    switch (command) {
    case CONSOLE_IOCTL_SWITCH:
        return console_switch (va_arg (vargs, unsigned int));
    case CONSOLE_IOCTL_CLEAR:
        clear_console ();
        return 0;
    default:
        return -EINVAL;
    }
    return -EIO;
}

/*-----------------------------------------------------------------------------
 * updates cursor position */
//-----------------------------------------------------------------------------
static void cursor (int pos) {
    outb (CLR_BASE, 14);
    outb (CLR_BASE+1, pos >> 8);
    outb (CLR_BASE, 15);
    outb (CLR_BASE+1, pos & 0xFF);
}

/*-----------------------------------------------------------------------------
 * Prints a character to the given console */
//-----------------------------------------------------------------------------
static void console_putc (unsigned char c, unsigned char attr,
        unsigned int cno) {

    unsigned char *base = (cno==visible) ? (unsigned char*) CLR_BUF :
                                           constab[cno].mem;

    // print the character
    switch (c) {
    case '\n':
        constab[cno].pos += COL * 2; //        |
    case '\r':               // <------'
        constab[cno].pos -= ((unsigned long) constab[cno].pos - CLR_BUF)
            % (COL * 2);
        break;
    case '\t':
        for (int i = 0; i < TAB_WIDTH; i++)
            console_putc (' ', attr, cno);
        break;
    case '\b':
        constab[cno].pos -= 2;
        if ((void*) constab[cno].pos < (void*) CLR_BUF)
            constab[cno].pos = (void*) CLR_BUF;
        *constab[cno].pos = ' ';
        break;
    default:
        *constab[cno].pos++ = c;
        *constab[cno].pos++ = attr & 0xF;
        break;
    }

    // scroll down
    if (constab[cno].pos >= base + (COL*ROW*CHR)) {
        memcpy (base, base + (COL*CHR), COL*(ROW-1)*CHR);
        for (unsigned char *i = base + (COL*(ROW-1)*CHR);
                i < base + (COL*ROW*CHR); i += CHR)
            i[0] = ' ';
        constab[cno].pos -= COL*CHR;
    }
    cursor ((constab[cno].pos - base) / 2);
}

/* KERNEL INTERFACE */

/*-----------------------------------------------------------------------------
 * Prints a string to the visible console */
//-----------------------------------------------------------------------------
static inline void kputs (char *s, unsigned char attr) {
    for (; *s != '\0'; s++)
        console_putc (*s, attr, visible);
}

/*-----------------------------------------------------------------------------
 * Prints a formatted string to the visible console */
//-----------------------------------------------------------------------------
int kvprintf (unsigned char clr, const char *fmt, va_list ap) {
    int i;
    char buf[33];

    for (i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
            // numbers
            case 'b':
                kputs (itoa (va_arg (ap, int), buf, 2), NUM_CLR);
            case 'd':
            case 'i':
                kputs (itoa (va_arg (ap, int), buf, 10), NUM_CLR);
                break;
            case 'o':
                kputs (itoa (va_arg (ap, int), buf, 8), NUM_CLR);
            case 'x':
                kputs ("0x", NUM_CLR);
                kputs (itoa (va_arg (ap, int), buf, 16), NUM_CLR);
                break;
            // strings n chars
            case 'c':
                console_putc (va_arg (ap, int), clr, visible);
                break;
            case 's':
                kputs (va_arg (ap, char*), TXT_CLR);
                break;
            case '%':
                console_putc ('%', clr, visible);
                break;
            default:
                // bad/unsupported format string; abort
                goto end;
                break;
            }
        } else {
            console_putc (fmt[i], clr, visible);
        }
    }
end:
    return i;
}


/*-----------------------------------------------------------------------------
 * Variadic wrapper for kvprintf */
//-----------------------------------------------------------------------------
int kprintf_clr (unsigned char clr, const char *fmt, ...) {
    va_list ap;
    va_start (ap, fmt);
    int ret = kvprintf (clr, fmt, ap);
    va_end (ap);
    return ret;
}

/*-----------------------------------------------------------------------------
 * Variadic wrapper for kvprintf which prints in the default text colour */
//-----------------------------------------------------------------------------
int kprintf (const char *fmt, ...) {
    va_list ap;
    va_start (ap, fmt);
    int ret = kvprintf (TXT_CLR, fmt, ap);
    va_end (ap);
    return ret;
}
