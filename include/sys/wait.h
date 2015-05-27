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

#ifndef _KERNEL_WAIT_H_
#define _KERNEL_WAIT_H_

#include <sys/type_defs.h>

#ifndef _ID_T_DEFINED
#define _ID_T_DEFINED
typedef _ID_T_TYPE id_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _UNION_SIGVAL_DEFINED
#define _UNION_SIGVAL_DEFINED
_UNION_SIGVAL_DEFN
#endif
#ifndef _SIGINFO_T_DEFINED
#define _SIGINFO_T_DEFINED
_SIGINFO_T_DEFN
#endif

#define WCONTINUED 1
#define WEXITED    2
#define WSIGNALLED 4
#define WSTOPPED   8
#define WNOHANG    16
#define WUNTRACED  32
#define WNOWAIT    64

#define _WCONTINUED 0
#define _WEXITED    1
#define _WSIGNALED 2
#define _WSTOPPED   3

/*
 * A status is made up of a 2-bit "how" value (one of _WCONTINUED, _WEXITED,
 * _WSIGNALLED or _WSTOPPED) and a generic n-bit (however many bits are left)
 * value, either the signal number or the exit status.
 *
 * val --> [...________][__] <-- how
 */
#define WEXITSTATUS(s)  (((s) & 0x3FC) >> 2)
#define WIFCONTINUED(s) (((s) & 0x3) == _WCONTINUED)
#define WIFEXITED(s)    (((s) & 0x3) == _WEXITED)
#define WIFSIGNALED(s)  (((s) & 0x3) == _WSIGNALED)
#define WIFSTOPPED(s)   (((s) & 0x3) == _WSTOPPED)
#define WSTOPSIG(s)     ((unsigned)((s) & ~0x3) >> 2)
#define WTERMSIG(s)     WSTOPSIG(s)

typedef enum {
	P_ALL,
	P_PGID,
	P_PID,
} idtype_t;

#ifdef __KERNEL__
#define WHOW(s) ((s) & 0x3)
#define WVALUE(s) WSTOPSIG(s)
static inline int make_status(int how, int val)
{
	return (how & 0x3) | ((unsigned)val << 2);
}
#else
pid_t wait(int *stat_loc);
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
pid_t waitpid(pid_t pid, int *stat_loc, int options);
#endif
#endif
