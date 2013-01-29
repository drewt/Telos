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
#include <unistd.h>
#include <string.h>

#include <telos/msg.h>
#include <telos/process.h>
#include <unistd.h>

#define BUF_LEN 100

static pid_t server_pid = -1;

void printserver (int argc, char *argv[]) {
    int rv;
    pid_t pid;
    char buf[BUF_LEN];

    server_pid = getpid ();

    for (;;) {
        pid = 0;
        if ((rv = recv (&pid, buf, BUF_LEN)) == -1) {
            puts ("printserver: recv error");
            return;
        }
        if (write (STDOUT_FILENO, buf, rv) == -1) {
            puts ("printserver: write error");
            return;
        }
        if (reply (pid, NULL, 0) == -1) {
            puts ("printserver: reply error");
            return;
        }
    }
}

void printclient (int argc, char *argv[]) {
    char reply_blk;

    if (argc < 1) {
        puts ("usage: printclient [string]");
        return;
    }
    for (int i = 0; argv[i] != NULL; i++) {
        if (send (server_pid, argv[i], strlen (argv[i])+1, &reply_blk, 1) == -1)
            puts ("printclient: send error");
        else
            printf (" ");
    }
    puts ("");
}
