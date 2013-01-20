/* string.c : string.h implementation
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

#include <string.h>

char *strcat (char *dest, const char *src) {
    char *begin = dest;
    for (; *dest != '\0'; dest++);
    for (; *src != '\0'; dest++, src++)
        *dest = *src;
    *dest = '\0';
    return begin;
}

char *strncat (char *dest, const char *src, size_t n) {
    char *begin = dest;

    for (; *dest != '\0'; dest++);
    for (size_t i = 0; i < n && *src != '\0'; i++, dest++, src++)
        *dest = *src;
    *dest = '\0';
    return begin;
}

char *strchr (const char *s, int c) {
    for (; *s != '\0'; s++) {
        if (*s == c)
            return s;
    }
    return NULL;
}

char *strrchr (const char *s, int c) {
    const char *rc = NULL;
    for (; *s != '\0'; s++) {
        if (*s == c)
            rc = s;
    }
    return rc;
}

char *strchrnul (const char *s, int c) {
    for (; *s != '\0' && *s != c; s++);
    return s;
}

int strcmp (const char *s1, const char *s2) {
    for(;;s1++, s2++) {
        if (*s1 != *s2)
            return *s1 - *s2;
        if (*s1 == '\0')
            return 0;
    }
}

int strncmp (const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n && *s1 != '\0'; i++, s1++, s2++) {
        if (*s1 != *s2)
            return *s1 - *s2;
    }
    return 0;
}

char *strcpy (char *dest, const char *src) {
    size_t i;
    for (i = 0; src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';
    return dest;
}

char *strncpy (char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}

size_t strlen (const char *s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

char *strpbrk (const char *s, const char *accept) {
    char *c;
    for (; s != '\0'; s++) {
        if ((c = strchr (accept, *s)))
            return s;
    }
    return NULL;
}

size_t strspn (const char *s, const char *accept) {
    size_t n;
    char *c;
    for (n = 0; *s != '\0'; n++, s++) {
        if (!(c = strchr (accept, *s)))
            break;
    }
    return n;
}

size_t strcspn (const char *s, const char *reject) {
    size_t n;
    char *c;
    for (n = 0; *s != '\0'; n++, s++) {
        if ((c = strchr (reject, *s)))
            break;
    }
    return n;
}

char *strtok (char *str, const char *delim) {
    char junk;
    return telos_strtok (str, delim, &junk);
}

char *telos_strtok (char *str, const char *delim, char *d) {
    static char *tokp;
    char *tmp;
    int i;

    if (str) tokp = str;
    if (*tokp == '\0') return NULL;

    for (i = 0; tokp[i] != '\0'; i++) {
        if ((strchr (delim, tokp[i]))) {
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
