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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <telos/process.h>
#include <telos/console.h>

#include <usr/test.h>

#define N_CMDS   (sizeof progtab / sizeof *progtab)
#define IN_LEN   512
#define MAX_ARGS 100

#define SHELL_EXIT  ((void*)-1)
#define SHELL_CLEAR ((void*)-2)

#define PROMPT "TELOS> "

typedef void(*funcptr)(int, char**);

void tsh ();

static void help (int argc, char *argv[]);
extern void echo (int argc, char *argv[]);
extern void printserver (int argc, char *argv[]);
extern void printclient (int argc, char *argv[]);

struct program {
    funcptr f;
    char *name;
};

static struct program progtab[] = {
    { SHELL_EXIT,  "exit"        },
    { SHELL_CLEAR, "clear"       },
    { help,        "help"        },
    { proc_test,   "proctest"    },
    { sig_test,    "sigtest"     },
    { kbd_test,    "kbdtest"     },
    { str_test,    "strtest"     },
    { event_test,  "eventtest"   },
    { msg_test,    "msgtest"     },
    { memtest,     "memtest"     },
    { echo,        "echo"        },
    { printserver, "printserver" },
    { printclient, "printclient" },
    { tsh,         "tsh"         }
};

static void sigchld_handler (int signo) {}

static void help (int argc, char *argv[])
{
    puts ("Valid commands are:");
    for (size_t i = 0; i < N_CMDS; i++)
        printf ("\t%s\n", progtab[i].name);
}

static funcptr lookup (char *in)
{
    size_t i;
    for (i = 0; i < N_CMDS; i++)
        if (!strcmp (in, progtab[i].name))
            return progtab[i].f;
    return NULL;
}

static size_t read_line (char *buf, size_t len)
{
    int c;
    int pos = 0;

    len--;
    while ((c = getchar ()) != '\n' && pos < len) {
        if (c == '\b' && pos > 0)
            pos--;
        else
            buf[pos++] = c;
    }
    buf[pos] = '\0';
}

static int parse_input (char *in, char **name, char *(*args)[])
{
    int i;
    int bg = 0;
    char *tmp;

    *name = strtok (in, " \t");
    for (i = 0; (tmp = strtok (NULL, " \t")) != NULL; i++) {
        if (*tmp == '&' && *(tmp+1) == '\0')
            bg = 1;
        else
            (*args)[i] = tmp;
    }

    (*args)[i] = NULL;
    return bg;
}

void tsh ()
{
    funcptr p;
    char *name;
    char in[IN_LEN];
    char *argv[MAX_ARGS];
    int argc;
    int sig;
    int bg;

    signal (SIGCHLD, sigchld_handler);

    while (1) {
        printf (PROMPT);
        read_line (in, IN_LEN);
        if (*in == '\0')
            continue;

        bg = parse_input (in, &name, &argv);
        if (!(p = lookup (name))) {
            printf ("tsh: '%s' not found\n", in);
            continue;
        }

        if (p == SHELL_EXIT)
            return;
        if (p == SHELL_CLEAR) {
            ioctl (STDOUT_FILENO, CONSOLE_IOCTL_CLEAR);
            continue;
        }

        for (argc = 0; argv[argc] != NULL; argc++);
        syscreate (p, argc, argv);
        if (!bg)
            for (sig = 0; sig != SIGCHLD; sig = sigwait ());
    }
}
