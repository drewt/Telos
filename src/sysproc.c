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

#include <telos/process.h>
#include <telos/print.h>
#include <signal.h>

void idle_proc (void *arg) {
    for(;;);
}

static void void_handler (int signo) {}

void root (void *arg) {
    int sig;
    void(*f)(void*) = (void(*)(void*)) arg;

    signal (SIGCHLD, void_handler);

    for (;;) {
        syscreate (f, NULL);
        for (sig = 0; sig != SIGCHLD; sig = sigwait ());
    }
}
