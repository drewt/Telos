/* Copyright (c) 2013-2015, Drew Thoreson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _TELOS_WAIT_H_
#define _TELOS_WAIT_H_

#define WCONTINUED 1
#define WEXITED    2
#define WSIGNALLED 4
#define WSTOPPED   8
#define WNOHANG    16
#define WUNTRACED  32
#define WNOWAIT    64

#define _WCONTINUED 0
#define _WEXITED    1
#define _WSIGNALED  2
#define _WSTOPPED   3

#ifndef __ASSEMBLER__
#include <telos/type_defs.h>

#ifndef _ID_T_DEFINED
#define _ID_T_DEFINED
typedef _ID_T_TYPE id_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif
#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED
typedef _UID_T_TYPE uid_t;
#endif
#ifndef _UNION_SIGVAL_DEFINED
#define _UNION_SIGVAL_DEFINED
_UNION_SIGVAL_DEFN
#endif
#ifndef _SIGINFO_T_DEFINED
#define _SIGINFO_T_DEFINED
_SIGINFO_T_DEFN
#endif

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
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLER__ */
#endif
