/* devinit.c : device initialization
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

#include <kernel/common.h>
#include <kernel/device.h>
#include <kernel/drivers/kbd.h>
#include <kernel/drivers/console.h>

static int io_error () {
    return SYSERR;
}

struct device devtab[DT_SIZE] = {
    [DEV_KBD] = {
        .dvnum   = DEV_KBD,
        .dvname  = "Keyboard (no echo)",
        .dvinit  = kbd_init,
        .dvopen  = kbd_open,
        .dvclose = kbd_close,
        .dvread  = kbd_read,
        .dvwrite = io_error,
        .dvioctl = kbd_ioctl,
        .dviint  = kbd_interrupt,
        .dvoint  = NULL,
        .dvioblk = NULL
    },
    [DEV_KBD_ECHO] = {
        .dvnum   = DEV_KBD_ECHO,
        .dvname  = "Keyboard (echo)",
        .dvinit  = kbd_init,
        .dvopen  = kbd_open,
        .dvclose = kbd_close,
        .dvread  = kbd_read,
        .dvwrite = io_error,
        .dvioctl = kbd_ioctl,
        .dviint  = kbd_interrupt,
        .dvoint  = NULL,
        .dvioblk = NULL
    },
    [DEV_CONSOLE_0] = {
        .dvnum   = DEV_CONSOLE_0,
        .dvname  = "Console 0",
        .dvinit  = console_init,
        .dvopen  = console_open,
        .dvclose = console_close,
        .dvread  = io_error,
        .dvwrite = console_write,
        .dvioctl = console_ioctl,
        .dviint  = NULL,
        .dvoint  = NULL,
        .dvioblk = NULL
    },
    [DEV_CONSOLE_1] = {
        .dvnum   = DEV_CONSOLE_1,
        .dvname  = "Console 1",
        .dvinit  = console_init,
        .dvopen  = console_open,
        .dvclose = console_close,
        .dvread  = io_error,
        .dvwrite = console_write,
        .dvioctl = console_ioctl,
        .dviint  = NULL,
        .dvoint  = NULL,
        .dvioblk = NULL
    }
};

void dev_init (void) {
    devtab[DEV_KBD].dvinit ();
}
