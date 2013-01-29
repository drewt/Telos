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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <telos/process.h>
#include <telos/io.h>

#include <klib.h>

#include <usr/test.h>

#define N_CMDS   12
#define IN_LEN   512
#define MAX_ARGS 100
#define SHELL_EXIT ((void*) -1)

static const char const *prompt = "TELOS> ";

typedef void(*funcptr)(int, char**);

void tsh ();

static void help (int argc, char *argv[]);
extern void printserver (int argc, char *argv[]);
extern void printclient (int argc, char *argv[]);

struct program {
    funcptr f;
    char *name;
};

static struct program progtab[N_CMDS] = {
    { SHELL_EXIT,  "exit"        },
    { help,        "help"        },
    { proc_test,   "proctest"    },
    { sig_test,    "sigtest"     },
    { kbd_test,    "kbdtest"     },
    { str_test,    "strtest"     },
    { event_test,  "eventtest"   },
    { msg_test,    "msgtest"     },
    { memtest,     "memtest"     },
    { printserver, "printserver" },
    { printclient, "printclient" },
    { tsh,         "tsh"         }
};

enum shellrc {
    RC_ARGS,
    RC_LEN,
    RC_EOF,
    RC_FG,
    RC_BG,
};

static void sigchld_handler (int signo) {}

static void help (int argc, char *argv[]) {
    puts ("Valid commands are:");
    for (int i = 0; i < N_CMDS; i++)
        printf ("\t%s\n", progtab[i].name);
}

static funcptr lookup (char *in) {
    int i;
    for (i = 0; i < N_CMDS; i++)
        if (!strcmp (in, progtab[i].name))
            return progtab[i].f;
    return NULL;
}

static enum shellrc get_cmd (char *in, int in_len, char *(*args)[], int nargs) {
    int i, c, arg = 0;
    enum shellrc rc = RC_LEN;
    bool in_space = false, bg = false;
    for (i = 0; i < in_len-1; i++) {
        c = getchar ();
        switch (c) {
        case '\b':
            if (i > 0)
                i -= 2;
            break;
        case '\t':
        case ' ':
            if (in_space) {
                i--;
            } else {
                in[i] = '\0';
                in_space = true;
            }
            break;
        case '&':
            i--;
            bg = true;
            break;
        case '\n':
            rc = bg ? RC_BG : RC_FG;
            goto end;
        case EOF:
            rc = RC_EOF;
            goto end;
        default:
            if (in_space) {
                if (arg >= nargs - 1) {
                    rc = RC_ARGS;
                    goto end;
                }
                in_space = false;
                (*args)[arg] = &in[i];
                arg++;
            }
            in[i] = (char) c;
            break;
        }
    }
end:
    in[i] = '\0';
    (*args)[arg] = NULL;
    return rc;
}

void tsh () {
    int sig, argc;
    char in[IN_LEN];
    char *argv[MAX_ARGS];
    enum shellrc rc;
    funcptr p;

    signal (SIGCHLD, sigchld_handler);

    char buf[21];
    snprintf (buf, 21, "formatted %s #%d\n", "string", 1);
    printf (buf);

    for (;;) {
        printf ("%s", prompt);
        rc = get_cmd (in, IN_LEN, &argv, MAX_ARGS);
        if (!(p = lookup (in))) {
            printf ("tsh: '%s' not found\n", in);
            continue;
        }

        if (p == SHELL_EXIT)
            return;

        for (argc = 0; argv[argc] != NULL; argc++);
        syscreate (p, argc, argv);
        if (rc != RC_BG)
            for (sig = 0; sig != SIGCHLD; sig = sigwait ());
    }
}
