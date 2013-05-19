/* ctsw.c : context switcher
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
#include <kernel/dispatch.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

static void *ksp;        /* kernel stack pointer  */
static void *psp;        /* process stack pointer */
static int  rc;          /* syscall return code   */
static unsigned int iid; /* syscall/interrupt ID  */

/* ISR entry points */
void fault_entry_point (void);
void timer_entry_point (void);
void kbd_entry_point (void);
void syscall_entry_point (void);

void isr_init (void)
{
    set_gate (PF_EXN,       (ulong) fault_entry_point,   SEG_KCODE);
    set_gate (TIMER_INTR,   (ulong) timer_entry_point,   SEG_KCODE);
    set_gate (KBD_INTR,     (ulong) kbd_entry_point,     SEG_KCODE);
    set_gate (SYSCALL_INTR, (ulong) syscall_entry_point, SEG_KCODE);
    ksp = 0;
}

unsigned int context_switch (struct pcb *p)
{
    // make sure TSS points to the right part of the stack
    tss.esp0 = (unsigned long) p->ifp;

    rc  = p->rc;
    psp = p->esp;
    asm volatile (
        "pushf                   \n" // save kernel context
        "pusha                   \n"
        "movl  %%esp,  ksp       \n" // switch stacks
        "movl  psp,    %%esp     \n"
        "movl  rc,     %%eax     \n" // put return code in %eax
        "mov   %[PGD], %%cr3     \n" // switch page directories
        "movl  %%eax,  28(%%esp) \n"
        "cmp   $0x0,   %[SPR]    \n"
        "jne   skip_seg_set      \n"
        "movw  %[UDS], %%ax      \n" // switch to user data segment
        "movw  %%ax,   %%ds      \n"
        "movw  %%ax,   %%es      \n"
        "movw  %%ax,   %%fs      \n"
        "movw  %%ax,   %%gs      \n"
    "skip_seg_set: "
        "popa                    \n" // restore user context
        "iret                    \n" // return to user process
    "fault_entry_point: "
        "pusha                   \n"
        "movl  %[PFX], %%eax     \n"
        "jmp   common_isr        \n"
    "timer_entry_point: "
        "pusha                   \n" // save user context
        "movl  %[TMR], %%eax     \n" // put interrupt ID in %eax
        "jmp   common_isr        \n" // jump to common ISR code
    "kbd_entry_point: "
        "pusha                   \n"
        "movl  %[KBD], %%eax     \n"
        "jmp   common_isr        \n"
    "syscall_entry_point: "
        "pusha                   \n"
    "common_isr: "
        "cmp   $0x0,      %[SPR] \n"
        "jne   skip_seg_set0     \n"
        "movw  %[KDS],    %%cx   \n" // switch to kernel data segment
        "movw  %%cx,      %%ds   \n"
        "movw  %%cx,      %%es   \n"
        "movw  %%cx,      %%fs   \n"
        "movw  %%cx,      %%gs   \n"
    "skip_seg_set0: "
        "movl  28(%%esp), %%ecx  \n"
        "movl  %%ecx,     rc     \n"
        "movl  %%esp,     psp    \n" // switch stacks
        "movl  ksp,       %%esp  \n"
        "movl  %%eax,     %[iid] \n" // save interrupt ID
        "popa                    \n" // restore kernel context
        "popf                    \n"
        : [iid] "=g" (iid)
        : [UDS] "i" (SEG_UDATA | 3), [KDS] "i" (SEG_KDATA),
          [TMR] "i" (TIMER_INTR), [KBD] "i" (KBD_INTR), [PFX] "i" (PF_EXN),
          [PGD] "b" (p->pgdir), [SPR] "g" (p->flags & PFLAG_SUPER)
        : "%eax", "%ecx"
    );
    p->esp = psp;
    p->rc  = rc;

    return iid;
}
