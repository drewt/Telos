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

#ifndef __KERNEL_H_
#define __KERNEL_H_

struct multiboot_info;

/* GLOBAL DATA */
extern void(*dfl_sighandlers[])(int);

/* initialization procedures */
extern void dev_init (void); /* devinit.c */
extern void isr_init (void); /* ctsw.c */
extern int paging_init (struct multiboot_info*);

/* user.c */
extern void root_proc ();
extern void idle_proc ();

#endif // __KERNEL_H_
