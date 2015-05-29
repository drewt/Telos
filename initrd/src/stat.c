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
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	struct stat s;
	if (argc != 2) {
		printf("wrong number of arguments");
		return -1;
	}
	if (stat(argv[1], &s))
		return -1;

	printf("%s:\n"
			"\tdev   = %lu\n"
			"\tino   = %lu\n"
			"\tmode  = %lx\n"
			"\tnlink = %lu\n"
			"\tuid   = %lu\n"
			"\tgid   = %lu\n"
			"\trdev  = %lu\n"
			"\tsize  = %lu\n"
			"\ticnt  = %u\n",
			argv[1],
			s.st_dev,
			s.st_ino,
			s.st_mode,
			s.st_nlink,
			s.st_uid,
			s.st_gid,
			s.st_rdev,
			s.st_size,
			s.st_icount);
	return 0;
}
