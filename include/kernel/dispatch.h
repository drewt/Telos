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

#ifndef _KERNEL_DISPATCH_H_
#define _KERNEL_DISPATCH_H_

#include <kernel/process.h>
#include <telos/wait.h>
#include <syscall.h>

struct sigaction;
struct sigevent;

struct timespec;
struct itimerspec;

struct stat;
struct mount;
struct exec_args;
struct __mmap_args;

struct dirent;

struct ucontext;

extern int idle_pid;

void __kill(struct pcb *p, int sig_no, int code);

static inline int verify_user_string(const char *str, size_t len)
{
	if (vm_verify(&current->mm, str, len, 0))
		return -EFAULT;
	if (str[len] != '\0')
		return -EINVAL;
	return 0;
}

struct vma *get_heap(struct mm_struct *mm);

void exn_page_fault(void);
void exn_fpe(void);
void exn_ill_instr(void);

void int_keyboard(void);

/* syscall routines */
long sys_sbrk(long inc, ulong *oldbrk);
long sys_mmap(struct __mmap_args *args);
long sys_munmap(void *addr, size_t len);
long sys_execve(struct exec_args *args);
long sys_fork(void);
long sys_yield(void);
long sys_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
long sys_exit(int status);
long sys_getpid(void);
long sys_getppid(void);
long sys_sleep(unsigned long ms);
long sys_alarm(unsigned long ms);
long sig_restore(struct ucontext *cx);
long sys_sigaction(int sig, struct sigaction *act,
		struct sigaction *oact);
long sys_sigprocmask(int how, sigset_t *set, sigset_t *oset);
long sys_kill(pid_t pid, int sig);
long sys_sigqueue(pid_t pid, int sig, const union sigval value);
long sys_sigwait(sigset_t set, int *sig);
long sys_sigsuspend(sigset_t mask);
long sys_open(const char *pathname, size_t name_len, int flags, int mode);
long sys_close(unsigned int fd);
long sys_read(unsigned int fd, char *buf, size_t nbyte);
long sys_pread(unsigned int fd, char *buf, size_t nbyte, unsigned long pos);
long sys_write(unsigned int fd, char *buf, size_t nbyte);
long sys_pwrite(unsigned int fd, char *buf, size_t nbyte, unsigned long pos);
long sys_lseek(unsigned int fd, off_t offset, unsigned int whence);
long sys_readdir(unsigned int fd, struct dirent *dirent, unsigned int count);
long sys_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
long sys_mknod(const char *filename, size_t name_len, int mode, dev_t dev);
long sys_mkdir(const char *pathname, size_t name_len, int mode);
long sys_rmdir(const char *pathname, size_t name_len);
long sys_chdir(const char *pathname, size_t name_len);
long sys_link(const char *oldname, size_t oldname_len, const char *newname,
		size_t newname_len);
long sys_unlink(const char *pathname, size_t name_len);
long sys_rename(const char *oldname, size_t oldname_len, const char *newname,
		size_t newname_len);
long sys_mount(const struct mount *mount);
long sys_umount(const char *name, size_t name_len);
long sys_stat(const char *pathname, size_t name_len, struct stat *s);
long sys_truncate(const char *pathname, size_t name_len, size_t length);
long sys_fcntl(int fd, int cmd, int arg);
long sys_time(time_t *t);
long sys_clock_getres(clockid_t clockid, struct timespec *res);
long sys_clock_gettime(clockid_t clockid, struct timespec *tp);
long sys_clock_settime(clockid_t clockid, struct timespec *tp);
long sys_timer_create(clockid_t clockid, struct sigevent *sevp,
		timer_t *timerid);
long sys_timer_delete(timer_t timerid);
long sys_timer_gettime(timer_t timerid, struct itimerspec *curr_value);
long sys_timer_settime(timer_t timerid, int flags,
		const struct itimerspec *new_value,
		struct itimerspec *old_value);

#endif
