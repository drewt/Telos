/* device.h : devices
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

#ifndef __DEVICE_H_
#define __DEVICE_H_

#include <stdarg.h>

#define DT_SIZE 16

#define IO_INCOMPLETE (-1)

enum dev_id {
    DEV_KBD,
    DEV_KBD_ECHO,
    DEV_CONSOLE_0,
    DEV_CONSOLE_1,
};

struct device {
    int dvnum;
    char *dvname;
    int (*dvinit)(void);
    int (*dvopen)(enum dev_id devno);
    int (*dvclose)(enum dev_id devno);
    int (*dvread)(int fd, void *buf, int nbytes);
    int (*dvwrite)(int fd, void *buf, int nbytes);
    int (*dvioctl)(int fd, unsigned long request, va_list vargs);
    void (*dviint)(void);
    void (*dvoint)(void);
    void *dvioblk;
};

extern struct device devtab[DT_SIZE];

#endif // __DEVICE_H_
