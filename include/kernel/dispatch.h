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

#ifndef _KERNEL_DISPATCH_H_
#define _KERNEL_DISPATCH_H_

#include <kernel/process.h>

struct sigaction;
struct sigevent;

struct timespec;
struct itimerspec;

extern struct pcb *current;

extern int idle_pid;
extern int root_pid;

void dispatch_init(void);
void dispatch(void);
void ready(struct pcb *p);
void new_process(void);

void __kill(struct pcb *p, int sig_no);

ulong virt_to_phys(pmap_t pgdir, ulong addr);
int copy_to_user(struct pcb *p, void *dst, const void *src, size_t len);
int copy_from_user(struct pcb *p, void *dst, const void *src, size_t len);
int copy_through_user(struct pcb *dst_p, struct pcb *src_p, void *dst,
		const void *src, size_t len);
int copy_string_through_user(struct pcb *dst_p, struct pcb *src_p, void *dst,
		const void *src, size_t len);
ulong kmap_tmp_range(pmap_t pgdir, ulong addr, size_t len);
void kunmap_range(ulong addr, size_t len);

#define copy_to_current(dst, src, len) \
	copy_to_user(current, dst, src, len)

#define copy_from_current(dst, src, len) \
	copy_from_user(current, dst, src, len)

void exn_page_fault(void);
void exn_fpe(void);
void exn_ill_instr(void);

/* service routines */
long sys_malloc(unsigned int size, void **p);
long sys_free(void *ptr);
long sys_palloc(void **p);
long sys_create(void(*func)(int,char*), int argc, char **argv);
long sys_yield(void);
long sys_exit(int status);
long sys_getpid(void);
long sys_puts(char *s);
long sys_report(char *s);
long sys_sleep(unsigned long ms);
long sys_alarm(unsigned long ms);
long sig_restore(void *osp);
long sys_sigaction(int sig, struct sigaction *act,
		struct sigaction *oact);
long sys_signal(int sig, void(*func)(int));
long sys_sigprocmask(int how, sigset_t *set, sigset_t *oset);
long sys_kill(pid_t pid, int sig);
long sys_sigqueue(pid_t pid, int sig, const union sigval value);
long sys_sigwait(void);
long sys_send(int dest_pid, void *obuf, int olen, void *ibuf, int ilen);
long sys_recv(int *src_pid, void *buffer, int length);
long sys_reply(int src_pid, void *buffer, int length);
long sys_open(const char *pathname, int flags, ...);
long sys_close(int fd);
long sys_read(int fd, void *buf, int nbyte);
long sys_write(int fd, void *buf, int nbyte);
long sys_ioctl(int fd, unsigned long command, va_list vargs);
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
