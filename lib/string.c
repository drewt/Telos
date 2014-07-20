/*  Copyright 2013 Drew Thoreson.
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

#include <stddef.h>
#include <string.h>

int atoi(const char *nptr)
{
	int n;
	for (n = 0; *nptr != '\0'; nptr++) {
		n *= 10;
		n += *nptr - '0';
	}
	return n;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const char *c1 = (const char*)s1;
	const char *c2 = (const char*)s2;

	for (size_t i = 0; i < n; i++)
		if (c1[i] != c2[i])
			return c1[i] - c2[i];
	return 0;
}

/*-----------------------------------------------------------------------------
 * Copies len bytes from src to dst */
//-----------------------------------------------------------------------------
void memcpy(void *restrict dest, const void *restrict src, size_t n)
{
	const char *s = src;
	char *d = dest;
	for (size_t i = 0; i < n; i++)
		d[i] = s[i];
}

/*-----------------------------------------------------------------------------
 * Fills the first n bytes of the memory area pointed to by s with the constant
 * byte c */
//-----------------------------------------------------------------------------
void *memset(void *s, char c, size_t n)
{
	char *d = s;
	for (size_t i = 0; i < n; i++)
		d[i] = c;
	return s;
}

/*-----------------------------------------------------------------------------
 * Scans the initial n bytes of the memory area pointed to by s for the first
 * instance of c */
//-----------------------------------------------------------------------------
void *memchr(const void *s, int c, size_t n)
{
	unsigned const char *p = s;
	for (size_t i = 0; i < n; i++) {
		if ( p[i] == c )
			return (void*) p+i;
	}
	return NULL;
}

/*-----------------------------------------------------------------------------
 * Like memchr, except searches backward from the end of the n bytes pointed to
 * by s instead of forward from the beginning */
//-----------------------------------------------------------------------------
void *memrchr(const void *s, int c, size_t n)
{
	unsigned const char *p = s;
	for (size_t i = n-1; i > 0; i--) {
		if ( p[i] == c )
			return (void*) p+i;
	}
	return NULL;
}

/*-----------------------------------------------------------------------------
 * Similar to memchr, except assumes that an instance of c lies somewhere in
 * the memory area starting at the location pointed to by s */
//-----------------------------------------------------------------------------
void *rawmemchr(const void *s, int c)
{
	unsigned const char *p = s;
	while ( 1 ) {
		if ( *p == c )
			return (void*) p;
		p++;
	}
}

/*-----------------------------------------------------------------------------
 * Appends the src string to the dest string, overwriting the terminating null
 * byte ('\0') at the end of src, then adds a terminating null byte.  The
 * strings may not overlap, and the dest string must have enough space for the
 * result */
//-----------------------------------------------------------------------------
char *strcat(char *restrict dest, const char *restrict src)
{
	char *begin = dest;
	for (; *dest != '\0'; dest++);
	for (; *src != '\0'; dest++, src++)
		*dest = *src;
	*dest = '\0';
	return begin;
}

/*-----------------------------------------------------------------------------
 * Similar to strcat, except that this function will use at most n bytes from
 * src, and src does not need to be null-terminated if it contains n or more
 * bytes */
//-----------------------------------------------------------------------------
char *strncat(char *restrict dest, const char *restrict src, size_t n)
{
	char *begin = dest;

	for (; *dest != '\0'; dest++);
	for (size_t i = 0; i < n && *src != '\0'; i++, dest++, src++)
		*dest = *src;
	*dest = '\0';
	return begin;
}

/*-----------------------------------------------------------------------------
 * Returns a pointer to the first occurrence of the character c in the string
 * s, or NULL if the character is not found */
//-----------------------------------------------------------------------------
char *strchr(const char *s, int c)
{
	for (; *s != '\0'; s++) {
		if (*s == c)
			return (char*) s;
	}
	return NULL;
}

/*-----------------------------------------------------------------------------
 * Returns a pointer to the last occurrence of the character c in the string s,
 * or NULL if the character is not found */
//-----------------------------------------------------------------------------
char *strrchr(const char *s, int c)
{
	const char *rc = NULL;
	for (; *s != '\0'; s++) {
		if (*s == c)
			rc = s;
	}
	return (char*) rc;
}

/*-----------------------------------------------------------------------------
 * Similar to strchr, except that if c is not found in s, then this function
 * returns a pointer to the null byte at the end of s, rather than NULL */
//-----------------------------------------------------------------------------
char *strchrnul(const char *s, int c)
{
	for (; *s != '\0' && *s != c; s++);
	return (char*) s;
}

/*-----------------------------------------------------------------------------
 * Compares the two strings s1 and s2, returning an integer less than, equal
 * to, or greater than zero if s1 is found, respectively, to be less than, to
 * match, or be greater than s2 */
