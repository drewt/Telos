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

#include <kernel/common.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>

#include <string.h> // pseudo-fs hack

#define FD_VALID(fd) (fd >= 0 && fd < FDT_SIZE && current->fds[fd] != FD_NONE)

// TODO: replace with real dev filesystem
static const char *devmap[4] = {
    [DEV_KBD]       = "/dev/kbd",
    [DEV_KBD_ECHO]  = "/dev/kbd_echo",
    [DEV_CONSOLE_0] = "/dev/cons0",
    [DEV_CONSOLE_1] = "/dev/cons1"
};

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_open (const char *pathname, int flags, ...) {
    dev_t fd;
    int devno;

    // look up device corresponding to pathname
    for (devno = 0; devno < 4; devno++)
        if (!strcmp (pathname, devmap[devno]))
            break;
    if (devno == 4) {
        current->rc = -ENOENT;
        return;
    }

    if ((current->rc = devtab[devno].dvopen(devno)))
        return;

    for (fd = 0; fd < FDT_SIZE && current->fds[fd] != FD_NONE; fd++);
    if (fd == FDT_SIZE) {
        current->rc = -EMFILE;
    } else {
        current->fds[fd] = devno;
        current->rc = fd;
    }
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_close (int fd) {

    dev_t devno = current->fds[fd];
    if (!FD_VALID (fd)) {
        current->rc = -EBADF;
        return;
    }
    if (devtab[devno].dvclose (devno)) {
        current->rc = -EIO;
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
        current->rc = -EBADF;
        return;
    }

    current->rc = devtab[current->fds[fd]].dvread (fd, buf, nbyte);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_write (int fd, void *buf, int nbyte) {

    if (!FD_VALID (fd)) {
        current->rc = -EBADF;
        return;
    }

    current->rc = devtab[current->fds[fd]].dvwrite (fd, buf, nbyte);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_ioctl (int fd, unsigned long command, va_list vargs) {

    if (!FD_VALID (fd)) {
        current->rc = -EBADF;
        return;
    }

    current->rc = devtab[current->fds[fd]].dvioctl (fd, command, vargs);
}
