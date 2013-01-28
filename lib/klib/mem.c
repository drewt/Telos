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
#include <string.h>

#include <telos/mem.h>

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *malloc (size_t size) {
    void *rp;
    if (telos_malloc (size, &rp))
        return NULL;
    return rp;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *calloc (size_t nmemb, size_t size) {
    void *rp;
    if (telos_malloc (nmemb * size, &rp))
        return NULL;
    memset (rp, 0, nmemb * size);
    return rp;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *realloc (void *ptr, size_t size) {
    void *rp;
    if (telos_malloc (size, &rp))
        return NULL;
    memcpy (rp, ptr, size);
    free (ptr);
    return rp;
}


