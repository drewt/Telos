/* kprintf.c : printing routines
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

#include <stdarg.h>

#include <kernel/common.h>
#include <kernel/i386.h>
#include <klib.h>

#define TXT_CLR   0x07
#define NUM_CLR   0x0F

static void kputc_clr (unsigned char c, unsigned char attr);

/*-----------------------------------------------------------------------------
 * Prints a null-terminated string to the console */
//-----------------------------------------------------------------------------
void kputs (char *s) {
    for (; *s != '\0'; s++)
        kputc_clr (*s, TXT_CLR);
}

/*-----------------------------------------------------------------------------
 * Prints a null-terminated string to the console in a given colour*/
//-----------------------------------------------------------------------------
void kputs_clr (char *s, unsigned char clr) {
    for (; *s != '\0'; s++)
        kputc_clr (*s, clr);
}

/*-----------------------------------------------------------------------------
 * Prints a null-terminated string to the console in many colours */
//-----------------------------------------------------------------------------
void kputs_rainbow (char *s) {
    for (int i = 0; *s != '\0'; s++, i++)
        kputc_clr (*s, (i%6) + 9);
}

/*-----------------------------------------------------------------------------
 * */
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
                kputs (itoa (va_arg (ap, int), buf, 2));
            case 'd':
            case 'i':
                kputs_clr (itoa (va_arg (ap, int), buf, 10), NUM_CLR);
                break;
            case 'o':
                kputs_clr (itoa (va_arg (ap, int), buf, 8), NUM_CLR);
            case 'x':
                kputs_clr ("0x", NUM_CLR);
                kputs_clr (itoa (va_arg (ap, int), buf, 16), NUM_CLR);
                break;
            // strings n chars
            case 'c':
                kputc_clr (va_arg (ap, int), TXT_CLR);
                break;
            case 's':
                kputs (va_arg (ap, char*));
                break;
            case '%':
                kputc_clr ('%', clr);
                break;
            default:
                // bad/unsupported format string; abort
                goto end;
                break;
            }
        } else {
            kputc_clr (fmt[i], clr);
        }
    }
end:
    return i;
}

int kprintf_clr (unsigned char clr, const char *fmt, ...) {
    va_list ap;
    va_start (ap, fmt);
    int ret = kvprintf (clr, fmt, ap);
    va_end (ap);
    return ret;
}

/*-----------------------------------------------------------------------------
 * variable argument version of kprintf */
//-----------------------------------------------------------------------------
int kprintf (const char *fmt, ...) {
    va_list ap;
    va_start (ap, fmt);
    int ret = kvprintf (TXT_CLR, fmt, ap);
    va_end (ap);
    return ret;
}

/*-----------------------------------------------------------------------------
 * Prints a single character to the console */
//-----------------------------------------------------------------------------
void kputc (unsigned char c) {
    kputc_clr (c, TXT_CLR);
}

/* cursor() and kputc_clr() functions below based on Berkeley code.  Copyright
 * notice follows.
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)cga.c	5.3 (Berkeley) 4/28/91
 */

#define COL       80
#define ROW       25
#define MONO_BASE 0x3B4
#define MONO_BUF  0xB0000
#define CLR_BASE  0x3D4
#define CLR_BUF   0xB8000

/*-----------------------------------------------------------------------------
 * updates cursor position */
//-----------------------------------------------------------------------------
static void cursor (int pos) {
    outb (CLR_BASE, 14);
    outb (CLR_BASE+1, pos >> 8);
    outb (CLR_BASE, 15);
    outb (CLR_BASE+1, pos & 0xFF);
}

static volatile unsigned char *pos = 0; // current position within VGA buffer

/*-----------------------------------------------------------------------------
 * Clears the console by printning lots of blank lines (XXX: inefficient) */
//-----------------------------------------------------------------------------
void clear_console (void) {
    for (int i = 0; i < ROW; i++)
        kputc_clr ('\n', TXT_CLR);
    cursor (0);
    pos = 0;
}

/*-----------------------------------------------------------------------------
 * Writes a character to the console */
//-----------------------------------------------------------------------------
static void kputc_clr (unsigned char c, unsigned char attr) {
    static unsigned char *base = 0;

    if (!pos) {
        // TODO: probe for colour/monochrome dispay; assume colour for now
        pos = (volatile unsigned char*) CLR_BUF;
        base = (unsigned char*) CLR_BUF;

        // get cursor position
        uint32_t cpos;
        outb (CLR_BASE, 14);
        cpos = inb (CLR_BASE+1) << 8;
        outb (CLR_BASE, 15);
        cpos |= inb (CLR_BASE+1);
        if (cpos <= COL * ROW)
            pos = base + cpos*2;
        else
            pos = base;
    }

    
    switch (c) {
        case '\n':
            pos += COL * 2; //        |
        case '\r':          // <------'
            pos -= ((uint32_t) pos - CLR_BUF) % (COL * 2);
            break;
        case '\t':
            kputs ("        ");
            break;
        case '\b':
            pos -= 2;
            if ((void*) pos < (void*) CLR_BUF)
                pos = (void*) CLR_BUF;
            *pos = ' ';
            break;
        default:
            *pos++ = c;
            *pos++ = attr & 0xF;
            break;
    }

    // scroll down
    if (pos >= base + (COL*ROW*2)) {
        memcpy (base, base + (COL*2), COL*(ROW-1)*2);
        for (uint8_t *i = base + (COL*(ROW-1)*2); i < base + (COL*ROW*2);
                i += 2)
            i[0] = ' ';
        pos -= COL*2;
    }
    cursor ((pos-base)/2);
}
