/* i386.h : routines specific to the i386 architecture
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

#ifndef __I386_H_
#define __I386_H_

#define EFLAGS_IOPL(x) ((x) << 12)
#define EFLAGS_IF 0x0200

#define KCODE_SEGN 1
#define KDATA_SEGN 2
#define UCODE_SEGN 3
#define UDATA_SEGN 4
#define TSS_SEGN   5

#define SEG_KCODE (KCODE_SEGN << 3)
#define SEG_KDATA (KDATA_SEGN << 3)
#define SEG_UCODE (UCODE_SEGN << 3)
#define SEG_UDATA (UDATA_SEGN << 3)
#define SEG_TSS   (TSS_SEGN   << 3)

/* general purpose registers (pusha ordering) */
struct gp_regs {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long esp;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
    unsigned long stack[0];
}; 

/* process context before iret (in the kernel) */
struct ctxt {
    struct gp_regs reg;
    unsigned long iret_eip;
    unsigned long iret_cs;
    unsigned long eflags;
    unsigned long iret_esp;
    unsigned long iret_ss;
    unsigned long stack[0];
};

struct tss_entry {
    unsigned long prev;
    unsigned long esp0;
    unsigned long ss0;
    unsigned long esp1;
    unsigned long ss1;
    unsigned long esp2;
    unsigned long ss2;
    unsigned long cr3;
    unsigned long eip;
    unsigned long eflags;
    unsigned long eax;
    unsigned long ecx;
    unsigned long edx;
    unsigned long ebx;
    unsigned long esp;
    unsigned long ebp;
    unsigned long esi;
    unsigned long edi;
    unsigned long es;
    unsigned long cs;
    unsigned long ss;
    unsigned long ds;
    unsigned long fs;
    unsigned long gs;
    unsigned long ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed));

extern struct tss_entry tss;

/* wrapper for outb instruction */
static inline void outb (port_t port, unsigned char data)
{
    asm volatile ("outb %1, %0" : : "d" (port), "a" (data));
}

/* wrapper for inb instruction */
static inline unsigned char inb (port_t port)
{
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "d" (port));
    return ret;
}

/* get the code segment selector */
static inline unsigned short get_cs (void)
{
    unsigned short cs;
    asm volatile ("movw %%cs, %0   \n" : "=g" (cs) : : );
    return cs;
}

/* get the data segment selector */
static inline unsigned short get_ds (void)
{
    unsigned short ds;
    asm volatile ("movw %%ds, %0   \n" : "=g" (ds) : : );
    return ds;
}

/* get the task register */
static inline unsigned short get_tr (void)
{
    unsigned short tr;
    asm volatile ("str %0" : "=g" (tr) : : );
    return tr;
}

static inline void halt (void)
{
    asm volatile ("_halt: hlt\njmp _halt");
}

/* from osdev.org wiki inline asm examples */
/*-----------------------------------------------------------------------------
 * Loads the interrupt descriptor table given by (base,size) */
//-----------------------------------------------------------------------------
static inline void load_idt (void *base, unsigned short size)
{
    volatile struct {
        u16 length;
        u32 base;
    } __attribute__((__packed__)) idtr = { size, (u32) base };
    asm volatile ("lidt (%0)" : : "g" (&idtr));
}

/* from osdev.org wiki inline asm examples (modified to update selectors) */
/*-----------------------------------------------------------------------------
 * Loads the global descriptor table given by (base,size) and updates the
 * segment selectors */
//-----------------------------------------------------------------------------
static inline void load_gdt (void *base, unsigned short size)
{
    volatile struct {
        u16 length;
        u32 base;
    } __attribute__((__packed__)) gdtr = { size, (u32) base };
    asm volatile (
            "lgdt (%0)         \n"
            "ljmp %1,   $setcs \n"
        "setcs:                \n"
            "movw %2,   %%ax   \n"
            "movw %%ax, %%ds   \n" 
            "movw %%ax, %%es   \n"
            "movw %%ax, %%ss   \n"
            :
            : "g" (&gdtr), "i" (SEG_KCODE), "i" (SEG_KDATA)
            : "%eax"
    );
}

/*-----------------------------------------------------------------------------
 * Loads the TSS register with a given value */
//-----------------------------------------------------------------------------
static inline void load_tss (unsigned short val)
{
    asm volatile (
        "movw %0, %%ax \n"
        "ltr  %%ax     \n"
        : : "g" (val) : "%eax"
    );
}

static inline void put_iret_frame (struct ctxt *f, unsigned long eip,
        unsigned long esp)
{
    f->iret_cs  = SEG_UCODE | 3;
    f->iret_eip = eip;
    f->eflags   = EFLAGS_IOPL(0) | EFLAGS_IF;
    f->iret_esp = esp;
    f->iret_ss  = SEG_UDATA | 3;
}

extern void gdt_install (void);
extern void idt_install (void);
extern void set_gate (unsigned int num, unsigned long handler,
        unsigned short selector);
extern void pic_init (u16 off1, u16 off2);
extern void enable_irq (unsigned char irq, bool disable);
extern void pic_eoi (void);
extern void pit_init (int div);

#endif // __I386_H_
