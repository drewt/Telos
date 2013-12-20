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

#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/device.h>
#include <kernel/drivers/serial.h>

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_init(void)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_read(int fd, void *buf, int buf_len)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_write(int fd, void *buf, int buf_len)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_open(dev_t dev)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_close(dev_t dev)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int serial_ioctl(int fd, unsigned long command, va_list vargs)
{
	return 0;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void serial_int(void)
{

}
