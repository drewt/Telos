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

enum dev_id {
    DEV_KBD,
    DEV_KBD_ECHO,
    CONSOLE_0,
    CONSOLE_1
};

#define IO_INCOMPLETE (-1)

struct device {
    int dvnum;
    char *dvname;
    int (*dvopen)(enum dev_id devno);
    int (*dvclose)(enum dev_id devno);
    int (*dvread)(void *buf, int nbytes);
    int (*dvwrite)(void *buf, int nbytes);
    int (*dvioctl)(unsigned long request, va_list vargs);
    void (*dviint)(void);
    void (*dvoint)(void);
    void *dvioblk;
};

extern struct device devtab[DT_SIZE];

#endif // __DEVICE_H_
