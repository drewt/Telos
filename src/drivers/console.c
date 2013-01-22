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

#include <klib.h>

#define N_CONSOLES 2

#define COL  80
#define ROW  25
#define CHR  2

#define CBUF_SIZE (COL*ROW*CHR)

#define CLR_BASE 0x3D4
#define CLR_BUF  0xB8000

#define TXT_CLR   0x7
#define TAB_WIDTH 8

// TODO: use a struct for all this
static unsigned char console_mem[N_CONSOLES][CBUF_SIZE]; /* driver memory */
static volatile unsigned char *pos[N_CONSOLES]; /* cursor positions       */
static unsigned int writers[N_CONSOLES];        /* number of times opened */

static unsigned int visible;                    /* current console        */

static void console_putc (unsigned char c, unsigned char attr,
        unsigned int cno);

int console_write (int fd, void *buf, int buf_len) {
    unsigned int cno = current->fds[fd] - DEV_CONSOLE_0;
    if (cno == visible) {
        for (int i = 0; i < buf_len; i++)
            console_putc (((unsigned char*) buf)[i], TXT_CLR, cno);
    } else {

    }
    return 0;
}

int console_open (enum dev_id devno) {
    unsigned int cno = devno - DEV_CONSOLE_0;
    writers[cno]++;
    return 0;
}

int console_close (enum dev_id devno) {
    unsigned int cno = devno - DEV_CONSOLE_0;
    writers[cno]--;
    return 0;
}

int console_init (void) {
    // TODO: probe for colour/monochrome display
    pos[0]  = (volatile unsigned char*) CLR_BUF;

    // get cursor position
    uint32_t cpos;
    outb (CLR_BASE, 14);
    cpos = inb (CLR_BASE+1) << 8;
    outb (CLR_BASE, 15);
    cpos |= inb (CLR_BASE+1);
    if (cpos <= COL * ROW)
        pos[0] = (unsigned char*) CLR_BUF + cpos*2;
    else
        pos[0] = (unsigned char*) CLR_BUF;;

    for (int i = 1; i < N_CONSOLES; i++)
        pos[i]  = &console_mem[i][0];

    return 0;
}

int console_ioctl (unsigned long command, va_list vargs) {
    unsigned int to;

    if (command != CONSOLE_IOCTL_SWITCH)
        return SYSERR;

    to = va_arg (vargs, unsigned int);
    if (to >= N_CONSOLES)
        return SYSERR;

    // swap current console to driver memory and load new console to vga memory
    memcpy (&console_mem[visible][0], (void*) CLR_BUF, ROW*COL*CHR);
    memcpy ((void*) CLR_BUF, &console_mem[to][0], ROW*COL*CHR);

    visible = to;
    return 0;
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

static void console_putc (unsigned char c, unsigned char attr,
        unsigned int cno) {

    unsigned char *base = (cno==visible) ? (unsigned char*) CLR_BUF :
                                           &console_mem[cno][0];

    // print the character
    switch (c) {
    case '\n':
        pos[cno] += COL * 2; //        |
    case '\r':               // <------'
        pos[cno] -= ((uint32_t) pos[cno] - CLR_BUF) % (COL * 2);
        break;
    case '\t':
        for (int i = 0; i < TAB_WIDTH; i++)
            console_putc (' ', attr, cno);
        break;
    case '\b':
        pos[cno] -= 2;
        if ((void*) pos[cno] < (void*) CLR_BUF)
            pos[cno] = (void*) CLR_BUF;
        *pos[cno] = ' ';
        break;
    default:
        *pos[cno]++ = c;
        *pos[cno]++ = attr & 0xF;
        break;
    }

    // scroll down
    if (pos[cno] >= base + (COL*ROW*CHR)) {
        memcpy (base, base + (COL*CHR), COL*(ROW-1)*CHR);
        for (unsigned char *i = base + (COL*(ROW-1)*CHR);
                i < base + (COL*ROW*CHR); i += CHR)
            i[0] = ' ';
        pos[cno] -= COL*CHR;
    }
    cursor ((pos[cno] - base) / 2);
}
