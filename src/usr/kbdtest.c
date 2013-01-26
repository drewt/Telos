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

#include <stddef.h>
#include <telos/io.h>
#include <telos/print.h>
#include <telos/process.h>
#include <telos/devices.h>

void kbd_test (void *arg) {
    int fd;
    char in[3];
    in[2] = '\0';

    fd = open (DEV_KBD);
    printf ("Enter 'asdf': ");
    read (fd, &in, 2);
    printf ("%s", in);
    syssleep (10000);
    read (fd, &in, 2);
    printf ("%s", in);
    puts (" ?= asdf");
    close (fd);
}
