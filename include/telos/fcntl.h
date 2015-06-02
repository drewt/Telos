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

#ifndef _TELOS_FCNTL_H_
#define _TELOS_FCNTL_H_

#include <telos/stat.h> /* file modes */

#define F_DUPFD         1
#define F_DUP2FD        2
#define F_DUPFD_CLOEXEC 3
#define F_GETFD         4
#define F_SETFD         5
#define F_GETFL         6
#define F_SETFL         7
#define F_GETLK         8
#define F_SETLK         9
#define F_SETLKW        10
#define F_GETOWN        11
#define F_SETOWN        12

#define FD_CLOEXEC 1

#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

#define O_RDONLY    0x1
#define O_WRONLY    0x2
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_EXEC      0x4
#define O_SEARCH    0x8
#define O_APPEND    0x10
#define O_CLOEXEC   0x20
#define O_CREAT     0x40
#define O_DIRECTORY 0x80
#define O_DSYNC     0x100
#define O_EXCL      0x200
#define O_NOCTTY    0x400
#define O_NOFOLLOW  0x800
#define O_NONBLOCK  0x1000
#define O_RSYNC     0x2000
#define O_SYNC      0x4000
#define O_TRUNC     (0x8000 | O_WRONLY)
#define O_TTY_INIT  0x10000

#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)

/* fd for *at functions */
#define AT_FDWCD (-1)

/* flags for faccessat */
#define AT_EACCESS 1

/* flags for fstatat, fchmodat, fchownat, utimensat */
#define AT_SYMLINK_NOFOLLOW 1

/* flags for linkat */
#define AT_SYMLINK_FOLLOW 1

/* flags for unlinkat */
#define AT_REMOVEDIR 1

/* advice for posix_fadvise */
#define POSIX_FADV_DONTNEED   1
#define POSIX_FADV_NOREUSE    2
#define POSIX_FADV_NORMAL     3
#define POSIX_FADV_RANDOM     4
#define POSIX_FADV_SEQUENTIAL 5
#define POSIX_FADV_WILLNEED   6

#ifndef __ASSEMBLER__
#include <telos/type_defs.h>

#ifndef _MODE_T_DEFINED
#define _MODE_T_DEFINED
typedef _MODE_T_TYPE mode_t;
#endif
#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef _OFF_T_TYPE off_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif

struct flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
};

#ifdef __KERNEL__
#define O_READ  O_RDONLY
#define O_WRITE O_WRONLY
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLER__ */
#endif
