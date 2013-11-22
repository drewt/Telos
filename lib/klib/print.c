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

static int fmt_print(int fd, const char *fmt, va_list *ap, int *count)
{
	char c;
	char *s;
	char buf[33];
	int rv = 0, tmp;

	for (int i = 0; fmt[i] != '\0'; i++) {
		switch (fmt[i]) {
		case '%':
			write(fd, "%", 1);
			(*count)++;
			return 1;
		case 'c':
			c = va_arg(*ap, int);
			write(fd, &c, 1);
			(*count)++;
			return 1;
		case 'd':
		case 'i':
			itoa(va_arg(*ap, int), buf, 10);
			rv = write(fd, buf, 33);
			(*count)++;
			return rv;
		case 'x':
			itoa_16(va_arg(*ap, int), buf);
			rv = write(fd, buf, 33);
			(*count)++;
			return rv;
		case 's':
			s = va_arg(*ap, char*);
			while ((tmp = write(fd, s, WRITE_SIZE)) != 0) {
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

int printf(const char *fmt, ...)
{
	int rv;
	va_list ap;
	va_start(ap, fmt);
	rv = vfprintf(stdout, fmt, ap);
	va_end(ap);
	return rv;
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int fprintf(FILE *stream, const char *fmt, ...)
{
	int rv;
	va_list ap;
	va_start(ap, fmt);
	rv = vfprintf(stream, fmt, ap);
	va_end(ap);
	return rv;
}

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	int rv = 0, tmp;
	const char *pos = fmt;

	for (int i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == '%') {
			rv += write(stream->fd, pos, &fmt[i] - pos);
			rv += fmt_print(stream->fd, &fmt[i+1], &ap, &i);
			pos = &fmt[i+1];
		}
	}
	while ((tmp = write(stream->fd, pos, WRITE_SIZE)) != 0) {
		rv += tmp;
		pos += tmp;
	}
	return rv;
}

int puts(const char *s)
{
	int rv;
	while ((rv = write(STDOUT_FILENO, s, WRITE_SIZE)) != 0)
		s += rv;
	write(STDOUT_FILENO, "\n", 1);
	return 1;
}

int putchar(int c)
{
	if (write(STDOUT_FILENO, &c, 1) == -1)
		return EOF;
	return c;
}
