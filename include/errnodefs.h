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

#ifndef _ERRNODEFS_H_
#define _ERRNODEFS_H_

enum errno_values {
	ENOSYS = 1,
	E2BIG,
	EACCES,
	EADDRINUSE,
	EADDRNOTAVAIL,
	EAFNOSUPPORT,
	EAGAIN,
	EALREADY,
	EBADF,
	EBADMSG,
	EBUSY,
	ECANCELED,
	ECHILD,
	ECONNABORTED,
	ECONNREFUSED,
	ECONNRESET,
	EDEADLK,
	EDESTADDRREQ,
	EDOM,
	EDQUOT,
	EEXIST,
	EFAULT,
	EFBIG,
	EHOSTUNREACH,
	EIDRM,
	EILSEQ,
	EINPROGRESS,
	EINTR,
	EINVAL,
	EIO,
	EISCONN,
	EISDIR,
	ELOOP,
	EMFILE,
	EMLINK,
	EMSGSIZE,
	EMULTIHOP,
	ENAMETOOLONG,
	ENETDOWN,
	ENETRESET,
	ENETUNREACH,
	ENFILE,
	ENOBUFS,
	ENODATA,
	ENODEV,
	ENOENT,
	ENOEXEC,
	ENOLCK,
	ENOLINK,
	ENOMEM,
	ENOMSG,
	ENOPROTOOPT,
	ENOSPC,
	ENOSR,
	ENOSTR,
	ENOTCONN,
	ENOTDIR,
	ENOTEMPTY,
	ENOTSOCK,
	ENOTSUP,
	ENOTTY,
	ENXIO,
	EOPNOTSUPP,
	EOVERFLOW,
	EPERM,
	EPIPE,
	EPROTO,
	EPROTONOSUPPORT,
	EPROTOTYPE,
	ERANGE,
	EROFS,
	ESPIPE,
	ESRCH,
	ESTALE,
	ETIME,
	ETIMEDOUT,
	ETXTBSY,
	EWOULDBLOCK,
	EXDEV
};

#endif
