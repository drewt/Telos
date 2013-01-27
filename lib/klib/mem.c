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

#include <stdint.h>
#include <stddef.h>

#include <telos/mem.h>
#include <klib.h>

void *malloc (size_t size) {
    void *r;
    telos_malloc (size, &r);
    return r;
}

/*-----------------------------------------------------------------------------
 * Copies len bytes from src to dst */
//-----------------------------------------------------------------------------
void memcpy (void *dest, const void *src, uint32_t n) {
    const char *s = src;
    char *d = dest;
    for ( uint32_t i = 0; i < n; i++ )
        d[i] = s[i];
}

/*-----------------------------------------------------------------------------
 * Fills the first n bytes of the memory area pointed to by s with the constant
 * byte c */
//-----------------------------------------------------------------------------
void *memset (void *s, char c, uint32_t n) {
    char *d = s;
    for ( uint32_t i = 0; i < n; i++ )
        d[i] = c;
    return s;
}

/*-----------------------------------------------------------------------------
 * Scans the initial n bytes of the memory area pointed to by s for the first
 * instance of c */
//-----------------------------------------------------------------------------
void *memchr (const void *s, int c, long n) {
    unsigned const char *p = s;
    for ( long i = 0; i < n; i++ ) {
        if ( p[i] == c )
            return p+i;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
 * Like memchr, except searches backward from the end of the n bytes pointed to
 * by s instead of forward from the beginning */
//-----------------------------------------------------------------------------
void *memrchr (const void *s, int c, long n) {
    unsigned const char *p = s;
    for ( long i = n-1; i > 0; i-- ) {
        if ( p[i] == c )
            return p+i;
    }
    return 0;
}

/*-----------------------------------------------------------------------------
 * Similar to memchr, except assumes that an instance of c lies somewhere in
 * the memory area starting at the location pointed to by s */
//-----------------------------------------------------------------------------
void *rawmemchr (const void *s, int c) {
    unsigned const char *p = s;
    while ( 1 ) {
        if ( *p == c )
            return p;
        p++;
    }
}
