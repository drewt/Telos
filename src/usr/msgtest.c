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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <telos/msg.h>
#include <telos/process.h>

pid_t main_pid;

void recv_proc (int argc, char *argv[]) {
    char msg[40];

    if (recv (&main_pid, msg, 40) == -1) {
        puts ("recv error: returned -1");
        return;
    }
    if (strcmp (msg, argv[0])) {
        puts ("recv error: msg != arg");
        return;
    }
}

void send_proc () {
    char msg[40] = "msg";
    char reply[40];

    if (send (main_pid, msg, 4, reply, 40) == -1) {
        puts ("send error: returned -1");
        return;
    }
    if (strcmp (msg, reply)) {
        puts ("reply error: msg != reply");
        return;
    }
}

void msg_test (void *arg) {
    pid_t pids[10];
    char buf[40];
    char *msg = "msg";

    main_pid = getpid ();

    puts ("Testing block-on-recv...");
    for (int i = 0; i < 10; i++)
        pids[i] = syscreate (recv_proc, 1, &msg);
    sleep (1);
    for (int i = 0; i < 10; i++)
        send (pids[i], msg, 4, NULL, 0);

    puts ("Testing block-on-send...");
    for (int i = 0; i < 10; i++)
        pids[i] = syscreate (send_proc, 0, NULL);
    sleep (1);
    for (int i = 0; i < 10; i++) {
        if (recv (&pids[i], buf, 40) == -1) {
            puts ("recv error: returned -1");
            return;
        }
        if (strcmp (buf, msg)) {
            puts ("recv error: buf != msg");
            return;
        }
        buf[0] = '\0';
    }

    puts ("Testing block-on-reply...");
    for (int i = 0; i < 10; i++) {
        if (reply (pids[i], msg, 4) == -1) {
            puts ("reply error: returned -1");
            return;
        }
    }
}
