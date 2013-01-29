/* io.c : device independent I/O routines
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
#include <kernel/device.h>

#include <errnodefs.h>

#define FD_VALID(fd) (fd >= 0 && fd < FDT_SIZE && current->fds[fd] != FD_NONE)

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_open (enum dev_id devno) {

    if (devno > DT_SIZE) {
        current->rc = ENODEV;
        return;
    }
    
    if (devtab[devno].dvopen(devno)) {
        current->rc = SYSERR;
        return;
    }

    int i;
    for (i = 0; i < FDT_SIZE && current->fds[i] != FD_NONE; i++);
    if (i == FDT_SIZE) {
        current->rc = EMFILE;
    } else {
        current->fds[i] = devno;
        current->rc = i;
    }
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_close (int fd) {

    enum dev_id devno = current->fds[fd];
    if (!FD_VALID (fd)) {
        current->rc = EBADF;
        return;
    }
    if (devtab[devno].dvclose (devno)) {
        current->rc = EIO;
        return;
    }

    current->fds[fd] = FD_NONE;
    current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_read (int fd, void *buf, int nbyte) {

    if (!FD_VALID (fd)) {
        current->rc = EBADF;
        return;
    }

    current->rc = devtab[current->fds[fd]].dvread (fd, buf, nbyte);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_write (int fd, void *buf, int nbyte) {

    if (!FD_VALID (fd)) {
        current->rc = EBADF;
        return;
    }

    current->rc = devtab[current->fds[fd]].dvwrite (fd, buf, nbyte);
}
