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
#include <string.h>
#include <unistd.h>

#define N 512

extern void print_free_list(void);

static void sbrk_test(void)
{
	unsigned char *brk = sbrk(N);

	for (int i = 0; i < N; i++)
		brk[i] = 0xFF;
}

static void malloc_test(void)
{
	unsigned char *t[N];
	unsigned char *m0 = malloc(N);
	unsigned char *m1 = malloc(N);
	unsigned char *brk = sbrk(0);

	printf("testing malloc()... ");

	if (m0 >= brk || m1 >= brk) {
		printf("error: allocated memory above brk\n");
		return;
	}

	memset(m1, 0, N);
	memset(m0, 1, N);

	for (int i = 0; i < N; i++) {
		if (m1[i] != 0) {
			printf("error: separately allocated blocks overlap\n");
			return;
		}
	}

	free(m1);
	free(m0);

	for (int i = 0; i < N; i++)
		t[i] = malloc(N+i);
	for (int i = 0; i < N; i++)
		memset(t[i], 0xAD, N+i);
	for (int i = 0; i < N; i++)
		free(t[i]);

	puts("OK");
}

static void realloc_test(void)
{
	unsigned char *ptr;

	printf("testing realloc()... ");

	if ((ptr = realloc(NULL, N)) == NULL) {
		printf("error: realloc() failed\n");
		return;
	}

	memset(ptr, 0xAD, N);

	if ((ptr = realloc(ptr, N*2)) == NULL) {
		printf("error: realloc() failed\n");
		return;
	}

	for (int i = 0; i < N; i++) {
		if (ptr[i] != 0xAD) {
			printf("error: realloc clobbered memory\n");
			return;
		}
	}

	puts("OK");
}

static inline int aligned(void *ptr, size_t alignment)
{
	return !((unsigned long)ptr & (alignment - 1));
}

static int aligned_alloc_test_alloc(size_t alignment, size_t size)
{
	unsigned char *ptr = aligned_alloc(alignment, size);
	if (ptr == NULL)
		return -1;
	if (!aligned(ptr, alignment))
		return -2;
	return 0;
}

static void aligned_alloc_test(void)
{
	printf("testing aligned_alloc()... ");

	#define run_test(alignment, size) \
		if (aligned_alloc_test_alloc(alignment, size)) { \
			printf("error: aligned_alloc(%u, %u) failed\n", \
					alignment, size); \
			return; \
		}

	run_test(16, 16);
	run_test(16, 32);
	run_test(32, 32);
	run_test(32, 64);
	#undef run_test
	puts("OK");
}

int main(int argc, char *argv[])
{
	sbrk_test();
	malloc_test();
	realloc_test();
	aligned_alloc_test();
	return 0;
}
