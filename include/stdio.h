/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include <stddef.h>
#include <sys/type_defs.h>

#ifndef EOF
#define EOF _EOF_DEFN
#endif

#ifndef _FPOS_T_DEFINED
#define _FPOS_T_DEFINED
typedef _FPOS_T_TYPE fpos_t;
#endif
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef _OFF_T_TYPE off_t;
#endif
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef _ssize_t ssize_t;
#endif

typedef struct {
	int fd;
	unsigned char buf;
	size_t buffered;
	int error;
	_Bool eof;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

static inline void clearerr(FILE *stream)
{
	stream->error = 0;
	stream->eof = 0;
}

static inline int feof(FILE *stream)
{
	return stream->eof;
}

static inline int ferror(FILE *stream)
{
	return stream->error;
}

static inline int fflush(FILE *stream)
{
	return 0;
}

static inline int fileno(FILE *stream)
{
	return stream->fd;
}

int fclose(FILE *stream);
int fgetc(FILE *stream);
int fprintf(FILE *stream, const char *fmt, ...);
int fputc(int c, FILE *stream);
int fputs(const char *restrict s, FILE *restrict stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
#define getc fgetc
int getchar(void);
int printf(const char *fmt, ...);
#define putc fputc
int putchar(int c);
int puts(const char *s);
int rename(const char *old, const char *new);
int snprintf(char *str, size_t size, const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int ungetc(int c, FILE *stream);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int vprintf(const char *fmt, va_list ap);
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int vsprintf(char *str, const char *fmt, va_list ap);

#endif
