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

#ifndef _KERNEL_COMPILER_H_
#define _KERNEL_COMPILER_H_

#define __always_inline __attribute__((always_inline))

/*
 * Using __builtin_constant_p(x) to ignore cases where the return
 * value is always the same.  This idea is taken from a similar patch
 * written by Daniel Walker.
 */
#define likely(x)	(__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 1))
#define unlikely(x)	(__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 0))


#endif
