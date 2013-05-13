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
#include <stdio.h>
#include <unistd.h>

#include <klib.h>

#define WRITE_SIZE 200

int vsnprintf (char *str, size_t size, const char *fmt, va_list ap)
{
    size_t count;
    char *s;
    char buf[33];
    const char *rpos = fmt;

    count = 0;
    while (count < size && *rpos != '\0') {
        if (*rpos == '%') {
            rpos++;
            switch (*rpos++) {
            case '%':
                str[count++] = '%';
                break;
            case 'c':
                str[count++] = va_arg (ap, int);
                break;
            case 's':
                s = va_arg (ap, char*);
                while (count < size && *s != '\0')
                    str[count++] = *s++;
                break;
            case 'd':
            case 'i':
                itoa (va_arg (ap, int), buf, 10);
                for (int i = 0; count < size && buf[i] != '\0'; i++)
                    str[count++] = buf[i];
                break;
            case 'x':
                itoa (va_arg (ap, int), buf, 16);
                for (int i = 0; count < size && buf[i] != '\0'; i++)
                    str[count++] = buf[i];
                break;
            default:
                break;
            }
        } else {
            str[count++] = *rpos++;
        }
    }
    if (count < size)
        str[count++] = '\0';
    return count;
}

int snprintf (char *str, size_t size, const char *fmt, ...)
{
    int rv;
    va_list ap;
    va_start (ap, fmt);
    rv = vsnprintf (str, size, fmt, ap);
    va_end (ap);
    return rv;
}

int vsprintf (char *str, const char *fmt, va_list ap)
{
    char *s;
    char buf[33];
    char *wpos = str;
    const char *rpos = fmt;

    while (*rpos != '\0') {
        if (*rpos == '%') {
            rpos++;
            switch (*rpos++) {
            case '%':
                *wpos++ = '%';
                break;
            case 'c':
                *wpos++ = va_arg (ap, int);
                break;
            case 's':
                s = va_arg (ap, char*);
                while (*s != '\0')
                    *wpos++ = *s++;
                break;
            case 'd':
            case 'i':
                itoa (va_arg (ap, int), buf, 10);
                for (int i = 0; buf[i] != '\0'; i++)
                    *wpos++ = buf[i];
                break;
            default:
                break;
            }
        } else {
            *wpos++ = *rpos++;
        }
    }
    *wpos++ = '\0';
    return wpos - str;
}

int sprintf (char *str, const char *fmt, ...)
{
    int rv;
    va_list ap;
    va_start (ap, fmt);
    rv = vsprintf (str, fmt, ap);
    va_end (ap);
    return rv;
}

static int fmt_print (int fd, const char *fmt, va_list *ap, int *count)
{
    char c;
    char *s;
    char buf[33];
    int rv = 0, tmp;

    for (int i = 0; fmt[i] != '\0'; i++) {
        switch (fmt[i]) {
        case '%':
            write (fd, "%", 1);
            (*count)++;
            return 1;
        case 'c':
            c = va_arg (*ap, int);
            write (fd, &c, 1);
            (*count)++;
            return 1;
        case 'd':
        case 'i':
            itoa (va_arg (*ap, int), buf, 10);
            rv = write (fd, buf, 33);
            (*count)++;
            return rv;
        case 'x':
            itoa (va_arg (*ap, int), buf, 16);
            rv = write (fd, buf, 33);
            (*count)++;
            return rv;
        case 's':
            s = va_arg (*ap, char*);
            while ((tmp = write (fd, s, WRITE_SIZE)) != 0) {
                s += tmp;
                rv += tmp;
            }
            (*count)++;
            return rv;
        case '.':
        case '*':
            (*count)++;
            break; // TODO
        default:
            return rv;
        }
    }
    return rv;
}

int printf (const char *fmt, ...)
{
    int rv;
    va_list ap;
    va_start (ap, fmt);
    rv = vfprintf (stdout, fmt, ap);
    va_end (ap);
    return rv;
}

int vprintf (const char *fmt, va_list ap)
{
    return vfprintf (stdout, fmt, ap);
}

int fprintf (FILE *stream, const char *fmt, ...)
{
    int rv;
    va_list ap;
    va_start (ap, fmt);
    rv = vfprintf (stream, fmt, ap);
    va_end (ap);
    return rv;
}

int vfprintf (FILE *stream, const char *fmt, va_list ap)
{
    int rv = 0, tmp;
    const char *pos = fmt;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            rv += write (stream->fd, pos, &fmt[i] - pos);
            rv += fmt_print (stream->fd, &fmt[i+1], &ap, &i);
            pos = &fmt[i+1];
        }
    }
    while ((tmp = write (stream->fd, pos, WRITE_SIZE)) != 0) {
        rv += tmp;
        pos += tmp;
    }
    return rv;
}

int puts (const char *s)
{
    int rv;
    while ((rv = write (STDOUT_FILENO, s, WRITE_SIZE)) != 0)
        s += rv;
    write (STDOUT_FILENO, "\n", 1);
    return 1;
}

int putchar(int c)
{
    if (write (STDOUT_FILENO, &c, 1) == -1)
        return EOF;
    return c;
}
