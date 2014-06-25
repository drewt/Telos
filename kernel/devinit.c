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

#include <kernel/device.h>
#include <kernel/drivers/console.h>
#include <kernel/drivers/serial.h>

struct device devtab[DT_SIZE] = {
	[DEV_CONSOLE_0] = {
		.dvnum   = DEV_CONSOLE_0,
		.dvname  = "Console 0",
		.dvioblk = NULL,
		.dv_op = &console_operations
	},
	[DEV_CONSOLE_1] = {
		.dvnum   = DEV_CONSOLE_1,
		.dvname  = "Console 1",
		.dvioblk = NULL,
		.dv_op = &console_operations
	},
	[DEV_SERIAL] = {
		.dvnum   = DEV_SERIAL,
		.dvname  = "Serial port",
		.dvioblk = NULL,
		.dv_op = NULL
	}
};

static void driver_init(void)
{
	devtab[DEV_CONSOLE_0].dv_op->dvinit();
}
EXPORT_KINIT(driver, SUB_DRIVER, driver_init);
