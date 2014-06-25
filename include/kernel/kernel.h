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

#ifndef _KERNEL_KERNEL_H_
#define _KERNEL_KERNEL_H_

struct multiboot_info;

/* GLOBAL DATA */
extern void(*dfl_sighandlers[])(int);

/* initialization procedures */
extern void isr_init(void); /* ctsw.c */

/* user.c */
extern void root_proc();
extern void idle_proc();

#endif
