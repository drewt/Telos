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

#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#include <sys/string.h>

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
