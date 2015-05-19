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

#include <kernel/fs.h>
#include <string.h>

#define NR_FILESYSTEMS 8

struct file_system_type file_systems[NR_FILESYSTEMS];

int register_filesystem(
	struct super_block *(*read_super)(struct super_block*, void*, int),
	char *name, int requires_dev)
{
	for (int i = 0; i < NR_FILESYSTEMS; i++) {
		if (!file_systems[i].read_super) {
			file_systems[i].read_super = read_super;
			file_systems[i].name = name;
			file_systems[i].requires_dev = requires_dev;
			return 0;
		}
	}
	return -1;
}

struct file_system_type *get_fs_type(const char *name)
{
	for (int i = 0; i < NR_FILESYSTEMS; i++)
		if (file_systems[i].read_super && !strcmp(file_systems[i].name, name))
			return &file_systems[i];
	return NULL;
}
