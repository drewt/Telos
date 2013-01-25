/* msgtest.c : message passing tests
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

#include <telos/msg.h>
#include <telos/process.h>
#include <telos/print.h>
#include <string.h>

pid_t main_pid;

void recv_proc (void *arg) {
    char msg[40];

    if (recv (main_pid, msg, 40) == -1) {
        sysputs ("recv error: returned -1\n");
        return;
    }
    if (strcmp (msg, arg)) {
        sysputs ("recv error: msg != arg\n");
        return;
    }
}

void send_proc (void *arg) {
    char msg[40] = "msg";
    char reply[40];

    /*if (send (main_pid, msg, 4, NULL, 0) == -1) {
        sysputs ("send error: returned -1\n");
        return;
    }*/
    if (send (main_pid, msg, 4, reply, 40) == -1) {
        sysputs ("send error: returned -1\n");
        return;
    }
    if (strcmp (msg, reply)) {
        sysputs ("reply error: msg != reply\n");
        return;
    }
}

void msg_test (void *arg) {
    pid_t pids[10];
    char buf[40];
    char *msg = "msg";

    main_pid = getpid ();

    sysputs ("Testing block-on-recv...\n");
    for (int i = 0; i < 10; i++)
        pids[i] = syscreate (recv_proc, msg);
    syssleep (100);
    for (int i = 0; i < 10; i++)
        send (pids[i], msg, 4, NULL, 0);

    sysputs ("Testing block-on-send...\n");
    for (int i = 0; i < 10; i++)
        pids[i] = syscreate (send_proc, NULL);
    syssleep (100);
    for (int i = 0; i < 10; i++) {
        if (recv (pids[i], buf, 40) == -1) {
            sysputs ("recv error: returned -1\n");
            return;
        }
        if (strcmp (buf, msg)) {
            sysputs ("recv error: buf != msg\n");
            return;
        }
        buf[0] = '\0';
    }

    sysputs ("Testing block-on-reply...\n");
    for (int i = 0; i < 10; i++) {
        if (reply (pids[i], msg, 4) == -1) {
            sysputs ("reply error: returned -1\n");
            return;
        }
    }
}
