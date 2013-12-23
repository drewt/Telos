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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define N 512

static void sbrk_test(void)
{
	char *brk = NULL;

	brk = sbrk(N);
	printf("old brk: %x\n", brk);

	for (int i = 0; i < N; i++)
		brk[i] = 0xFF;

	brk = sbrk(0);
	printf("new brk: %x\n", brk);
}

void memtest(int argc, char *argv[])
{
	sbrk_test();
}
