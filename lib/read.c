/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C library.
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

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

FILE *stdin  = &((FILE) { .fd = 0, .buffered = 0 });
FILE *stdout = &((FILE) { .fd = 1, .buffered = 0 });
FILE *stderr = &((FILE) { .fd = 2, .buffered = 0 });

int getc(FILE *stream)
{
	unsigned char c;

	if (stream->buffered) {
		stream->buffered = 0;
		return stream->buf;
	}

	if (!read(stream->fd, &c, 1))
		return EOF;
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
	return getc(stdin);
}

char *gets(char *s, int size)
{
	int rv;

	/* TODO: read from buffer */
	rv = read(STDIN_FILENO, s, size-1);
	if (rv <= 0)
		return NULL;
	s[rv] = '\0';
	return s;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int rv = write(stream->fd, ptr, size * nmemb);
	return (rv < 0) ? 0 : rv;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	int rv = read(stream->fd, ptr, size * nmemb);
	return (rv < 0) ? 0 : rv;
}
