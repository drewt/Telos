/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
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
#define SHELL_CD    ((void*)-3)

#define PROMPT "TELOS> "

typedef int(*funcptr)(int, char**);

int main(int argc, char *argv[]);

#define PROGRAM(name) int name(int argc, char *argv[])

static PROGRAM(help);
extern PROGRAM(cat);
extern PROGRAM(echo);
extern PROGRAM(echoserver);
extern PROGRAM(echoclient);
extern PROGRAM(uls);
extern PROGRAM(mount_ramfs);
extern PROGRAM(uumount);
extern PROGRAM(umkdir);
extern PROGRAM(urmdir);
extern PROGRAM(ulink);
extern PROGRAM(uunlink);
extern PROGRAM(urename);
extern PROGRAM(ustat);
extern PROGRAM(multi);

struct program {
	funcptr f;
	char *name;
};

static struct program progtab[] = {
	{ SHELL_EXIT,	"exit"		},
	{ SHELL_CLEAR,	"clear"		},
	{ SHELL_CD,     "cd"            },
	{ help,		"help"		},
	{ exntest,	"exntest"	},
	{ proctest,	"proctest"	},
	{ sigtest,	"sigtest"	},
	{ kbdtest,	"kbdtest"	},
	{ strtest,	"strtest"	},
	{ eventtest,	"eventtest"	},
	{ msgtest,	"msgtest"	},
	{ memtest,	"memtest"	},
	{ consoletest,	"consoletest"	},
	{ jmptest,      "jmptest"       },
	{ cat,          "cat"           },
	{ echo,		"echo"		},
	{ date,		"date"		},
	{ echoserver,   "echoserver"    },
	{ echoclient,   "echoclient"    },
	{ uls,          "ls"            },
	{ umkdir,       "mkdir"         },
	{ urmdir,       "rmdir"         },
	{ mount_ramfs,  "mount-ramfs"   },
	{ uumount,      "umount"        },
	{ ustat,        "stat"          },
	{ ulink,        "link"          },
	{ uunlink,      "unlink"        },
	{ urename,      "rename"        },
	{ multi,        "multi"         },
	{ main,		"tsh"		}
};

static void sigchld_handler(int signo) {}

static int help(int argc, char *argv[])
{
	puts("Valid commands are:");
	for (size_t i = 0; i < N_CMDS; i++)
		printf("\t%s\n", progtab[i].name);
	return 0;
}

static funcptr lookup(char *in)
{
	size_t i;
	for (i = 0; i < N_CMDS; i++)
		if (!strcmp(in, progtab[i].name))
			return progtab[i].f;
	return NULL;
}

static ssize_t read_line(char *buf, size_t len)
{
	int c;
	size_t pos = 0;

	len--;
	while ((c = getchar()) != '\n' && pos < len) {
		if (c == EOF)
			return EOF;
		else
			buf[pos++] = c;
	}
	buf[pos] = '\0';
	return pos;
}

#define skip_ws(str) \
	for (;*(str) == ' ' || *(str) == '\t'; (str)++)
#define skip_nonws(str) \
	for (;*(str) != '\0' && *(str) != ' ' && *(str) != '\t'; (str)++)

static char *consume_argument(char *s)
{
	static char *tokp;

	if (s != NULL)
		tokp = s;

	skip_ws(tokp);
	if (*tokp == '\0')
		return NULL;
	s = tokp;

	if (*tokp == '\'') {
		s = ++tokp;
		while (*tokp != '\0' && *tokp != '\'')
			tokp++;
	} else {
		skip_nonws(tokp);
	}

	if (*tokp != '\0')
		*tokp++ = '\0';

	return s;
}

static int parse_input(char *in, char *(*args)[])
{
	int i;
	int bg = 0;
	char *tmp;

	/* get program name */
	skip_ws(in);
	(*args)[0] = in;
	skip_nonws(in);

	if (*in != '\0')
		*in++ = '\0';

	/* get arguments */
	for (i = 1, tmp = consume_argument(in); tmp != NULL;
			tmp = consume_argument(NULL), i++) {
		if (*tmp == '&' && *(tmp+1) == '\0')
			bg = 1;
		else
			(*args)[i] = tmp;
	}

	(*args)[i] = NULL;
	return bg;
}

int main(int _argc, char *_argv[])
{
	funcptr p;
	char in[IN_LEN];
	char *argv[MAX_ARGS];
	int argc;
	int sig;
	int bg;

	signal(SIGCHLD, sigchld_handler);

	if (getpid() != 2) for(;;);
	while (1) {
		printf(PROMPT);
		if (read_line(in, IN_LEN) == EOF)
			return 0;
		if (*in == '\0')
			continue;

		bg = parse_input(in, &argv);
		if (!(p = lookup(argv[0]))) {
			printf("tsh: '%s' not found\n", argv[0]);
			continue;
		}

		if (p == SHELL_EXIT)
			return 0;
		if (p == SHELL_CLEAR) {
			ioctl(STDOUT_FILENO, CONSOLE_IOCTL_CLEAR);
			continue;
		}
		if (p == SHELL_CD) {
			if (argv[1])
				chdir(argv[1]);
			continue;
		}

		for (argc = 0; argv[argc] != NULL; argc++);
		syscreate(p, argc, argv);
		if (!bg)
			for (sig = 0; sig != SIGCHLD; sig = sigwait());
	}
}
