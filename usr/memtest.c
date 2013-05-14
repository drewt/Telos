/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
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

#include <stdlib.h>
#include <stdio.h>

#define N 25
#define M 100
#define A 144

void memtest (int argc, char *argv[])
{
/*    void *mem[N];

    puts ("Testing malloc()...");
    for (int i = 0; i < N; i++) {
        if ((mem[i] = malloc (i*M+A)) == NULL) {
            puts ("memtest: malloc returned NULL");
            return;
        }
    }

    puts ("Testing free()...");
    for (int i = N-1; i >= 0; i--)
        free (mem[i]);*/
    for (int i = 0; i < N; i++) {
        int x = i*M+A;
        printf ("dec=%d oct=%o hex=%x\n", x, x, x);
    }
}
