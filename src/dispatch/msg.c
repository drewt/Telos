/* msg.c : message passing IPC
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

#include <kernel/dispatch.h>

#include <klib.h>

/*-----------------------------------------------------------------------------
 * Sends a message to another process.  If ibuf is not NULL, then the sending
 * process will block until it receives a reply from the receiving process */
//-----------------------------------------------------------------------------
void sys_send (int dest_pid, void *obuf, int olen, void *ibuf, int ilen) {

    int tmp;
    struct pcb *dest;

    if (olen <= 0 || dest_pid <= 0) {
        current->rc = SYSERR;
        return;
    }

    tmp = PT_INDEX (dest_pid);
    if (proctab[tmp].pid != dest_pid) {
        current->rc = SYSERR;
        return;
    }

    dest = &proctab[tmp];

    if (ibuf) {
        current->reply_blk = (struct msg)
            { .buf = ibuf, .len = ilen, .pid = dest_pid };
    }

    // if dest is ready to receive from sending process...
    if (dest->state == STATE_BLOCKED &&
            (!dest->msg.pid || dest->msg.pid == current->pid)) {

        if (!dest->msg.pid)
            *((int*) dest->parg) = current->pid;

        // unblock receiver
        proc_rm (&current->recv_q, dest);
        ready (dest);

        // send message
        tmp = (olen < dest->msg.len) ? olen : dest->msg.len;
        memcpy (dest->msg.buf, obuf, tmp);

        // if sender expects reply, block in receiver's reply queue
        if (ibuf) {
            proc_enqueue (&dest->repl_q, current);
            current->state = STATE_BLOCKED;
            new_process ();
        }
    } else {
        // block sender on receiver's recv queue
        current->msg = (struct msg)
            { .buf = obuf, .len = olen, .pid = dest_pid };
        proc_enqueue (&dest->send_q, current);
        current->state = STATE_BLOCKED;
        new_process ();
    }
}

/*-----------------------------------------------------------------------------
 * Receives a message from another process */
//-----------------------------------------------------------------------------
void sys_recv (int *src_pid, void *buffer, int length) {

    int tmp;
    struct pcb *src = NULL;

    if (length <= 0 || *src_pid < 0) {
        current->rc = SYSERR;
        return;
    }

    // block if recvall and no processes waiting to send
    if (!(*src_pid) && !(src = proc_peek (&current->send_q))) {
        current->msg = (struct msg)
            { .buf = buffer, .len = length, .pid = 0 };
        current->state = STATE_BLOCKED;
        current->parg = src_pid;
        new_process ();
        return;
    } else if (src) {
        *src_pid = src->pid; // just set src_pid and proceed as usual
    }

    tmp = PT_INDEX (*src_pid);
    if (proctab[tmp].pid != *src_pid) {
        current->rc = SYSERR;
        return;
    }

    src = &proctab[tmp];

    // block receiver if src isn't ready to send
    if (src->state != STATE_BLOCKED || src->msg.pid != current->pid) {
        current->msg = (struct msg)
            { .buf = buffer, .len = length, .pid = src->pid };
        current->state = STATE_BLOCKED;
        proc_enqueue (&src->recv_q, current);
        new_process ();
    } else {
        // unblock sender if no-reply send, otherwise move to reply queue
        proc_rm (&current->send_q, src);
        if (!src->reply_blk.pid)
            ready (src);
        else
            proc_enqueue (&current->repl_q, src);

        // receive message
        tmp = (length < src->msg.len) ? length : src->msg.len;
        memcpy (buffer, src->msg.buf, tmp);
        current->rc = tmp;
    }
}

/*-----------------------------------------------------------------------------
 * Sends a reply to another process */
//-----------------------------------------------------------------------------
void sys_reply (int src_pid, void *buffer, int length) {

    int tmp;
    struct pcb *src;

    if (length < 0 || src_pid <= 0) {
        current->rc = SYSERR;
        return;
    }

    tmp = PT_INDEX (src_pid);
    if (proctab[tmp].pid != src_pid) {
        current->rc = SYSERR;
        return;
    }

    src = &proctab[tmp];

    // it is an error to reply before a corresponding send
    if (src->state != STATE_BLOCKED || src->reply_blk.pid != current->pid) {
        current->rc = SYSERR;
        return;
    }

    // send reply
    tmp = (length < src->reply_blk.len) ? length : src->reply_blk.len;
    memcpy (src->reply_blk.buf, buffer, tmp);
    current->rc = tmp;
    src->rc = tmp;

    // unblock sender
    src->reply_blk.pid = 0;
    proc_rm (&current->repl_q, src);
    ready (src);
}
