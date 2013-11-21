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
	[DEV_KBD]	= "/dev/kbd",
	[DEV_KBD_ECHO]	= "/dev/kbd_echo",
	[DEV_CONSOLE_0]	= "/dev/cons0",
	[DEV_CONSOLE_1]	= "/dev/cons1"
};

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_open(const char *pathname, int flags, ...)
{
	dev_t fd;
	int devno;

	const char *path = (char*) kmap_tmp_range(current->pgdir,
			(ulong) pathname, 1024);

	/* look up device corresponding to pathname */
	for (devno = 0; devno < 4; devno++)
		if (!strcmp(path, devmap[devno]))
			break;

	kunmap_range((ulong) path, 1024);

	if (devno == 4) {
		current->rc = -ENOENT;
		return;
	}

	for (fd = 0; fd < FDT_SIZE && current->fds[fd] != FD_NONE; fd++);
	if (fd == FDT_SIZE) {
		current->rc = -ENFILE;
		return;
	}

	if (devtab[devno].dv_op->dvopen == NULL)
		return;
	if ((current->rc = devtab[devno].dv_op->dvopen(devno)))
		return;

	current->fds[fd] = devno;
	current->rc = fd;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_close(int fd)
{
	dev_t devno = current->fds[fd];
	if (!FD_VALID(fd)) {
		current->rc = -EBADF;
		return;
	}
	if (devtab[devno].dv_op->dvclose &&devtab[devno].dv_op->dvclose(devno)) {
		current->rc = -EIO;
		return;
	}

	current->fds[fd] = FD_NONE;
	current->rc = 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_read(int fd, void *buf, int nbyte)
{
	if (!FD_VALID(fd)) {
		current->rc = -EBADF;
		return;
	}

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	current->rc = dev_op->dvread ? dev_op->dvread(fd, buf, nbyte) : ENXIO;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_write(int fd, void *buf, int nbyte)
{
	if (!FD_VALID(fd)) {
		current->rc = -EBADF;
		return;
	}

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	current->rc = dev_op->dvwrite ? dev_op->dvwrite(fd, buf, nbyte) : ENXIO;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void sys_ioctl(int fd, ulong cmd, va_list vargs)
{
	if (!FD_VALID(fd)) {
		current->rc = -EBADF;
		return;
	}

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	current->rc = dev_op->dvioctl ? dev_op->dvioctl(fd, cmd, vargs) : ENXIO;
}
