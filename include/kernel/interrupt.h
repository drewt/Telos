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

#ifndef __INTERRUPT_H_
#define __INTERRUPT_H_

/* vectors into the IDT; also serve as vectors into the sysactions table */
enum int_vectors {
    TIMER_INTR   = 0x20,
    KBD_INTR     = 0x21,
    SYSCALL_INTR = 0x80
};

#endif // __INTERRUPT_H_
