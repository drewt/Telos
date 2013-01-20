/* strtest.c : string.h tests
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

#include <string.h>

#include <kernel/common.h>
#include <telos/print.h>
#include <usr/test.h>

static void strcmp_test (void) {
    int error = 0;
    char *a = "equal";
    char *b = "equal? no";
    char *c = "a";
    char *d = "f";
    if (strcmp (a, a))
        error = 1;
    else if (!strcmp (a, b))
        error = 2;
    else if (strcmp (a, c) <= 0)
        error = 3;
    else if (strcmp (a, d) >= 0)
        error = 4;

    sysputs ("strcmp: ");
    if (error) {
        kprintf ("error %d\n", error);
        for (;;);
    }
    else
        sysputs ("OK\n");
}

static void strcat_test (void) {
    char *o = "origin";
    char d[7] = "ori";
    char s[4] = "gin";
    strcat (d, s);
    sysputs ("strcat: ");
    if (strcmp (d, o)) {
        sysputs ("error\n");
        sysputs (d);
        sysputs (" : ");
        sysputs (o);
        for(;;);
    }
    else
        sysputs ("OK\n");
}

static void strncat_test (void) {
    int error = 0;
    char *o = "origin";
    char *t = "orig";
    char d[8] = "ori";
    char s[4] = "gin";
    strncat (d, s, 1);
    if (strcmp (d, t))
        error = 1;
    d[3] = '\0';
    strncat (d, s, 4);
    if (strcmp (d, o))
        error = 2;
    sysputs ("strncat: ");
    if (error) {
        kprintf ("error %d\n", error);
        for(;;);
    }
    else
        sysputs ("OK\n");
}

static void strchr_test (void) {
    int error = 0;
    char *res;
    char *o = "origin";
    if (!(res = strchr (o, 'i')))
        error = 1;
    else if (strcmp (res, "igin"))
        error = 2;
    else if (strchr (o, 'x'))
        error = 3;
    sysputs ("strchr: ");
    if (error) {
        kprintf ("error: %d\n", error);
        for(;;);
    }
    else
        sysputs ("OK\n");
}

static void strrchr_test (void) {
    int error = 0;
    char *res;
    char *o = "origin";
    if (!(res = strrchr (o, 'i')))
        error = 1;
    else if (strcmp (res, "in"))
        error = 2;
    else if (strrchr (o, 'x'))
        error = 3;
    sysputs ("strrchr: ");
    if (error) {
        kprintf ("error %d\n", error);
        for(;;);
    }
    else
        sysputs ("OK\n");
}

static void strtok_test (void) {
    char *tok;
    int i, error = 0;
    char str[] = "this(is).a.string$with/delimiters";
    char toks[7][11] = { "this", "is", "", "a", "string", "with", "delimiters" };

    for (i = 0, tok = strtok (str, "().$/"); tok != NULL;
            i++, tok = strtok (NULL, "().$/")) {
        if (strcmp (tok, toks[i]))
            error = i+1;
    }
    sysputs ("strtok: ");
    if (error)
        kprintf ("error %d\n", error);
    else
        sysputs ("OK\n");
}

void str_test (void *arg) {
    strcmp_test ();
    strcat_test ();
    strncat_test ();
    strchr_test ();
    strrchr_test ();
    strtok_test ();
}
