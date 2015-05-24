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

#include <stdio.h>
#include <string.h>

static void strcmp_test(void)
{
	int error = 0;
	char *a = "equal";
	char *b = "equal? no";
	char *c = "a";
	char *d = "f";
	if (strcmp(a, a))
		error = 1;
	else if (!strcmp(a, b))
		error = 2;
	else if (strcmp(a, c) <= 0)
		error = 3;
	else if (strcmp(a, d) >= 0)
		error = 4;

	printf("strcmp: ");
	if (error) {
		printf("error %d\n", error);
		for (;;);
	} else {
		puts("OK");
	}
}

static void strcat_test(void)
{
	char *o = "origin";
	char d[7] = "ori";
	char s[4] = "gin";
	strcat(d, s);
	printf("strcat: ");
	if (strcmp(d, o))
		printf("error: %s != %s\n", d, o);
	else
		puts("OK");
}

static void strncat_test(void)
{
	int error = 0;
	char *o = "origin";
	char *t = "orig";
	char d[8] = "ori";
	char s[4] = "gin";
	strncat(d, s, 1);
	if (strcmp(d, t))
		error = 1;
	d[3] = '\0';
	strncat(d, s, 4);
	if (strcmp(d, o))
		error = 2;
	printf("strncat: ");
	if (error)
		printf("error %d\n", error);
	else
		puts("OK");
}

static void strchr_test(void)
{
	int error = 0;
	char *res;
	char *o = "origin";
	if (!(res = strchr(o, 'i')))
		error = 1;
	else if (strcmp(res, "igin"))
		error = 2;
	else if (strchr(o, 'x'))
		error = 3;
	printf("strchr: ");
	if (error)
		printf("error: %d\n", error);
	else
		puts("OK");
}

static void strrchr_test(void)
{
	int error = 0;
	char *res;
	char *o = "origin";
	if (!(res = strrchr(o, 'i')))
		error = 1;
	else if (strcmp(res, "in"))
		error = 2;
	else if (strrchr(o, 'x'))
		error = 3;
	printf("strrchr: ");
	if (error)
		printf("error %d\n", error);
	else
		puts("OK");
}

static void strtok_test(void)
{
	char *tok;
	int i, error = 0;
	char str[] = "this(is).a.string$with/delimiters";
	char toks[7][11] = { "this", "is", "", "a", "string", "with", "delimiters" };

	for (i = 0, tok = strtok(str, "().$/"); tok != NULL;
			i++, tok = strtok(NULL, "().$/")) {
		if (strcmp(tok, toks[i]))
			error = i+1;
	}
	printf("strtok: ");
	if (error)
		printf("error %d\n", error);
	else
		puts("OK");
}

int main(int argc, char *argv[])
{
	strcmp_test();
	strcat_test();
	strncat_test();
	strchr_test();
	strrchr_test();
	strtok_test();
	return 0;
}
