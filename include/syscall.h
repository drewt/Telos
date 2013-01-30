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

#ifndef __SYSCALL_H_
#define __SYSCALL_H_

#include <stdint.h>

#ifndef SYSCALL_INTR
#define SYSCALL_INTR 0x80
#endif

enum syscall_id {
    SYS_CREATE = 48, // 0-47 reserved for hardware interrupts
    SYS_YIELD,
    SYS_STOP,
    SYS_GETPID,
    SYS_REPORT,
    SYS_SLEEP,
    SYS_SIGRETURN,
    SYS_KILL,
    SYS_SIGWAIT,
    SYS_SIGACTION,
    SYS_SIGNAL,
    SYS_SIGMASK,
    SYS_SEND,
    SYS_RECV,
    SYS_REPLY,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_ALARM,
    SYS_MALLOC,
    SYS_FREE,
    SYSCALL_MAX
};

/* structure of arguments on the stack during a system call */
struct sys_args {
    uint32_t arg3;
    uint32_t arg4;
    uint64_t pad0; // unused
    uint32_t arg0;
    uint32_t arg2;
    uint32_t arg1;
    uint32_t call; // call type
};

static inline int syscall0 (int call) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "int  %[xnum]        \n"
        "movl %%eax,   %[rc] \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [xnum] "i" (SYSCALL_INTR)
        : "%eax"
    );
    return rc;
}

static inline int syscall1 (int call, void *arg0) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "movl %[arg0], %%ebx \n"
        "int  %[xnum]        \n"
        "movl %%eax, %[rc]   \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [arg0] "g" (arg0), [xnum] "i" (SYSCALL_INTR)
        : "%eax", "%ebx"
    );
    return rc;
}

static inline int syscall2 (int call, void *arg0, void *arg1) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "movl %[arg0], %%ebx \n"
        "movl %[arg1], %%ecx \n"
        "int  %[xnum]        \n"
        "movl %%eax, %[rc]   \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
          [xnum] "i" (SYSCALL_INTR)
        : "%eax", "%ebx", "%ecx"
    );
    return rc;
}

static inline int syscall3 (int call, void *arg0, void *arg1, void *arg2) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "movl %[arg0], %%ebx \n"
        "movl %[arg1], %%ecx \n"
        "movl %[arg2], %%edx \n"
        "int  %[xnum]        \n"
        "movl %%eax, %[rc]   \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
          [arg2] "g" (arg2), [xnum] "i" (SYSCALL_INTR)
        : "%eax", "%ebx", "%ecx", "%edx"
    );
    return rc;
}

static inline int syscall4 (int call, void *arg0, void *arg1, void *arg2,
        void *arg3) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "movl %[arg0], %%ebx \n"
        "movl %[arg1], %%ecx \n"
        "movl %[arg2], %%edx \n"
        "movl %[arg3], %%edi \n"
        "int  %[xnum]        \n"
        "movl %%eax, %[rc]   \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
          [arg2] "g" (arg2), [arg3] "g" (arg3), [xnum] "i" (SYSCALL_INTR)
        : "%eax", "%ebx", "%ecx", "%edx", "%edi"
    );
    return rc;
}

static inline int syscall5 (int call, void *arg0, void *arg1, void *arg2,
        void *arg3, void *arg4) {
    int rc;
    asm volatile (
        "movl %[call], %%eax \n"
        "movl %[arg0], %%ebx \n"
        "movl %[arg1], %%ecx \n"
        "movl %[arg2], %%edx \n"
        "movl %[arg3], %%edi \n"
        "movl %[arg4], %%esi \n"
        "int  %[xnum]        \n"
        "movl %%eax, %[rc]   \n"
        : [rc] "=g" (rc)
        : [call] "g" (call), [arg0] "g" (arg0), [arg1] "g" (arg1),
          [arg2] "g" (arg2), [arg3] "g" (arg3), [arg4] "g" (arg4),
          [xnum] "i" (SYSCALL_INTR)
        : "%eax", "%ebx", "%ecx", "%edx"
    );
    return rc;
}

#endif // __SYSCALL_H_
