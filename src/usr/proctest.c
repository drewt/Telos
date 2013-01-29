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
#include <unistd.h>

#include <telos/process.h>
#include <telos/print.h>

static void print_proc (int argc, char *argv[]) {
    printf ("%s", (char*) argv[0]);
}

static void stop_proc () {
    sysstop ();
    printf ("FAIL");
}

void proc_test (void *arg) {
    char *a = "a", *s = "s", *d = "d", *f = "f";
    printf ("Testing syscreate... asdf ?= ");
    syscreate (print_proc, 1, &a);
    sleep (1);
    syscreate (print_proc, 1, &s);
    sleep (1);
    syscreate (print_proc, 1, &d);
    sleep (1);
    syscreate (print_proc, 1, &f);
    sleep (1);
    puts ("");

    printf ("Testing sysstop...");
    syscreate (stop_proc, 0, NULL);
    sleep (1);
    puts ("");
}
