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
#include <unistd.h>

int main(int argc, char *argv[])
{
	off_t length;
	if (argc != 3) {
		fprintf(stderr, "truncate: wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}
	length = atoi(argv[2]);
	if (truncate(argv[1], length) < 0) {
		fprintf(stderr, "truncate: truncate() call failed\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
