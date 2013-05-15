/* dispatch.h : dispatcher 2.0
 */

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

#ifndef __DISPATCH_H_
#define __DISPATCH_H_

#include <kernel/common.h>
#include <kernel/process.h>

struct sigaction;

extern struct pcb *current;

extern int idle_pid;
extern int root_pid;

void dispatch_init (void);
void dispatch (void);
void ready (struct pcb *p);
void new_process (void);
int sq_rm (struct pcb *p);

/* service routines */
void sys_malloc (unsigned int size, void **p);
void sys_free (void *ptr);
void sys_palloc (void **p);
int sys_create (void (*func)(int,char*), int argc, char **argv);
void sys_yield (void);
void sys_exit (int status);
void sys_getpid (void);
void sys_puts (char *s);
void sys_report (char *s);
void sys_sleep (unsigned int milliseconds);
void sys_alarm (unsigned int seconds);
void sig_restore (void *osp);
void sys_sigaction (int sig, struct sigaction *act,
        struct sigaction *oact);
void sys_signal (int sig, void(*func)(int));
void sys_sigprocmask (int how, sigset_t *set, sigset_t *oset);
void sys_kill (int pid, int sig_no);
void sys_sigwait (void);
void sys_send (int dest_pid, void *obuf, int olen, void *ibuf, int ilen);
void sys_recv (int *src_pid, void *buffer, int length);
void sys_reply (int src_pid, void *buffer, int length);
void sys_open (const char *pathname, int flags, ...);
void sys_close (int fd);
void sys_read (int fd, void *buf, int nbyte);
void sys_write (int fd, void *buf, int nbyte);
void sys_ioctl (int fd, unsigned long command, va_list vargs);

#endif // __DISPATCH_H_
