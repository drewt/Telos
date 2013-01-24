/* tsh.c : Telos shell
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
#include <mem.h>

#include <string.h>
#include <signal.h>

#include <telos/print.h>
#include <telos/process.h>
#include <telos/io.h>

/* user programs */
#include <usr/test.h>

#define TSH_N_CMDS  9
#define TSH_IN_SIZE 512
#define TSH_EXIT ((void*) -1)

#define TSH_PROMPT "TELOS> "

typedef void(*funcptr)(void*);

void tsh (void *arg);
static void hello_tsh (void *arg);
static void help (void *arg);

struct tprog {
    void (*func)(void*);
    char *name;
};

/* program table */
struct tprog tsh_progs[TSH_N_CMDS] =
{
    { TSH_EXIT,   "exit"      },
    { help,       "help"      },
    { hello_tsh,  "hello"     },
    { proc_test,  "proctest"  },
    { sig_test,   "sigtest"   },
    { kbd_test,   "kbdtest"   },
    { str_test,   "strtest"   },
    { event_test, "eventtest" },
    { tsh,        "tsh"       }
};

static funcptr tsh_lookup (char *in) {
    int i;
    for (i = 0; i < TSH_N_CMDS; i++) {
        if (!strcmp (in, tsh_progs[i].name))
            break;
    }
    return (i == TSH_N_CMDS) ? NULL : tsh_progs[i].func;
}

int process_input (char *in, int in_len) {
    int wpos, rpos;

    bool inws = false;
    for (wpos = 0, rpos = 0; rpos < in_len; rpos++) {
        switch (in[rpos]) {
        case '\b':
            if (wpos) wpos--;
            break;
        case ' ':
        case '\t':
            if (!inws) {
                inws = true;
                in[wpos++] = ' ';
            }
            break;
        case '\0':
            rpos = in_len;
            break;
        case '&':
        case '|':
        case '$':
        default:
            in[wpos++] = in[rpos];
            inws = false;
            break;
        }
    }
    in[wpos] = '\0';
    return wpos;
}

void sigchld_handler (int signo) {}

void tsh (void *arg) {
    int in_len, kbd_fd, sig;
    char in[TSH_IN_SIZE+1];
    char *cmd, *op;
    void (*p)(void*);
    bool bg;

    signal (SIGCHLD, sigchld_handler);

    // loop until exit command or error
    for (;;) {
        if ((kbd_fd = open (DEV_KBD_ECHO)) == -1) {
            sysputs ("tsh: error opening keyboard device\n");
            return;
        }

        bg = false;
        for (;;) {
            sysputs (TSH_PROMPT);
            if ((in_len = read (kbd_fd, in, TSH_IN_SIZE)) == -1) {
                sysputs ("tsh: error reading keyboard device\n");
                p = TSH_EXIT;
                break;
            }
            if (!in_len) {
                sysputs ("\n");
                p = TSH_EXIT;
                break;
            }
            if (in_len < 2)
                continue;

            in[in_len-1] = '\0';
            in_len = process_input (in, in_len);
            cmd = strtok (in, " ");
            op  = strtok (NULL, " ");
            if (*op == '&') bg = true;
            if (!(p = tsh_lookup (cmd))) {
                sysputs ("tsh: '");
                sysputs (in);
                sysputs ("' not found\n");
            } else {
                break;
            }
        }
        if (close (kbd_fd)) {
            sysputs ("tsh: error closing keyboard device\n");
            return;
        }
        if (p == TSH_EXIT)
            return;
        syscreate (p, NULL);
        if (!bg)
            for (sig = 0; sig != SIGCHLD; sig = sigwait ());
    }
}

static void help (void *arg) {
    sysputs ("valid commands are:\n");
    for (int i = 0; i < TSH_N_CMDS; i++) {
        sysputs ("\t");
        sysputs (tsh_progs[i].name);
        sysputs ("\n");
    }
}

static void hello_tsh (void *arg) {
    sysreport ("Hello, tsh!\n");
}
