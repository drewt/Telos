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

struct pcb;
struct sigaction;

extern struct pcb *current;
extern struct pcb *ready_head;
extern struct pcb *ready_tail;

extern int idle_pid;
extern int root_pid;

void print_ready_queue (void);

void dispatch_init (void);
void dispatch (void);
unsigned int context_switch (struct pcb *p);
struct pcb *next (void);
void ready (struct pcb *p);
void new_process (void);
int sq_rm (struct pcb *p);

int send_signal (int pid, int sig_no);

void tick (void);

int sys_create (void (*func)(void*), void *arg);
void sys_yield (void);
void sys_stop (void);
void sys_getpid (void);
void sys_puts (char *s);
void sys_report (char *s);
void sys_sleep (unsigned int milliseconds);
void sys_alarm (unsigned int seconds);
void sig_restore (void *osp);
void sys_sigaction (int sig, struct sigaction *act,
        struct sigaction *oact);
void sys_signal (int sig, void(*func)(int));
void sys_sigprocmask (int how, uint32_t *set, uint32_t *oset);
void sys_kill (int pid, int sig_no);
void sys_sigwait (void);
void sys_send (int dest_pid, void *obuf, int olen, void *ibuf, int ilen);
void sys_recv (int src_pid, void *buffer, int length);
void sys_reply (int src_pid, void *buffer, int length);
void sys_open (enum dev_id devno);
void sys_close (int fd);
void sys_read (int fd, void *buf, int nbyte);
void sys_write (int fd, void *buf, int nbyte);

#endif // __DISPATCH_H_
