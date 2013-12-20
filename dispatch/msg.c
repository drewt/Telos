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

#include <kernel/common.h>
#include <kernel/dispatch.h>
#include <kernel/list.h>

#include <string.h>

/*-----------------------------------------------------------------------------
 * Sends a message to another process.  If ibuf is not NULL, then the sending
 * process will block until it receives a reply from the receiving process */
//-----------------------------------------------------------------------------
long sys_send(int dest_pid, void *obuf, int olen, void *ibuf, int ilen)
{
	int tmp;
	struct pcb *dest;

	if (olen <= 0 || dest_pid <= 0)
		return -EINVAL;

	tmp = PT_INDEX(dest_pid);
	if (proctab[tmp].pid != dest_pid)
		return -ESRCH;

	dest = &proctab[tmp];

	if (ibuf) {
		current->reply_blk = (struct pbuf)
				{ .buf = ibuf, .len = ilen, .id = dest_pid };
	}

	/* if dest is ready to receive from sending process... */
	if (dest->state == STATE_BLOCKED &&
			(!dest->pbuf.id || dest->pbuf.id == current->pid)) {

		if (!dest->pbuf.id)
			copy_to_userspace(dest->pgdir, dest->parg,
					&current->pid, sizeof(int));

		/* unblock receiver */
		list_del(&dest->chain);
		ready(dest);

		/* send message */
		tmp = (olen < dest->pbuf.len) ? olen : dest->pbuf.len;
		copy_through_userspace(dest->pgdir, current->pgdir, dest->pbuf.buf,
				obuf, tmp);
		dest->rc = tmp;

		/* if sender expects reply, block in receiver's reply queue */
		if (ibuf) {
			list_add_tail(&current->chain, &dest->repl_q);
			current->state = STATE_BLOCKED;
			new_process();
		}
	} else {
		/* block sender on receiver's recv queue */
		current->pbuf = (struct pbuf)
				{ .buf = obuf, .len = olen, .id = dest_pid };
		list_add_tail(&current->chain, &dest->send_q);
		current->state = STATE_BLOCKED;
		new_process();
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * Receives a message from another process */
//-----------------------------------------------------------------------------
long sys_recv(int *pid_ptr, void *buffer, int length)
{
	int tmp, src_pid;
	struct pcb *src = NULL;

	copy_from_userspace(current->pgdir, &src_pid, pid_ptr, sizeof(int));

	if (length <= 0 || src_pid < 0)
		return -EINVAL;

	if (src_pid == 0) {
		if (list_empty(&current->send_q)) {
			/* no process waiting to send: block */
			current->pbuf.buf = buffer;
			current->pbuf.len = length;
			current->pbuf.id  = 0;
			current->state = STATE_BLOCKED;
			current->parg = pid_ptr;
			new_process();
			return 0;
		} else {
			/* set *src_pid and pretend it was set all along... */
			src_pid = ((struct pcb*) current->send_q.next)->pid;
			copy_to_userspace(current->pgdir, pid_ptr, &src_pid, sizeof(int));
		}
	}

	tmp = PT_INDEX(src_pid);
	if (proctab[tmp].pid != src_pid)
		return -ESRCH;

	src = &proctab[tmp];

	/* block receiver if src isn't ready to send */
	if (src->state != STATE_BLOCKED || src->pbuf.id != current->pid) {
		current->pbuf = (struct pbuf)
				{ .buf = buffer, .len = length, .id = src->pid };
		current->state = STATE_BLOCKED;
		list_add_tail(&current->chain, &src->recv_q);
		new_process();
	} else {
		/* unblock sender if no-reply send, otherwise move to reply queue */
		list_del(&src->chain);
		if (!src->reply_blk.id)
			ready(src);
		else
			list_add_tail(&src->chain, &current->repl_q);

		/* receive message */
		tmp = (length < src->pbuf.len) ? length : src->pbuf.len;
		copy_through_userspace(current->pgdir, src->pgdir, buffer,
				src->pbuf.buf, tmp);
		return tmp;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * Sends a reply to another process */
//-----------------------------------------------------------------------------
long sys_reply(int src_pid, void *buffer, int length)
{
	int tmp;
	struct pcb *src;

	if (length < 0 || src_pid <= 0)
		return -EINVAL;

	tmp = PT_INDEX(src_pid);
	if (proctab[tmp].pid != src_pid)
		return -ESRCH;

	src = &proctab[tmp];

	/* it is an error to reply before a corresponding send */
	if (src->state != STATE_BLOCKED || src->reply_blk.id != current->pid)
		return -EBADMSG; // XXX: better errno for this?

	/* send reply */
	tmp = (length < src->reply_blk.len) ? length : src->reply_blk.len;
	copy_through_userspace(src->pgdir, current->pgdir, src->reply_blk.buf,
			buffer, tmp);
	src->rc = tmp;

	/* unblock sender */
	src->reply_blk.id = 0;
	list_del(&src->chain);
	ready(src);
	return tmp;
}
