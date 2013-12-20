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
long sys_open(const char *pathname, int flags, ...)
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

	if (devno == 4)
		return -ENOENT;

	for (fd = 0; fd < FDT_SIZE && current->fds[fd] != FD_NONE; fd++);
	if (fd == FDT_SIZE)
		return -ENFILE;

	if (devtab[devno].dv_op->dvopen == NULL)
		return -1; /* FIXME */
	if ((current->rc = devtab[devno].dv_op->dvopen(devno)))
		return -1; /* FIXME */

	current->fds[fd] = devno;
	return fd;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
long sys_close(int fd)
{
	dev_t devno = current->fds[fd];
	if (!FD_VALID(fd))
		return -EBADF;
	if (devtab[devno].dv_op->dvclose &&devtab[devno].dv_op->dvclose(devno))
		return -EIO;

	current->fds[fd] = FD_NONE;
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
long sys_read(int fd, void *buf, int nbyte)
{
	if (!FD_VALID(fd))
		return -EBADF;

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	return dev_op->dvread ? dev_op->dvread(fd, buf, nbyte) : ENXIO;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
long sys_write(int fd, void *buf, int nbyte)
{
	if (!FD_VALID(fd))
		return -EBADF;

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	return dev_op->dvwrite ? dev_op->dvwrite(fd, buf, nbyte) : ENXIO;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
long sys_ioctl(int fd, ulong cmd, va_list vargs)
{
	if (!FD_VALID(fd))
		return -EBADF;

	struct device_operations *dev_op = devtab[current->fds[fd]].dv_op;
	return dev_op->dvioctl ? dev_op->dvioctl(fd, cmd, vargs) : ENXIO;
}
