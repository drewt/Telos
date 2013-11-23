/*  Copyright 2013 Drew Thoresonoreson
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

#ifndef _KERNEL_DEVICE_H_
#define _KERNEL_DEVICE_H_

#include <stdarg.h>

#define DT_SIZE 16

#define IO_INCOMPLETE (-1)

enum {
	DEV_KBD,
	DEV_KBD_ECHO,
	DEV_CONSOLE_0,
	DEV_CONSOLE_1,
	DEV_SERIAL
};

struct device_operations;

struct device {
	int dvnum;
	char *dvname;
	void *dvioblk;
	struct device_operations *dv_op;
};

struct device_operations {
	int(*dvinit)(void);
	int(*dvopen)(dev_t devno);
	int(*dvclose)(dev_t devno);
	int(*dvread)(int fd, void *buf, int nbytes);
	int(*dvwrite)(int fd, void *buf, int nbytes);
	int(*dvioctl)(int fd, unsigned long request, va_list vargs);
	void(*dviint)(void);
	void(*dvoint)(void);
};

extern struct device devtab[DT_SIZE];

#endif
