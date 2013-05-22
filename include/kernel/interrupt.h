/*  Copyright 2013 Drew Thoreson
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

#ifndef _KERNEL_INTERRUPT_H_
#define _KERNEL_INTERRUPT_H_

/* vectors into the IDT; also serve as vectors into the sysactions table */
#define DBZ_EXN         0x00
#define ILLOP_EXN       0x06
#define GP_EXN          0x0D
#define PF_EXN          0x0E
#define TIMER_INTR      0x20
#define KBD_INTR        0x21

#define FPE_EXN         DBZ_EXN
#define ILL_EXN         ILLOP_EXN
#define SYSCALL_INTR    0x80

#endif /* _KERNEL_INTERRUPT_H_ */
