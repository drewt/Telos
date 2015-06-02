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

#ifndef _TELOS_TERMIOS_H_
#define _TELOS_TERMIOS_H_

#define VEOF   0
#define VEOL   1
#define VERASE 2
#define VINTR  3
#define VKILL  4
#define VMIN   5
#define VQUIT  6
#define VSTART 7
#define VSTOP  8
#define VSUSP  9
#define VTIME  10
#define NCCS   11

/* input modes */
#define BRKINT 0x0001 /* signal interrupt on break */
#define ICRNL  0x0002 /* map CR to NL on input */
#define IGNBRK 0x0004 /* ignore break condition */
#define IGNCR  0x0008 /* ignore CR */
#define IGNPAR 0x0010 /* ignore characters with parity errors */
#define INLCR  0x0020 /* map NL to CR on input */
#define INPCK  0x0040 /* enable input parity check */
#define ISTRIP 0x0080 /* strip character */
#define IXANY  0x0100 /* enable any character to restart output */
#define IXOFF  0x0200 /* enable start/stop input control */
#define IXON   0x0400 /* enable start/stop output control */
#define PARMRK 0x0800 /* mark parity errors */

/* output modes */
#define OPOST  0x00000001 /* post-process output */
#define ONLCR  0x00000002 /* map NL to CR-NL on output */
#define OCRNL  0x00000004 /* map CR to NL on output */
#define ONOCR  0x00000008 /* no CR output at column 0 */
#define ONLRET 0x00000010 /* NL performs CR function */
#define OFDEL  0x00000020 /* fill is DEL */
#define OFILL  0x00000040 /* use fill characters for delay */
#define NLDLY  0x00000080 /* select newline delays */
#define NL0    0x00000000
#define NL1    0x00000080
#define CRDLY  0x00000F00 /* select carriage-return delays */
#define CR0    0x00000100
#define CR1    0x00000200
#define CR2    0x00000400
#define CR3    0x00000800
#define TABDLY 0x0000F000 /* select horizontal-tab delays */
#define TAB0   0x00001000
#define TAB1   0x00002000
#define TAB2   0x00004000
#define TAB3   0x00008000
#define BSDLY  0x00030000 /* select backspace delays */
#define BS0    0x00010000
#define BS1    0x00020000
#define VTDLY  0x000C0000 /* select vertical-tab delays */
#define VT0    0x00040000
#define VT1    0x00080000
#define FFDLY  0x00300000 /* select form-feed delays */
#define FF0    0x00100000
#define FF1    0x00200000

/* baud rate selection */
#define B0     0
#define B50    1
#define B75    2
#define B110   3
#define B134   4
#define B150   5
#define B200   6
#define B300   7
#define B600   8
#define B1200  9
#define B1800  10
#define B2400  11
#define B4800  12
#define B9600  13
#define B19200 14
#define B38400 15

/* control modes */
#define CSIZE  0x000F /* character size */
#define CS5    0x0001
#define CS6    0x0002
#define CS7    0x0004
#define CS8    0x0008
#define CSTOPB 0x0010 /* send two stop bits, else one */
#define CREAD  0x0020 /* enable receiver */
#define PARENB 0x0040 /* parity enable */
#define PARODD 0x0080 /* odd parity, else even */
#define HUPCL  0x0100 /* hang up on last close */
#define CLOCAL 0x0200 /* ignore modem status lines */

/* local modes */
#define ECHO   0x0001 /* enable echo */
#define ECHOE  0x0002 /* echo erase character as error-correcting backsapce */
#define ECHOK  0x0004 /* echo KILL */
#define ECHONL 0x0008 /* echo NL */
#define ICANON 0x0010 /* canonical input (erase and kill processing) */
#define IEXTEN 0x0020 /* enable extended input character processing */
#define ISIG   0x0040 /* enable signals */
#define NOFLSH 0x0080 /* disable flush after interrupt or quit */
#define TOSTOP 0x0100 /* send SIGTTOU for background output */

/* attribute selection */
#define TCSANOW   1 /* change attributes immediately */
#define TCSADRAIN 2 /* change attributes when output has drained */
#define TCSAFLUSH 3 /* TCSADRAIN + flush pending input */

/* line control */
#define TCIFLUSH  1 /* flush pending input */
#define TCIOFLUSH 2 /* flush both pending input and untransmitted output */
#define TCOFLUSH  3 /* flush untransmitted output */
#define TCIOFF    4 /* transmit a STOP char, intended to suspend input data */
#define TCION     5 /* transmit a START char, intended to restart input data */
#define TCOOFF    6 /* suspend output */
#define TCOON     7 /* restart output */

#ifndef __ASSEMBLER__
#include <telos/type_defs.h>

#ifndef _CC_T_DEFINED
#define _CC_T_DEFINED
typedef _CC_T_TYPE cc_t;
#endif
#ifndef _SPEED_T_DEFINED
#define _SPEED_T_DEFINED
typedef _SPEED_T_TYPE speed_t;
#endif
#ifndef _TCFLAG_T_DEFINED
#define _TCFLAG_T_DEFINED
typedef _TCFLAG_T_TYPE tcflag_t;
#endif
#ifndef _PID_T_DEFINED
#define _PID_T_DEFINED
typedef _PID_T_TYPE pid_t;
#endif

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
};

#endif /* !__ASSEMBLER__ */
#endif
