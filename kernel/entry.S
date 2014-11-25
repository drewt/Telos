/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
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

#include <kernel/i386.h>
#include <kernel/process.h>

.macro set_segs val reg
	movw \val, \reg
	movw \reg, %ds
	movw \reg, %es
	movw \reg, %fs
	movw \reg, %gs
.endm

.macro save_all_regs
	pushl %gs
	pushl %fs
	pushl %es
	pushl %ds
	subl  $4, %esp
	pushl %ebp
	pushl %eax
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
.endm

# current in %ebx afterwards
.macro save_all
	save_all_regs
	movl  current, %ebx
	movl  PCB_ESP(%ebx), %ecx
	movl  %ecx, ESP(%esp)
	movl  %esp, PCB_ESP(%ebx)
.endm

.macro restore_all_regs
	popl %ebx
	popl %ecx
	popl %edx
	popl %edi
	popl %esi
	popl %eax
	popl %ebp
	addl $4, %esp
	popl %ds
	popl %es
	popl %fs
	popl %gs
.endm

# XXX: assumes current in %ebx
.macro restore_all
	movl ESP(%esp), %ecx
	movl %ecx, PCB_ESP(%ebx)
	restore_all_regs
.endm

.macro isr_entry entry handler
.global \entry
\entry:	save_all
	set_segs $SEG_KDATA, %cx
	call \handler
	jmp return_to_user
.endm

.macro exn_entry nr
.global exn\nr
exn\nr:	save_all
	set_segs $SEG_KDATA, %cx
	pushl $\nr
	call exn_err
	jmp return_to_user
.endm

exn_entry 0
exn_entry 1
exn_entry 2
exn_entry 3
exn_entry 4
exn_entry 5
exn_entry 6
exn_entry 7
exn_entry 8
exn_entry 9
exn_entry 10
exn_entry 11
exn_entry 12
exn_entry 13
exn_entry 14
exn_entry 15
exn_entry 16
exn_entry 17
exn_entry 18
exn_entry 19
exn_entry 20

isr_entry pgf_entry,   exn_page_fault
isr_entry fpe_entry,   exn_fpe
isr_entry ill_entry,   exn_ill_instr
isr_entry timer_entry, tick
isr_entry kbd_entry,   int_keyboard

.global syscall_entry
syscall_entry:
	save_all
	set_segs $SEG_KDATA, %cx
	movl $-1,           EAX(%esp)
	cmpl NR_SYSCALLS,   %eax
	jae  return_to_user
	call *systab(,%eax,4)
	movl %eax,          EAX(%esp)
return_to_user:
	call handle_signal
_return_to_user:
	movl PCB_ESP(%ebx), %esp
	movl PCB_IFP(%ebx), %ecx
	movl %ecx,          (tss + 4)
	restore_all
	iret

.global switch_to
switch_to:
	movl 0x4(%esp),     %ebx
_switch_to:
	movl PCB_PGD(%ebx), %eax
	movl %eax,          %cr3
	jmp _return_to_user

.global schedule
schedule:
	movl current, %eax
	movl PCB_ESP(%eax), %ecx
	pushfl
	pushl %cs
	pushl $schedule_return
	save_all
	movl %esp, PCB_ESP(%eax)
	call _schedule
	movl %eax, %ebx
	jmp _switch_to
schedule_return:
	movl current, %eax
	movl %ecx, PCB_ESP(%eax)
	movl PCB_RC(%eax), %eax
	ret

.global sys_exit
sys_exit:
	movl  0x4(%esp), %eax
	movl  current,   %ebx
	movl  $(_kstack + 0x4000), %esp
	subl  $0x8, %esp
	pushl %eax
	pushl %ebx
	call  do_exit
	call  schedule

