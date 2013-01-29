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
#include <stdio.h>

#include <signal.h>
#include <unistd.h>

#include <telos/process.h>
#include <telos/devices.h>
#include <telos/io.h>

static void read_proc () {
    int fd;
    char in[5];
    
    puts ("Reading: ");
    if ((fd = open ("/dev/kbd", 0)) == -1) {
        puts ("Error opening keyboard");
        return;
    }

    if (read (fd, in, 4) == -1) {
        puts ("Error reading from keyboard");
        return;
    }
    in[4] = '\0';
    puts (in);
}

static void sigchld_handler (int signo) {}

void kbd_test (void *arg) {

    int sig;

    signal (SIGCHLD, sigchld_handler);

    syscreate (read_proc, 0, NULL);
    syscreate (read_proc, 0, NULL);

    for (sig = 0; sig != SIGCHLD; sig = sigwait ());
    for (sig = 0; sig != SIGCHLD; sig = sigwait ());

    printf ("Enter any key to exit: ");
    getchar (); puts ("");
}
