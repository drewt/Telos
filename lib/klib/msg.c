/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C Library.
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

#include <syscall.h>
#include <telos/msg.h>

/*-----------------------------------------------------------------------------
 * */ 
//-----------------------------------------------------------------------------
int send(pid_t pid, void *obuf, int olen, void *ibuf, int ilen)
{
	return syscall5(SYS_SEND, (void*) pid, obuf, (void*) olen, ibuf,
			(void*) ilen);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int recv(pid_t *pid, void *buffer, int length)
{
	return syscall3(SYS_RECV, (void*) pid, buffer, (void*) length);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int reply (pid_t pid, void *buffer, int length)
{
	return syscall3(SYS_REPLY, (void*) pid, buffer, (void*) length);
}
