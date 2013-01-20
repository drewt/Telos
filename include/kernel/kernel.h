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

/* GLOBAL DATA */
extern unsigned int stack;
extern unsigned int kstart;
extern unsigned int kend;
extern void(*dfl_sighandlers[])(int);

/* initialization procedures */
extern void mem_init (void); /* mem.c */
extern void isr_init (void); /* ctsw.c */
extern void paging_init (void);

/* user.c */
extern void root_proc (void *arg);
extern void idle_proc (void *arg);

#endif // __KERNEL_H_
