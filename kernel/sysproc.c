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
#include <signal.h>
#include <unistd.h>

#include <telos/process.h>

int tsh(int argc, char **argv);

int idle_proc()
{
	asm volatile("halt: hlt\njmp halt");
	return 0;
}

static void sigchld_handler(int signo) {}

static void reboot(void)
{
	asm volatile(
	"cli			\n"
	"rb_loop:		\n"
		"inb $0x64, %al	\n"
		"and $0x2,  %al	\n"
		"cmp $0x0,  %al	\n"
		"jne  rb_loop	\n"
	"mov $0xFE, %al		\n"
	"out %al, $0x64		\n"
	);
}

void root_proc()
{
	int sig;

	signal(SIGCHLD, sigchld_handler);

	syscreate(tsh, 0, NULL);
	for (sig = sigwait(); sig != SIGCHLD; sig = sigwait());

	printf("\nPress any key to reboot");
	getchar();
	reboot();
}
