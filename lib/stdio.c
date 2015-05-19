/*  Copyright 2013-2015 Drew Thoreson
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

FILE *stdin  = &((FILE) { .fd = 0, .buffered = 0, .eof = 0, });
FILE *stdout = &((FILE) { .fd = 1, .buffered = 0, .eof = 0, });
FILE *stderr = &((FILE) { .fd = 2, .buffered = 0, .eof = 0, });

int fclose(FILE *stream)
{
	if (close(stream->fd) < 0)
		return EOF;
	return 0;
}

int fgetc(FILE *stream)
{
	ssize_t bytes;
	unsigned char c;

	if (stream->buffered) {
		stream->buffered = 0;
		return stream->buf;
	}

	bytes = read(stream->fd, &c, 1);
	if (!bytes) {
		stream->eof = 1;
		return EOF;
	}
	if (bytes < 0) {
		stream->error = 1;
		return EOF;
	}
	return (int) c;
}

int ungetc(int c, FILE *stream)
{
	stream->buf = (unsigned char) c;
	stream->buffered = 1;
	return c;
}

int getchar(void)
{
	return fgetc(stdin);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int rv = write(stream->fd, ptr, size * nmemb);
	return (rv < 0) ? 0 : rv;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	unsigned char *dst = ptr;
	for (size_t i = 0; i < size * nmemb; i++) {
		int c = fgetc(stream);
		if (c == EOF)
			return i;
		dst[i] = c;
	}
	return size * nmemb;
}

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

int fputs(const char *restrict s, FILE *restrict stream)
{
	int rv;
	size_t len = strlen(s);
	while ((rv = write(stream->fd, s, len)) > 0) {
		s += rv;
		len -= rv;
	}
	if (rv < 0) {
		stream->error = 1;
		return EOF;
	}
	return 1;
}

int puts(const char *s)
{
	if (fputs(s, stdout) == EOF)
		return EOF;
	if (write(stdout->fd, "\n", 1) < 0) {
		stdout->error = 1;
		return EOF;
	}
	return 1;
}

int fputc(int c, FILE *stream)
{
	if (write(stream->fd, &c, 1) < 0) {
		stream->error = 1;
		return EOF;
	}
	return c;
}

int putchar(int c)
{
	return fputc(c, stdout);
}
