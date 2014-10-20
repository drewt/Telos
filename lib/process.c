/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <syscall.h>
#include <telos/process.h>

typedef int pid_t;

/*-----------------------------------------------------------------------------
 * Create a process */
//-----------------------------------------------------------------------------
pid_t syscreate(int(*func)(int,char**), int argc, char *argv[])
{
	return syscall3(SYS_CREATE, func, (void*) argc, argv);
}

pid_t fcreate(const char *pathname, int argc, char *const argv[])
{
	return syscall3(SYS_FCREATE, (void*)pathname, (void*)argc, (void*)argv);
}

/*-----------------------------------------------------------------------------
 * Yield the processor */
//-----------------------------------------------------------------------------
void sysyield(void)
{
	syscall0(SYS_YIELD);
}

/*-----------------------------------------------------------------------------
 * Terminate */
//-----------------------------------------------------------------------------
void exit(int status)
{
	syscall1(SYS_STOP, (void*) status);
}
