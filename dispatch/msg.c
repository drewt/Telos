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

#include <kernel/common.h>
#include <kernel/dispatch.h>
#include <kernel/list.h>

#include <string.h>

/*-----------------------------------------------------------------------------
 * Sends a message to another process.  If ibuf is not NULL, then the sending
 * process will block until it receives a reply from the receiving process */
//-----------------------------------------------------------------------------
void sys_send (int dest_pid, void *obuf, int olen, void *ibuf, int ilen) {

    int tmp;
    struct pcb *dest;

    if (olen <= 0 || dest_pid <= 0) {
        current->rc = -(EINVAL);
        return;
    }

    tmp = PT_INDEX (dest_pid);
    if (proctab[tmp].pid != dest_pid) {
        current->rc = -(ESRCH);
        return;
    }

    dest = &proctab[tmp];

    if (ibuf) {
        current->reply_blk = (struct pbuf)
            { .buf = ibuf, .len = ilen, .id = dest_pid };
    }

    // if dest is ready to receive from sending process...
    if (dest->state == STATE_BLOCKED &&
            (!dest->pbuf.id || dest->pbuf.id == current->pid)) {

        if (!dest->pbuf.id)
            *((int*) dest->parg) = current->pid;

        // unblock receiver
        list_remove (&current->recv_q, (list_entry_t) dest);
        ready (dest);

        // send message
        tmp = (olen < dest->pbuf.len) ? olen : dest->pbuf.len;
        copy_through_userspace (dest->pgdir, current->pgdir, dest->pbuf.buf,
                obuf, tmp);
        dest->rc = tmp;

        // if sender expects reply, block in receiver's reply queue
        if (ibuf) {
            enqueue (&dest->repl_q, (list_entry_t) current);
            current->state = STATE_BLOCKED;
            new_process ();
        }
    } else {
        // block sender on receiver's recv queue
        current->pbuf = (struct pbuf)
            { .buf = obuf, .len = olen, .id = dest_pid };
        enqueue (&dest->send_q, (list_entry_t) current);
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
        current->rc = -(EINVAL);
        return;
    }

    if (*src_pid == 0) {
        if (list_empty (&current->send_q)) {
            /* no process waiting to send: block */
            current->pbuf.buf = buffer;
            current->pbuf.len = length;
            current->pbuf.id  = 0;
            current->state = STATE_BLOCKED;
            current->parg = src_pid;
            new_process ();
            return;
        } else {
            /* set *src_pid and pretend it was set all along... */
            *src_pid = ((struct pcb*) list_first (&current->send_q))->pid;
        }
    }

    tmp = PT_INDEX (*src_pid);
    if (proctab[tmp].pid != *src_pid) {
        current->rc = -(ESRCH);
        return;
    }

    src = &proctab[tmp];

    // block receiver if src isn't ready to send
    if (src->state != STATE_BLOCKED || src->pbuf.id != current->pid) {
        current->pbuf = (struct pbuf)
            { .buf = buffer, .len = length, .id = src->pid };
        current->state = STATE_BLOCKED;
        enqueue (&src->recv_q, (list_entry_t) current);
        new_process ();
    } else {
        // unblock sender if no-reply send, otherwise move to reply queue
        list_remove (&current->send_q, (list_entry_t) src);
        if (!src->reply_blk.id)
            ready (src);
        else
            enqueue (&current->repl_q, (list_entry_t) src);

        // receive message
        tmp = (length < src->pbuf.len) ? length : src->pbuf.len;
        copy_through_userspace (current->pgdir, src->pgdir, buffer,
                src->pbuf.buf, tmp);
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
        current->rc = -(EINVAL);
        return;
    }

    tmp = PT_INDEX (src_pid);
    if (proctab[tmp].pid != src_pid) {
        current->rc = -(ESRCH);
        return;
    }

    src = &proctab[tmp];

    // it is an error to reply before a corresponding send
    if (src->state != STATE_BLOCKED || src->reply_blk.id != current->pid) {
        current->rc = -(EBADMSG); // XXX: better errno for this?
        return;
    }

    // send reply
    tmp = (length < src->reply_blk.len) ? length : src->reply_blk.len;
    copy_through_userspace (src->pgdir, current->pgdir, src->reply_blk.buf,
            buffer, tmp);
    //memcpy (src->reply_blk.buf, buffer, tmp);
    current->rc = tmp;
    src->rc = tmp;

    // unblock sender
    src->reply_blk.id = 0;
    list_remove (&current->repl_q, (list_entry_t) src);
    ready (src);
}
