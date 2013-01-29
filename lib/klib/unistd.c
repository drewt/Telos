/*  Copyright 2013 Drew T.
 *
 *  This file is part of the Telos C Library.
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

#include <unistd.h>
#include <syscall.h>

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
unsigned int alarm (unsigned int seconds) {
    return syscall1 (SYS_ALARM, (void*) seconds);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
unsigned int sleep (unsigned int seconds) {
    return syscall1 (SYS_SLEEP, (void*) seconds);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
pid_t getpid (void) {
    return syscall0 (SYS_GETPID);
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int open (const char *pathname, int flags, ...) {
    int rv = syscall2 (SYS_OPEN, (void*) pathname, (void*) flags);
    if (rv < 0)
        return -1;
    return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
int close (int fd) {
    int rv = syscall1 (SYS_CLOSE, (void*) fd);
    if (rv < 0)
        return -1;
    return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t read (int fd, void *buf, size_t nbyte) {
    int rv = syscall3 (SYS_READ, (void*) fd, buf, (void*) nbyte);
    if (rv < 0)
        return -1;
    return rv;
}

/*-----------------------------------------------------------------------------
 * */
//-----------------------------------------------------------------------------
ssize_t write (int fd, const void *buf, size_t nbyte) {
    int rv = syscall3 (SYS_WRITE, (void*) fd, (void*) buf, (void*) nbyte);
    if (rv < 0)
        return -1;
    return rv;
}
