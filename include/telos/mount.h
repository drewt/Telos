/* Copyright (c) 2013-2015, Drew Thoreson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _TELOS_MOUNT_H_
#define _TELOS_MOUNT_H_

#include <telos/string.h>

/*
 * fs-independent mount-flags
 */
enum {
	MS_RDONLY  = 0x01, // mount read-only
	MS_NOSUID  = 0x02, // ignore suid and sgid bits
	MS_NODEV   = 0x04, // disallow access to device special files
	MS_NOEXEC  = 0x08, // disallow program execution
	MS_SYNC    = 0x10, // writes are synced at once
	MS_REMOUNT = 0x20, // alter flags of a mounted FS
	MS_MEMFS   = 0x40, // do not free inodes

	MS_MOUNT_MASK = MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC
		      | MS_SYNC,
};

struct mount {
	const struct _Telos_string dev;
	const struct _Telos_string dir;
	const struct _Telos_string type;
	unsigned long flags;
	const void *data;
};

#endif
