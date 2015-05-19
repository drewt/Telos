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

#ifndef _SYS_MAJOR_H_
#define _SYS_MAJOR_H_

#define MAX_CHRDEV 32
#define MAX_BLKDEV 32

#ifndef makedev
#define makedev _MAKEDEV_DEFN
#endif
#ifndef major
#define major _MAJOR_DEFN
#endif
#ifndef minor
#define minor _MINOR_DEFN
#endif

enum {
	UNNAMED_MAJOR	= 0,
	MEM_MAJOR	= 1,
	FLOPPY_MAJOR	= 2,
	HD_MAJOR	= 3,
	TTY_MAJOR	= 4,
	MOD_MAJOR       = 5,
};

#endif
