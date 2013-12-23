/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of the Telos C Library.
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <syscall.h>

int errno;

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void free(void *ptr)
{
	/* TODO */
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *malloc(size_t size)
{
	/* TODO */
	return NULL;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *calloc(size_t nmemb, size_t size)
{
	/* TODO */
	return NULL;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
void *realloc(void *ptr, size_t size)
{
	/* TODO */
	return NULL;
}

void *sbrk(long increment)
{
	unsigned long old;
	long rc;

	if ((rc = syscall2(SYS_SBRK, (void*) increment, &old)) < 0) {
		errno = -rc;
		return (void*) -1;
	}

	return (void*) old;
}
