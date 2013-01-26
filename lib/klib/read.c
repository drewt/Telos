/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C library.
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

#include <stddef.h>
#include <telos/io.h>
#include <telos/filedes.h>

int getchar (void) {
    unsigned char c;
    if (!read (STDIN_FILENO, &c, 1))
        return EOF;
    return (int) c;
}

char *gets (char *s, int size) {
    int rv;

    rv = read (STDIN_FILENO, s, size-1);
    if (rv <= 0)
        return NULL;
    s[rv] = '\0';
    return s;
}
