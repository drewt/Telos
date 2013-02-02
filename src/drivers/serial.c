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
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/drivers/serial.h>

#include <errnodefs.h>

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_init (void) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_read (int fd, void *buf, int buf_len) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_write (int fd, void *buf, int buf_len) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_open (enum dev_id devno) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_close (enum dev_id devno) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_ioctl (int fd, unsigned long command, va_list vargs) {
    return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void serial_int (void) {

}
