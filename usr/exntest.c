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
#include <signal.h>

#include <telos/process.h>

int _dbz_ = 0;

int dbz_proc(void *arg)
{
	printf("Dividing by zero... ");
	_dbz_ = 1 / _dbz_;
	printf("error\n");
	return 0;
}

void sigchld_handler(int sig) {}

int main(int argc, char *argv[])
{
	signal(SIGCHLD, sigchld_handler);

	syscreate(dbz_proc, NULL);
	for (int sig = 0; sig != SIGCHLD; sig = sigwait());
	printf("done.\n");
	return 0;
}
