/*  Copyright 2013-2015 Drew Thoreson
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
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <telos/console.h>

#define IN_LEN   512
#define MAX_ARGS 100

#define PROMPT "TELOS> "

static _Noreturn void builtin_exit(void)
{
	exit(0);
}

static int builtin_cd(int argc, char *argv[])
{
	if (argc != 2) {
		printf("cd: wrong number of arguments\n");
		return -1;
	}
	chdir(argv[1]);
	return 0;
}

static int builtin_clear(void)
{
	ioctl(STDOUT_FILENO, CONSOLE_IOCTL_CLEAR);
	return 0;
}

typedef int(*funptr)(int, char**);

struct tsh_builtin {
	funptr f;
	char *name;
};

#define BUILTIN(fun, name) { (funptr)fun, name }
static struct tsh_builtin builtins[] = {
	BUILTIN(builtin_exit,  "exit"),
	BUILTIN(builtin_cd,    "cd"),
	BUILTIN(builtin_clear, "clear"),
};
#define NR_BUILTIN (sizeof(builtins)/sizeof(*builtins))

static funptr builtin_lookup(const char *name)
{
	for (unsigned i = 0; i < NR_BUILTIN; i++)
		if (!strcmp(name, builtins[i].name))
			return builtins[i].f;
	return NULL;
}

static void sigchld_handler(int signo) {}

static ssize_t read_line(char *buf, size_t len)
{
	int c;
	size_t pos = 0;

	len--;
	while ((c = getchar()) != '\n' && pos < len-1) {
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

static int exec_with_prefix(const char *prefix, char *argv[], char *envp[])
{
	char path[IN_LEN + 16];
	strcpy(path, prefix);
	strcat(path, argv[0]);
	return execve(path, argv, envp);
}

static void reaper(int signo)
{
	int status;
	wait(&status);
}

int main(int _argc, char *_argv[])
{
	funptr p;
	char in[IN_LEN];
	char *argv[MAX_ARGS];
	int argc;
	sigset_t set;

	sigfillset(&set);
	sigdelset(&set, SIGCHLD);
	signal(SIGCHLD, reaper);

	while (1) {
		printf(PROMPT);
		if (read_line(in, IN_LEN) == EOF)
			return 0;
		if (*in == '\0')
			continue;

		parse_input(in, &argv);
		for (argc = 0; argv[argc]; argc++);
		if ((p = builtin_lookup(argv[0]))) {
			p(argc, argv);
			continue;
		}

		// FIXME: fork/exec seems to cause stack corruption in some cases
		if (!fork()) {
			char *envp[] = { NULL };
			execve(argv[0], argv, envp);
			exec_with_prefix("/bin/", argv, envp);
			printf("tsh: %s: command not found\n", argv[0]);
			exit(1);
		}

		sigsuspend(&set);
	}
}
