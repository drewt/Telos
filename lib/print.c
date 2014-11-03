/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <klib.h>

int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	char buf[1024];
	int ret = vsnprintf(buf, 1024, fmt, ap);
	return write(stream->fd, buf, ret);
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

#define va_printf(stream, fmt) \
	do { \
		int ret; \
		va_list ap; \
		va_start(ap, fmt); \
		ret = vfprintf(stream, fmt, ap); \
		va_end(ap); \
		return ret; \
	} while (0)

int fprintf(FILE *stream, const char *fmt, ...)
{
	va_printf(stream, fmt);
}

int printf(const char *fmt, ...)
{
	va_printf(stdout, fmt);
}

int puts(const char *s)
{
	int rv;
	size_t len = strlen(s);
	while ((rv = write(STDOUT_FILENO, s, len)) != 0) {
		s += rv;
		len -= rv;
	}
	write(STDOUT_FILENO, "\n", 1);
	return 1;
}

int putchar(int c)
{
	if (write(STDOUT_FILENO, &c, 1) == -1)
		return EOF;
	return c;
}

int putc(int c, FILE *stream)
{
	if (write(stream->fd, &c, 1) == -1)
		return EOF;
	return c;
}
