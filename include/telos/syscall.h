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

#ifndef _TELOS_SYSCALL_H_
#define _TELOS_SYSCALL_H_

#ifndef INTR_SYSCALL
#define INTR_SYSCALL 0x80
#endif

#define SYS_YIELD         0
#define SYS_EXIT          1
#define SYS_WAITID        2
#define SYS_GETPID        3
#define SYS_GETPPID       4
#define SYS_SLEEP         5
#define SYS_SIGRETURN     6
#define SYS_KILL          7
#define SYS_SIGQUEUE      8
#define SYS_SIGWAIT       9
#define SYS_SIGSUSPEND    10
#define SYS_SIGACTION     11
#define SYS_SIGMASK       12
#define SYS_OPEN          13
#define SYS_CLOSE         14
#define SYS_READ          15
#define SYS_PREAD         16
#define SYS_WRITE         17
#define SYS_PWRITE        18
#define SYS_LSEEK         19
#define SYS_IOCTL         20
#define SYS_ALARM         21
#define SYS_TIMER_CREATE  22
#define SYS_TIMER_DELETE  23
#define SYS_TIMER_GETTIME 24
#define SYS_TIMER_SETTIME 25
#define SYS_TIME          26
#define SYS_CLOCK_GETRES  27
#define SYS_CLOCK_GETTIME 28
#define SYS_CLOCK_SETTIME 29
#define SYS_SBRK          30
#define SYS_MMAP          40
#define SYS_MUNMAP        41
#define SYS_MOUNT         42
#define SYS_UMOUNT        43
#define SYS_MKDIR         44
#define SYS_READDIR       45
#define SYS_MKNOD         46
#define SYS_RMDIR         47
#define SYS_UNLINK        48
#define SYS_LINK          49
#define SYS_RENAME        50
#define SYS_CHDIR         51
#define SYS_STAT          52
#define SYS_TRUNCATE      53
#define SYS_FCNTL         54
#define SYS_FORK          55
#define SYS_EXECVE        56
#define SYS_FSTAT         57
#define SYS_PIPE          58
#define SYSCALL_MAX       59

#ifndef __ASSEMBLER__
static inline int syscall0(int call)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"int  %[xnum]		\n"
	"movl %%eax,   %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [xnum] "i" (INTR_SYSCALL)
	: "%eax", "memory"
	);
	return rc;
}

#define syscall1(call, arg0) __syscall1(call, (unsigned long) arg0)
static inline int __syscall1(int call, unsigned long arg0)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"movl %[arg0], %%ebx	\n"
	"int  %[xnum]		\n"
	"movl %%eax, %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [arg0] "g" (arg0), [xnum] "i" (INTR_SYSCALL)
	: "%eax", "%ebx", "memory"
	);
	return rc;
}

#define syscall2(call, arg0, arg1) \
	__syscall2(call, (unsigned long) arg0, (unsigned long) arg1)
static inline int __syscall2(int call, unsigned long arg0, unsigned long arg1)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"movl %[arg0], %%ebx	\n"
	"movl %[arg1], %%ecx	\n"
	"int  %[xnum]		\n"
	"movl %%eax, %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
	  [xnum] "i" (INTR_SYSCALL)
	: "%eax", "%ebx", "%ecx", "memory"
	);
	return rc;
}

#define syscall3(call, arg0, arg1, arg2) \
	__syscall3(call, (unsigned long) arg0, (unsigned long) arg1, \
			(unsigned long) arg2)
static inline int __syscall3(int call, unsigned long arg0, unsigned long arg1,
		unsigned long arg2)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"movl %[arg0], %%ebx	\n"
	"movl %[arg1], %%ecx	\n"
	"movl %[arg2], %%edx	\n"
	"int  %[xnum]		\n"
	"movl %%eax, %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
	  [arg2] "g" (arg2), [xnum] "i" (INTR_SYSCALL)
	: "%eax", "%ebx", "%ecx", "%edx", "memory"
	);
	return rc;
}

#define syscall4(call, arg0, arg1, arg2, arg3) \
	__syscall4(call, (unsigned long) arg0, (unsigned long) arg1, \
			(unsigned long) arg2, (unsigned long) arg3)
static inline int __syscall4(int call, unsigned long arg0, unsigned long arg1,
		unsigned long arg2, unsigned long arg3)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"movl %[arg0], %%ebx	\n"
	"movl %[arg1], %%ecx	\n"
	"movl %[arg2], %%edx	\n"
	"movl %[arg3], %%edi	\n"
	"int  %[xnum]		\n"
	"movl %%eax, %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
	  [arg2] "g" (arg2), [arg3] "g" (arg3), [xnum] "i" (INTR_SYSCALL)
	: "%eax", "%ebx", "%ecx", "%edx", "%edi", "memory"
	);
	return rc;
}

#define syscall5(call, arg0, arg1, arg2, arg3, arg4) \
	__syscall5(call, (unsigned long) arg0, (unsigned long) arg1, \
			(unsigned long) arg2, (unsigned long) arg3, \
			(unsigned long) arg4)
static inline int __syscall5(int call, unsigned long arg0, unsigned long arg1,
		unsigned long arg2, unsigned long arg3, unsigned long arg4)
{
	int rc;
	__asm__ volatile(
	"movl %[call], %%eax	\n"
	"movl %[arg0], %%ebx	\n"
	"movl %[arg1], %%ecx	\n"
	"movl %[arg2], %%edx	\n"
	"movl %[arg3], %%edi	\n"
	"movl %[arg4], %%esi	\n"
	"int  %[xnum]		\n"
	"movl %%eax, %[rc]	\n"
	: [rc] "=g" (rc)
	: [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
	  [arg2] "g" (arg2), [arg3] "g" (arg3), [arg4] "g" (arg4),
	  [xnum] "i" (INTR_SYSCALL)
	: "%eax", "%ebx", "%ecx", "%edx", "%edi", "%esi", "memory"
	);
	return rc;
}
#endif /* !__ASSEMBLER__ */
#endif
