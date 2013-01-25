/* print.c : console I/O
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C Library.
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
#include <stddef.h>
#include <string.h>
#include <telos/io.h>
#include <telos/filedes.h>
#include <telos/print.h>

#include <klib.h>

int printf (const char *fmt, ...) {
    int rv;
    va_list ap;
    va_start (ap, fmt);
    rv = vprintf (fmt, ap);
    va_end (ap);
    return rv;
}

int vprintf (const char *fmt, va_list ap) {
    char *s;
    char buf[33];
    int rv = 0;
    const char *pos = fmt;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            rv += write (STDOUT_FILENO, pos, &fmt[i] - pos);

            switch (fmt[++i]) {
            case '%':
                rv += write (STDOUT_FILENO, "%", 1);
                break;
            case 's':
                s = va_arg (ap, char*);
                rv += write (STDOUT_FILENO, s, strlen (s));
                break;
            case 'd':
            case 'i':
                itoa (va_arg (ap, int), buf, 10);
                rv += write (STDOUT_FILENO, buf, 33);
                break;
            default:
                return -1;
            }
            pos = &fmt[i+1];
        }
    }
    rv += write (STDOUT_FILENO, pos, strlen (pos));
    return rv;
}

int puts (const char *s) {
    int rv;
    while ((rv = write (STDOUT_FILENO, s, 200)) != 0)
        s += rv;
    write (STDOUT_FILENO, "\n", 1);
    return 1;
}
