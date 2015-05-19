/*  Copyright 2013-2015 Drew Thoreson
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

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/string.h>

/*
 * fs-independent mount-flags
 */
enum {
	MS_RDONLY  = 0x01, // mount read-only
	MS_NOSUID  = 0x02, // ignore suid and sgid bits
	MS_NODEV   = 0x04, // disallow access to device special files
	MS_NOEXEC  = 0x08, // disallow program execution
	MS_SYNC    = 0x10, // writes are synced at once
	MS_REMOUNT = 0x20, // alter flags of a mounted FS
	MS_MEMFS   = 0x40, // do not free inodes

	MS_MOUNT_MASK = MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC
		      | MS_SYNC,
};

struct mount {
	const struct _String dev;
	const struct _String dir;
	const struct _String type;
	unsigned long flags;
	const void *data;
};

int mount(const char *dev_name, const char *dir_name, const char *type,
		unsigned long new_flags, const void *data);
int umount(const char *target);

#endif
