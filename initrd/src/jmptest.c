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

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

void f1(void)
{
	longjmp(env, 1);
	puts("ERROR: returning from f1");
}

void f0(void)
{
	f1();
	puts("ERROR: returning from f0");
}

int main(int argc, char *argv[])
{
	printf("testing longjmp()... ");

	if (setjmp(env) == 0)
		f0();
	else
		puts("done");
}
