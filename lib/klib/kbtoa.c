/* kbtoa.c : convert keyboard scan code to ascii character
 */

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

#define NOCHAR   256
#define KEY_UP   0x80

enum ctl_codes {
    LSHIFT = 0x2A,
    RSHIFT = 0x36,
    LMETA  = 0x38,
    LCTL   = 0x1D,
    CAPSL  = 0x3A
};

enum scan_state_flags {
    INCTL = 1,
    INSHIFT = 1 << 1,
    INCAPS  = 1 << 2,
    INMETA  = 1 << 3
};

static int state; /* the state of the keyboard */

/*  Normal table to translate scan code  */
static const unsigned char kbcode[] = { 0,
          27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
         '0',  '-',  '=', '\b', '\t',  'q',  'w',  'e',  'r',  't',
         'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',
         's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',
         '`',    0, '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',
         ',',  '.',  '/',    0,    0,    0,  ' '
};

/* captialized ascii code table to tranlate scan code */
static const unsigned char kbshift[] = { 0,
          0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',
        ')',  '_',  '+', '\b', '\t',  'Q',  'W',  'E',  'R',  'T',
        'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',    0,  'A',
        'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
        '~',    0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',
        '<',  '>',  '?',    0,    0,    0,  ' '
};

/* extended ascii code table to translate scan code */
static const unsigned char kbctl[] = { 0,
         0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
         0,   31,    0, '\b', '\t',   17,   23,    5,   18,   20,
        25,   21,    9,   15,   16,   27,   29, '\n',    0,    1,
        19,    4,    6,    7,    8,   10,   11,   12,    0,    0,
         0,    0,   28,   26,   24,    3,   22,    2,   14,   13
};

/*-----------------------------------------------------------------------------
 * Converts a keyboard scan code to an ascii character code */
//-----------------------------------------------------------------------------
unsigned int kbtoa (unsigned char code) {
    unsigned int ch;

    if (code & KEY_UP) {
        switch (code & 0x7f) {
        case LSHIFT:
        case RSHIFT:
            state &= ~INSHIFT;
            break;
        case LCTL:
            state &= ~INCTL;
            break;
        case LMETA:
            state &= ~INMETA;
            break;
        }
        return NOCHAR;
    }

    // meta keys
    switch (code) {
    case LSHIFT:
    case RSHIFT:
        state |= INSHIFT;
        return NOCHAR;
    case CAPSL:
        state ^= INCAPS;
        return NOCHAR;
    case LCTL:
        state |= INCTL;
        return NOCHAR;
    case LMETA:
        state |= INMETA;
        return NOCHAR;
    }

    ch = NOCHAR;

    if ((state & INCTL) && code < sizeof(kbctl))
        ch = kbctl[code];
    else if (code < sizeof(kbcode))
        // ch = (INCAPS xor INSHIFT) ? kbshift[code] : kbcode[code]
        ch = (!(state & INCAPS) != !(state & INSHIFT)) ? kbshift[code] :
                                                         kbcode[code];
    
    if (state & INMETA)
        ch += 0x80;
    return ch;
}
