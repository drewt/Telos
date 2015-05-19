/*  Copyright 2013-2015 Drew Thoreson.
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

char *itoa_16 (unsigned int val, char *buf)
{
	char *ptr, *rc;
	rc = ptr = buf;

	do {
		*ptr++ = "0123456789abcdef"[val & 0xF];
		val >>= 4;
	} while (val);
	*ptr-- = '\0';

	while (buf < ptr) {
		char tmp = *buf;
		*buf++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

char *itoa_2 (unsigned int val, char *buf)
{
	char *ptr, *rc;
	rc = ptr = buf;

	do {
		*ptr++ = val & 1 ? '1' : '0';
		val >>= 1;
	} while (val);
	*ptr-- = '\0';

	while (buf < ptr) {
		char tmp = *buf;
		*buf++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

/* swiped from osdev.org wiki */
/*-----------------------------------------------------------------------------
 * Converts an integer in a given base to a string */
//-----------------------------------------------------------------------------
char *itoa (int val, char *str, int base)
{
	char *rc, *ptr, *low;
    
	/* check for supported base */
	if ( base < 2 || base > 36 ) {
		*str = '\0';
		return str;
	}

	rc = ptr = str;

	/* set '-' for negative decimals */
	if ( val < 0 && base == 10 )
		*ptr++ = '-';

	/* remember where the numbers start */
	low = ptr;

	/* the actual conversion */
	do {
		/* modulo is negative for negative value; this makes abs() unnecessary */
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmno"
				"pqrstuvwxyz"[35 + val % base];
		val /= base;
	} while ( val );

	/* terminate the string */
	*ptr-- = '\0';

	/* invert the numbers */
	while ( low < ptr ) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}
