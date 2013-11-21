#include <stdio.h>

void echo(int argc, char *argv[])
{
	if (argc < 1) {
		puts("usage: echo [args]");
		return;
	}

	printf("%s", argv[0]);
	for (int i = 1; i < argc; i++)
		printf(" %s", argv[i]);
	putchar('\n');
}
