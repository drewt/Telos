/* string.h : standard C string functions
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

#ifndef __STRING_H_
#define __STRING_H_

#include <stddef.h>

void memcpy (void *restrict dest, const void *restrict src, size_t n);
void *memset (void *s, char c, size_t n);
void *memchr (const void *s, int c, size_t n);
void *memrchr (const void *s, int c, size_t n);
void *rawmemchr (const void *s, int c);
char *strcat (char *restrict dest, const char *restrict src);
char *strncat (char *restrict dest, const char *restrict src, size_t n);
char *strchr (const char *s, int c);
char *strrchr (const char *s, int c);
char *strchrnul (const char *s, int c);
int strcmp (const char *s1, const char *s2);
int strncmp (const char *s1, const char *s2, size_t n);
char *strcpy (char *restrict dest, const char *restrict src);
char *strncpy (char *dest, const char *src, size_t n);
size_t strlen (const char *s);
char *strpbrk (const char *s, const char *accept);
size_t strspn (const char *s, const char *accept);
size_t strcspn (const char *s, const char *reject);
char *strtok (char *restrict str, const char *restrict delim);
char *telos_strtok (char *restrict str, const char *restrict delim, char *d);

#endif // __STRING_H_
