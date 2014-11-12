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
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>

pid_t waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	return syscall4(SYS_WAITID, idtype, id, infop, options);
}

pid_t waitpid(pid_t pid, int *stat_loc, int options)
{
	siginfo_t info;
	idtype_t type;
	id_t id;

	if (pid < -1) {
		type = P_PGID;
		id = -pid;
	} else if (pid == -1) {
		type = P_ALL;
		id = 0;
	} else if (pid == 0) {
		type = P_PGID;
		id = getpid(); // FIXME
	} else {
		type = P_PID;
		id = pid;
	}
	if (waitid(type, id, &info, options | WEXITED) < 0)
		return -1;
	if (stat_loc)
		*stat_loc = info.si_value.sigval_int;
	return info.si_pid;
}

pid_t wait(int *stat_loc)
{
	return waitpid(-1, stat_loc, 0);
}

void sched_yield(void)
{
	syscall0(SYS_YIELD);
}

void exit(int status)
{
	syscall1(SYS_EXIT, status);
}
