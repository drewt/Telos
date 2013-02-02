/* process.h : process control system calls
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

#ifndef __PROCESS_H_
#define __PROCESS_H_

#ifndef __pid_t_defined
typedef int pid_t;
#define __pid_t_defined
#endif // __pid_t_defined

#ifdef __need_pid_t
#undef __need_pid_t
#undef __PROCESS_H_
#else

int syscreate (void(*func)(int,char**), int argc, char *argv[]);
void sysyield (void);
void sysstop (void);

#endif // __need_pid_t
#endif // __PROCESS_H_
