/*  Copyright 2013 Drew Thoreson
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

#include <stddef.h>
#include <stdarg.h>
#include <sys/type_macros.h>

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
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef _SIZE_T_TYPE size_t;
#endif
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef _SSIZE_T_TYPE ssize_t;
#endif

typedef struct {
	int fd;
	unsigned char buf;
	size_t buffered;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int snprintf(char *str, size_t size, const char *fmt, ...);
int vsprintf(char *str, const char *fmt, va_list ap);
int sprintf(char *str, const char *fmt, ...);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int fprintf(FILE *stream, const char *fmt, ...);
int vfprintf(FILE *stream, const char *fmt, va_list ap);
int puts(const char *s);
int putchar(int c);
int putc(int c, FILE *stream);

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

int getc(FILE *stream);
int ungetc(int c, FILE *stream);
int getchar(void);
char *gets(char *s, int size);

int rename(const char *old, const char *new);

#endif
