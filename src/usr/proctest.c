/* proctest.c : process control tests
 */

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

#include <telos/process.h>
#include <telos/print.h>

static void print_proc (void *str) {
    printf ("%s", (char*) str);
}

static void stop_proc () {
    sysstop ();
    printf ("FAIL");
}

void proc_test (void *arg) {
    printf ("Testing syscreate... asdf ?= ");
    syscreate (print_proc, "a");
    syssleep (50);
    syscreate (print_proc, "s");
    syssleep (50);
    syscreate (print_proc, "d");
    syssleep (50);
    syscreate (print_proc, "f");
    syssleep (50);
    puts ("");

    printf ("Testing sysstop...");
    syscreate (stop_proc, NULL);
    syssleep (50);
    puts ("");
}
