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

#include <kernel/common.h>
#include <kernel/device.h>
#include <telos/io.h>

#include <telos/process.h>
#include <telos/print.h>
#include <signal.h>

void tsh (void *arg);

void idle_proc (void *arg) {
    for(;;);
}

static void sigchld_handler(int signo) {}

void root (void *arg) {

    /*cfd = open (DEV_CONSOLE_0);
    write (cfd, "Hello, console!", 15);
    close (DEV_CONSOLE_0);*/

    int sig;

    signal (SIGCHLD, sigchld_handler);
    
    syscreate (tsh, NULL);
    for (sig = sigwait (); sig != SIGCHLD; sig = sigwait ());
    kprintf ("Goodbye!");

    for(;;);
}
