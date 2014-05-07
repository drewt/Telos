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

#define N 25
#define M 100
#define A 144

int main(int argc, char *argv[])
{
	void *mem[N];

	puts("Testing palloc()...");
	for (int i = 0; i < N; i++) {
		if ((mem[i] = palloc()) == NULL) {
			printf("memtest[%d]: palloc returned NULL\n", i);
			return -1;
		}
		memset(mem[i], 0xFF, 4096);
	}

	unsigned long *bad = (void*) (0x00112000 + 0xC0000000);
	*bad = 0xFF;

	/*puts("Testing malloc()...");
	for (int i = 0; i < N; i++) {
		if ((mem[i] = malloc(i*M+A)) == NULL) {
			puts("memtest: malloc returned NULL");
			return;
		}
	}

	puts("Testing free()...");
	for (int i = N-1; i >= 0; i--)
		free(mem[i]);*/
	return 0;
}