//-----------------------------------------------------------------------------
int strcmp(const char *s1, const char *s2)
{
	for(;;s1++, s2++) {
		if (*s1 != *s2)
			return *s1 - *s2;
		if (*s1 == '\0')
			return 0;
	}
}

/*-----------------------------------------------------------------------------
 * Similar to strcmp, except that only the first (at most) n bytes of s1 and s2
 * are compared */
//-----------------------------------------------------------------------------
int strncmp(const char *s1, const char *s2, size_t n)
{
	for (size_t i = 0; i < n && *s1 != '\0'; i++, s1++, s2++) {
		if (*s1 != *s2)
			return *s1 - *s2;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * Copies the string pointed to by src, including the terminating null byte
 * ('\0'), to the buffer pointed to by dest.  The strings may not overlap, and
 * the destination string dest must be large enough to receive the copy */
//-----------------------------------------------------------------------------
char *strcpy(char *restrict dest, const char *restrict src)
{
	size_t i;
	for (i = 0; src[i] != '\0'; i++)
		dest[i] = src[i];
	dest[i] = '\0';
	return dest;
}

/*-----------------------------------------------------------------------------
 * Similar to strcpy, except that at most n bytes of src are copied.  If the
 * length of src is less than n, strncpy() writes additional null bytes to dest
 * to ensure that a total of n bytes are written */
//-----------------------------------------------------------------------------
char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
	size_t i;
	for (i = 0; i < n && src[i] != '\0'; i++) {
		dest[i] = src[i];
	}
	for (; i < n; i++)
		dest[i] = '\0';
	return dest;
}

/*-----------------------------------------------------------------------------
 * Calculates the length of the string s, excluding the terminating null byte
 * ('\0') */
//-----------------------------------------------------------------------------
size_t strlen(const char *s)
{
	size_t i;
	for (i = 0; s[i] != '\0'; i++);
	return i;
}

size_t strnlen(const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] != '\0' && i < n; i++);
	return i;
}

/*-----------------------------------------------------------------------------
 * Locates the first occurrence in the string s of any of the bytes in the
 * string accept */
//-----------------------------------------------------------------------------
char *strpbrk(const char *s, const char *accept)
{
	char *c;
	for (; s != '\0'; s++) {
		if ((c = strchr(accept, *s)))
			return (char*) s;
	}
	return NULL;
}

/*-----------------------------------------------------------------------------
 * Calculates the length of the initial segment of s which consists entirely of
 * bytes in accept */
//-----------------------------------------------------------------------------
size_t strspn(const char *s, const char *accept)
{
	size_t n;
	char *c;
	for (n = 0; *s != '\0'; n++, s++) {
		if (!(c = strchr(accept, *s)))
			break;
	}
	return n;
}

/*-----------------------------------------------------------------------------
 * Calculates the length of the initial segment of s which consists entirely of
 * bytes not in reject */
//-----------------------------------------------------------------------------
size_t strcspn(const char *s, const char *reject)
{
	size_t n;
	char *c;
	for (n = 0; *s != '\0'; n++, s++) {
		if ((c = strchr(reject, *s)))
			break;
	}
	return n;
}

/*-----------------------------------------------------------------------------
 * Parses str into a sequence of tokens, delimited by the bytes in delim */
//-----------------------------------------------------------------------------
/*char *strtok(char *restrict str, const char *restrict delim)
{
	char junk;
	return telos_strtok(str, delim, &junk);
}*/

/*-----------------------------------------------------------------------------
 * Like strtok, except that *d is set to the delimiter for the current token
 * when this function returns */
//-----------------------------------------------------------------------------
char *telos_strtok(char *restrict str, const char *restrict delim, char *d)
{
	static char *tokp;
	char *tmp;
	int i;

	if (str) tokp = str;
	if (*tokp == '\0') return NULL;

	for (i = 0; tokp[i] != '\0'; i++) {
		if ((strchr(delim, tokp[i]))) {
			*d = tokp[i];
			tmp = tokp;
			tokp[i] = '\0';
			tokp = &tokp[i+1];
			return tmp;
		}
	}
	*d = '\0';
	tmp = tokp;
	tokp = &tokp[i];
	return tmp;
}

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

char *strtok(char *restrict s, const char *restrict delim)
{
	static char *last;

	return strtok_r(s, delim, &last);
}

char *strtok_r(char *restrict s, const char *restrict delim,
		char **restrict last)
{
	char *spanp;
	int c, sc;
	char *tok;

	if (s == NULL && (s = *last) == NULL)
		return (NULL);

	/*
	 * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
	 */
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
		if (c == sc)
			goto cont;
	}

	if (c == 0) {       /* no non-delimiter characters */
		*last = NULL;
		return (NULL);
	}
	tok = s - 1;

	/*
	 * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
	 * Note that delim must have one NUL; we stop if we see that, too.
	 */
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*last = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
