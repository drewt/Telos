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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#ifndef INTR_SYSCALL
#define INTR_SYSCALL 0x80
#endif

enum {
	SYS_YIELD         = 0,
	SYS_EXIT          = 1,
	SYS_WAITID        = 2,
	SYS_GETPID        = 3,
	SYS_GETPPID       = 4,
	SYS_SLEEP         = 5,
	SYS_SIGRETURN     = 6,
	SYS_KILL          = 7,
	SYS_SIGQUEUE      = 8,
	SYS_SIGWAIT       = 9,
	SYS_SIGSUSPEND    = 10,
	SYS_SIGACTION     = 11,
	SYS_SIGMASK       = 12,
	SYS_OPEN          = 13,
	SYS_CLOSE         = 14,
	SYS_READ          = 15,
	SYS_PREAD         = 16,
	SYS_WRITE         = 17,
	SYS_PWRITE        = 18,
	SYS_LSEEK         = 19,
	SYS_IOCTL         = 20,
	SYS_ALARM         = 21,
	SYS_TIMER_CREATE  = 22,
	SYS_TIMER_DELETE  = 23,
	SYS_TIMER_GETTIME = 24,
	SYS_TIMER_SETTIME = 25,
	SYS_TIME          = 26,
	SYS_CLOCK_GETRES  = 27,
	SYS_CLOCK_GETTIME = 28,
	SYS_CLOCK_SETTIME = 29,
	SYS_SBRK          = 30,
	SYS_MMAP          = 40,
	SYS_MUNMAP        = 41,
	SYS_MOUNT         = 42,
	SYS_UMOUNT        = 43,
	SYS_MKDIR         = 44,
	SYS_READDIR       = 45,
	SYS_MKNOD         = 46,
	SYS_RMDIR         = 47,
	SYS_UNLINK        = 48,
	SYS_LINK          = 49,
	SYS_RENAME        = 50,
	SYS_CHDIR         = 51,
	SYS_STAT          = 52,
	SYS_TRUNCATE      = 53,
	SYS_FCNTL         = 54,
	SYS_FORK          = 55,
	SYS_EXECVE        = 56,
	SYS_FSTAT         = 57,
	SYSCALL_MAX       = 58
};

static inline int syscall0(int call)
{
	int rc;
	asm volatile(
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
	asm volatile(
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
	asm volatile(
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
	asm volatile(
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
	asm volatile(
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
	asm volatile(
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

#endif
