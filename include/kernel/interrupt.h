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
#define EXN_DBZ		0x00
#define EXN_ILLOP	0x06
#define EXN_GP		0x0D
#define EXN_PF		0x0E
#define INTR_TIMER	0x20
#define INTR_KBD	0x21

#define EXN_FPE		EXN_DBZ
#define EXN_ILL		EXN_ILLOP
#define INTR_SYSCALL	0x80

#endif
