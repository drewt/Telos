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

#include <stdio.h>
#include <unistd.h>
#include <telos/console.h> /* ioctl commands */

void consoletest(int argc, char *argv[])
{
	int fd;

	puts("This is standard output");
	sleep(10);

	if ((fd = open("/dev/cons1", 0)) == -1) {
		puts("error opening console 1");
		return;
	}

	if (ioctl(fd, CONSOLE_IOCTL_SWITCH, 1) == -1) {
		puts("error switching to console 1");
		return;
	}
	puts("Printing to hidden console!");

	if (write(fd, "This is console 1\n", 18) == -1) {
		puts("error writing to console 1");
		return;
	}

	sleep(10);

	if (ioctl(fd, CONSOLE_IOCTL_SWITCH, 0) == -1) {
		puts("error switching to console 0");
		return;
	}

	puts("Back to standard output");
}
