/*  Copyright 2013 Drew Thoreson
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

#include <kernel/bitops.h>
#include <kernel/dispatch.h>
#include <kernel/interrupt.h>
#include <kernel/list.h>
#include <syscall.h>

/* routines defined in other files */
extern void tick(void);

const int NR_SYSCALLS = SYSCALL_MAX;

typedef long(*syscall_t)();
syscall_t systab[SYSCALL_MAX] = {
	[SYS_EXECVE]        = sys_execve,
	[SYS_FORK]          = sys_fork,
	[SYS_YIELD]         = sys_yield,
	[SYS_EXIT]          = sys_exit,
	[SYS_WAITID]        = sys_waitid,
	[SYS_GETPID]        = sys_getpid,
	[SYS_GETPPID]       = sys_getppid,
	[SYS_SLEEP]         = sys_sleep,
	[SYS_SIGRETURN]     = sig_restore,
	[SYS_KILL]          = sys_kill,
	[SYS_SIGQUEUE]      = sys_sigqueue,
	[SYS_SIGWAIT]       = sys_sigwait,
	[SYS_SIGSUSPEND]    = sys_sigsuspend,
	[SYS_SIGACTION]     = sys_sigaction,
	[SYS_SIGMASK]       = sys_sigprocmask,
	[SYS_OPEN]          = sys_open,
	[SYS_CLOSE]         = sys_close,
	[SYS_READ]          = sys_read,
	[SYS_PREAD]         = sys_pread,
	[SYS_WRITE]         = sys_write,
	[SYS_LSEEK]         = sys_lseek,
	[SYS_READDIR]       = sys_readdir,
	[SYS_IOCTL]         = sys_ioctl,
	[SYS_MKNOD]         = sys_mknod,
	[SYS_MKDIR]         = sys_mkdir,
	[SYS_RMDIR]         = sys_rmdir,
	[SYS_CHDIR]         = sys_chdir,
	[SYS_LINK]          = sys_link,
	[SYS_UNLINK]        = sys_unlink,
	[SYS_RENAME]        = sys_rename,
	[SYS_MOUNT]         = sys_mount,
	[SYS_UMOUNT]        = sys_umount,
	[SYS_STAT]          = sys_stat,
	[SYS_TRUNCATE]      = sys_truncate,
	[SYS_FCNTL]         = sys_fcntl,
	[SYS_ALARM]         = sys_alarm,
	[SYS_TIMER_CREATE]  = sys_timer_create,
	[SYS_TIMER_DELETE]  = sys_timer_delete,
	[SYS_TIMER_GETTIME] = sys_timer_gettime,
	[SYS_TIMER_SETTIME] = sys_timer_settime,
	[SYS_TIME]          = sys_time,
	[SYS_CLOCK_GETRES]  = sys_clock_getres,
	[SYS_CLOCK_GETTIME] = sys_clock_gettime,
	[SYS_CLOCK_SETTIME] = sys_clock_settime,
	[SYS_SBRK]          = sys_sbrk,
	[SYS_MMAP]          = sys_mmap,
};
