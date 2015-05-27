/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <stddef.h>
#include <sys/type_defs.h>

#ifndef _DIV_T_DEFINED
#define _DIV_T_DEFINED
//typedef _DIV_T_TYPE div_t;
#endif
#ifndef _LDIV_T_DEFINED
#define _LDIV_T_DEFINED
//typedef _LDIV_T_TYPE ldiv_t;
#endif

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

void malloc_init(void);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void free(void *ptr);
_Noreturn void exit(int status);

int atoi(const char *nptr);

#endif
