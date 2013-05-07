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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <telos/process.h>

static void sleep_proc (int argc, char *argv[]) {
    sleep (argc);
    printf ("%d ", argc);
}

static void sleep_test (void) {
    printf ("Testing sleep...\nVerify ascending: ");
    syscreate (sleep_proc, 1, NULL);
    syscreate (sleep_proc, 2, NULL);
    syscreate (sleep_proc, 3, NULL);
    sleep (4);

    printf ("\nVefify ascending: ");
    syscreate (sleep_proc, 3, NULL);
    syscreate (sleep_proc, 2, NULL);
    syscreate (sleep_proc, 1, NULL);
    sleep (4);
    puts ("");
}

static void alarm_handler (int signo) {
    puts ("rm");
}

static void alarm_test (void) {
    int sig, rv;
    signal (SIGALRM, alarm_handler);
    printf ("Testing alarm... alrm ?= ");
    alarm (2);
    if ((rv = alarm (2)) <= 0)
        printf ("error");
    printf ("al");
    for (sig = sigwait (); sig != SIGALRM; sig = sigwait ());
    printf ("Testing alarm(0)... ");
    alarm (2);
    if (alarm (0) > 0)
        puts ("okay");
    else
        puts ("error");
}

void event_test (int argc, char *argv[]) {
    sleep_test ();
    alarm_test ();
}
