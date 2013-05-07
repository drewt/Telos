/* process.h : process structures
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

#ifndef _KERNEL_PROCESS_H_
#define _KERNEL_PROCESS_H_

#include <signal.h>
#include <kernel/device.h>
#include <kernel/queue.h>

#define PT_SIZE  256
#define PID_MASK (PT_SIZE - 1)

#define PT_INDEX(pid) ((pid) & PID_MASK)

#define FDT_SIZE 8
#define FD_NONE  (DT_SIZE + 1)

/* process status codes */
enum proc_state {
    STATE_STOPPED    = 0,
    STATE_RUNNING    = 1 << 1,
    STATE_READY      = 1 << 2,
    STATE_BLOCKED    = 1 << 3,
    STATE_SIGWAIT    = 1 << 4,
    STATE_SIGSUSPEND = 1 << 5,
    STATE_SLEEPING   = 1 << 6
};

struct pbuf {
    void *buf; // buffer
    int  len;  // length of buffer
    int  id;   // id (context dependent)
};

/* process control block */
struct pcb {
    queue_chain_t chain;
    /* metadata */
    int          pid;                // process ID
    int          parent_pid;         // parent process's pid
    uint32_t     state;              // state
    unsigned int rc;                 // return value for system calls
    /* stacks */
    void         *stack_mem;         // beginning of stack memory
    void         *int_stack;         // stack for interrupts
    void         *esp;               // stack pointer
    void         *ifp;               // interrupt frame pointer
    /* time */
    unsigned int timestamp;          // creation time
    int          sleep_delta;        // delta for sleeping queue
    /* signals */
    struct sigaction sigactions[_TELOS_SIGMAX]; // signal handlers
    struct siginfo   siginfos[_TELOS_SIGMAX];   // signal information
    uint32_t     sig_pending;        // bitmask for pending signals
    uint32_t     sig_accept;         // bitmask for accepted signals
    uint32_t     sig_ignore;         // bitmask for ignored signals
    /* message passing IPC */
    struct pbuf  pbuf;               // saved buffer
    struct pbuf  reply_blk;
    queue_head_t  send_q;            // processes waiting to send
    queue_head_t  recv_q;            // processes waiting to receive
    queue_head_t  repl_q;            // processes waiting for a reply
    /* */
    void         *parg;              // pointer to... something
    enum dev_id  fds[FDT_SIZE];      // file descriptors
    struct mem_header *heap_mem;     // heap-allocated memory
};

extern struct pcb proctab[];
extern const struct sigaction default_sigactions[_TELOS_SIGMAX];

#endif /* _KERNEL_PROCESS_H_ */
