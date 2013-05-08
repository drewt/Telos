/* pic.c : 8259 PIC
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

#include <kernel/common.h>
#include <kernel/i386.h>

/* PIC ports */
#define PIC1_CMD 0x20
#define PIC1_DAT 0x21
#define PIC2_CMD 0xA0
#define PIC2_DAT 0xA1

/* PIC commands */
#define PIC_EOI   0x20

/* PIT ports */
#define PIT_CH0  0x40
#define PIT_CH1  0x41
#define PIT_CH2  0x42
#define PIT_MODE 0x43

/* PIT commands */
#define PIT_SEL0    0x0
#define PIT_16BIT   0x3
#define PIT_RATEGEN 0x4

#define PIT_FREQ 1193182
#define PIT_DIV(x) ((PIT_FREQ+(x)/2)/(x))

/*-----------------------------------------------------------------------------
 * Initializes the 8259 Programmable Interrupt Controller */
//-----------------------------------------------------------------------------
void pic_init (u16 off1, u16 off2)
{
    // master PIC
    outb (PIC1_CMD, 0x11);
    outb (PIC1_DAT, off1);
    outb (PIC1_DAT, 0x4);
    outb (PIC1_DAT, 0x1);
    outb (PIC1_CMD, 0xB);
    // slave PIC
    outb (PIC2_CMD, 0x11);
    outb (PIC2_DAT, off2);
    outb (PIC2_DAT, 0x2);
    outb (PIC2_DAT, 0xB);
    outb (PIC2_CMD, 0xB);
    // mask all interrupts
    outb (PIC2_DAT, 0xFF);
    outb (PIC1_DAT, 0xFF);
}

/*-----------------------------------------------------------------------------
 * Signals end of interrupt to the PIC */
//-----------------------------------------------------------------------------
void pic_eoi (void)
{
    outb (PIC1_CMD, PIC_EOI);
    outb (PIC2_CMD, PIC_EOI);
}

/*-----------------------------------------------------------------------------
 * Enables/disables IRQ line */
//-----------------------------------------------------------------------------
void enable_irq (unsigned char irq, bool disable)
{
    port_t port;
    unsigned char val;

    // select PIC
    if (irq < 8) {
        port = PIC1_DAT;
    } else {
        port = PIC2_DAT;
        irq &= 0x7;
    }

    // adjust mask
    val = inb (port);
    val = disable ? val | (1 << irq) : val & ~(1 << irq);
    outb (port, val);
}

/*-----------------------------------------------------------------------------
 * Initializes the programmably interval timer */
//-----------------------------------------------------------------------------
void pit_init (int div) {
    /* program the PIT for rategen on channel 0 */
    outb (PIT_MODE, PIT_SEL0 | PIT_16BIT | PIT_RATEGEN);
    outb (PIT_CH0, PIT_DIV(div) & 0xFF); // low div byte
    outb (PIT_CH0, PIT_DIV(div) >> 8);   // high div byte
    
    enable_irq (0, 0);
}
