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
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>

static _Noreturn void usage(void)
{
	fprintf(stderr, "Usage: mount <src> <target> <type> [flags...]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	unsigned long flags = 0;

	if (argc < 4)
		usage();

	if (argc > 4) {
		for (int i = 4; i < argc; i++) {
			if (!strcmp(argv[i], "ro"))
				flags |= MS_RDONLY;
			else if (!strcmp(argv[i], "nosuid"))
				flags |= MS_NOSUID;
			else if (!strcmp(argv[i], "nodev"))
				flags |= MS_NODEV;
			else if (!strcmp(argv[i], "noexec"))
				flags |= MS_NOEXEC;
			else if (!strcmp(argv[i], "sync"))
				flags |= MS_SYNC;
			else if (!strcmp(argv[i], "remount"))
				flags |= MS_REMOUNT;
		}
	}
	int rc = mount(argv[1], argv[2], argv[3], flags, NULL);
	if (rc < 0)
		printf("Failed to mount %s\n", argv[1]);
	return rc;
}
