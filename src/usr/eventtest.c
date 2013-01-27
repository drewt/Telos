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

#include <telos/process.h>
#include <telos/print.h>
#include <signal.h>

static void sleep_proc (int argc, char *argv[]) {
    syssleep (argc);
    printf ("%d ", argc);
}

static void sleep_test (void) {
    printf ("Testing sleep...\nVerify ascending: ");
    syscreate (sleep_proc, 100, NULL);
    syscreate (sleep_proc, 990, NULL);
    syscreate (sleep_proc, 1440, NULL);
    syssleep (2000);

    printf ("\nVefify ascending: ");
    syscreate (sleep_proc, 1440, NULL);
    syscreate (sleep_proc, 990, NULL);
    syscreate (sleep_proc, 100, NULL);
    syssleep (2000);
    puts ("");
}

static void alarm_handler (int signo) {
    puts ("rm");
}

static void alarm_test (void) {
    int sig;
    signal (SIGALRM, alarm_handler);
    printf ("Testing alarm... alrm ?= ");
    alarm (2);
    printf ("al");
    for (sig = sigwait (); sig != SIGALRM; sig = sigwait ());
}

void event_test (int argc, char *argv[]) {
    sleep_test ();
    alarm_test ();
}
