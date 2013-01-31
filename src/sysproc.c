/* sysproc.c : system processes
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

#include <stdio.h>
#include <signal.h>

#include <telos/process.h>

void tsh (int argc, char **argv);

void idle_proc () {
    for(;;);
}

static void sigchld_handler(int signo) {}

void root_proc () {

    int sig;

    signal (SIGCHLD, sigchld_handler);
    
    syscreate (tsh, 0, NULL);
    for (sig = sigwait (); sig != SIGCHLD; sig = sigwait ());
    printf ("Goodbye!");

    for(;;);
}
