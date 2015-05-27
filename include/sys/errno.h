/*  Copyright 2013-2015 Drew Thoreson
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

#ifndef _SYS_ERRNO_H_
#define _SYS_ERRNO_H_

enum {
	ENOSYS          = 1,
	E2BIG           = 2,
	EACCES          = 3,
	EADDRINUSE      = 4,
	EADDRNOTAVAIL   = 5,
	EAFNOSUPPORT    = 6,
	EAGAIN          = 7,
	EALREADY        = 8,
	EBADF           = 9,
	EBADMSG         = 10,
	EBUSY           = 11,
	ECANCELED       = 12,
	ECHILD          = 13,
	ECONNABORTED    = 14,
	ECONNREFUSED    = 15,
	ECONNRESET      = 16,
	EDEADLK         = 17,
	EDESTADDRREQ    = 18,
	EDOM            = 19,
	EDQUOT          = 20,
	EEXIST          = 21,
	EFAULT          = 22,
	EFBIG           = 23,
	EHOSTUNREACH    = 24,
	EIDRM           = 25,
	EILSEQ          = 26,
	EINPROGRESS     = 27,
	EINTR           = 28,
	EINVAL          = 29,
	EIO             = 30,
	EISCONN         = 31,
	EISDIR          = 32,
	ELOOP           = 33,
	EMFILE          = 34,
	EMLINK          = 35,
	EMSGSIZE        = 36,
	EMULTIHOP       = 37,
	ENAMETOOLONG    = 38,
	ENETDOWN        = 39,
	ENETRESET       = 40,
	ENETUNREACH     = 41,
	ENFILE          = 42,
	ENOBUFS         = 43,
	ENODATA         = 44,
	ENODEV          = 45,
	ENOENT          = 46,
	ENOEXEC         = 47,
	ENOLCK          = 48,
	ENOLINK         = 49,
	ENOMEM          = 50,
	ENOMSG          = 60,
	ENOPROTOOPT     = 61,
	ENOSPC          = 62,
	ENOSR           = 63,
	ENOSTR          = 64,
	ENOTCONN        = 65,
	ENOTDIR         = 66,
	ENOTEMPTY       = 67,
	ENOTSOCK        = 68,
	ENOTSUP         = 69,
	ENOTTY          = 70,
	ENXIO           = 71,
	EOPNOTSUPP      = 72,
	EOVERFLOW       = 73,
	EPERM           = 74,
	EPIPE           = 75,
	EPROTO          = 76,
	EPROTONOSUPPORT = 77,
	EPROTOTYPE      = 78,
	ERANGE          = 79,
	EROFS           = 80,
	ESPIPE          = 81,
	ESRCH           = 82,
	ESTALE          = 83,
	ETIME           = 84,
	ETIMEDOUT       = 85,
	ETXTBSY         = 86,
	EWOULDBLOCK     = 87,
	EXDEV           = 89,
	ENOTBLK         = 90,
	EFTYPE          = 91,
};

#endif
