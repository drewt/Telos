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

#include <stdint.h>

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
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t stack[0];
}; 

/* process context before iret (in the kernel) */
struct ctxt {
    struct gp_regs reg;
    uint32_t iret_eip;
    uint32_t iret_cs;
    uint32_t eflags;
    uint32_t iret_esp;
    uint32_t iret_ss;
    uint32_t stack[0];
};

struct tss_entry {
    uint32_t prev;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

extern struct tss_entry tss;

/* wrapper for outb instruction */
static inline void outb (uint16_t port, uint8_t data) {
    asm volatile ("outb %1, %0" : : "d" (port), "a" (data));
}

/* wrapper for inb instruction */
static inline unsigned char inb (uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a" (ret) : "d" (port));
    return ret;
}

/* get the code segment selector */
static inline uint16_t get_cs (void) {
    uint16_t cs;
    asm volatile ("movw %%cs, %0   \n" : "=g" (cs) : : );
    return cs;
}

/* get the data segment selector */
static inline uint16_t get_ds (void) {
    uint16_t ds;
    asm volatile ("movw %%ds, %0   \n" : "=g" (ds) : : );
    return ds;
}

/* get the task register */
static inline uint16_t get_tr (void) {
    uint16_t tr;
    asm volatile ("str %0" : "=g" (tr) : : );
    return tr;
}

static inline void halt (void) {
    asm volatile ("_halt: hlt\njmp _halt");
}

extern int kprintf (const char *c, ...);
/* from osdev.org wiki inline asm examples */
/*-----------------------------------------------------------------------------
 * Loads the interrupt descriptor table given by (base,size) */
//-----------------------------------------------------------------------------
static inline void load_idt (void *base, uint16_t size) {
    volatile struct {
        uint16_t length;
        uint32_t base;
    } __attribute__((__packed__)) idtr = { size, (uint32_t) base };
    asm volatile ("lidt (%0)" : : "g" (&idtr));
}

/* from osdev.org wiki inline asm examples (modified to update selectors) */
/*-----------------------------------------------------------------------------
 * Loads the global descriptor table given by (base,size) and updates the
 * segment selectors */
//-----------------------------------------------------------------------------
static inline void load_gdt (void *base, uint16_t size) {
    volatile struct {
        uint16_t length;
        uint32_t base;
    } __attribute__((__packed__)) gdtr = { size, (uint32_t) base };
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
static inline void load_tss (uint16_t val) {
    asm volatile (
        "movw %0, %%ax \n"
        "ltr  %%ax     \n"
        : : "g" (val) : "%eax"
    );
}

extern void gdt_install (void);
extern void idt_install (void);
extern void set_gate (uint8_t num, unsigned long handler, int selector);
extern void pic_init (uint16_t off1, uint16_t off2);
extern void enable_irq (uint8_t irq, int disable);
extern void pic_eoi (void);
extern void pit_init (int div);

#endif // __I386_H_
