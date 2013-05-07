/* syscall.c : system calls
 */

/*  Copyright 2013 Drew T.
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
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

/* PROCESS.H */

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
pid_t syscreate (void(*func)(int,char**), int argc, char *argv[]) {
    return syscall3 (SYS_CREATE, func, (void*) argc, argv);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysyield (void) {
    syscall0 (SYS_YIELD);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysstop (void) {
    syscall0 (SYS_STOP);
}

/* PRINT.H */

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sysreport (char *s) {
    syscall1 (SYS_REPORT, s);
}
