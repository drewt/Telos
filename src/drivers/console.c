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
#include <kernel/device.h>
#include <kernel/drivers/console.h>

#include <klib.h>

#define N_CONSOLES 2

#define COL  80
#define ROW  25
#define CHR  2
#define SCRS 1

#define CBUF_SIZE   (COL*ROW*CHR*SCRS)
#define DISPLAY_OFFSET (COL*ROW*CHR*(SCRS-1))

#define CLR_BASE 0x3D4
#define CLR_BUF  0xB8000

static char console_mem[N_CONSOLES][CBUF_SIZE]; /* driver memory          */
static volatile unsigned char *pos[N_CONSOLES]; /* cursor positions       */
static unsigned int writers[N_CONSOLES];        /* number of times opened */
static unsigned int visible;                    /* current console        */


int console_write (void *buf, int buf_len) {
    
    return SYSERR; // TODO
}

int console_open (enum dev_id devno) {
    unsigned int cno = devno - CONSOLE_0;
    writers[cno]++;
    return SYSERR;
}

int console_close (enum dev_id devno) {
    unsigned int cno = devno - CONSOLE_0;
    writers[cno]--;
    return SYSERR;
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
