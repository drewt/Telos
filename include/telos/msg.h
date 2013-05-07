/* msg.h : message passing IPC
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

#ifndef __TELOS_MSG_H_
#define __TELOS_MSG_H_

#include <sys/types.h>

int send (pid_t dest_pid, void *obuf, int olen, void *ibuf, int ilen);
int recv (pid_t *src_pid, void *buffer, int length);
int reply (pid_t src_pid, void *buffer, int length);

#endif // __TELOS_MSG_H_
