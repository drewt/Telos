# stolen from linux

.global setjmp
.global longjmp

setjmp:
	movl 4(%esp),%edx
	popl %ecx			# Return address, and adjust the stack
	xorl %eax,%eax			# Return value
	movl %ebx,(%edx)
	movl %esp,4(%edx)		# Post-return %esp!
	pushl %ecx			# Make the call/return stack happy
	movl %ebp,8(%edx)
	movl %esi,12(%edx)
	movl %edi,16(%edx)
	movl %ecx,20(%edx)		# Return address
	ret

longjmp:
	movl 4(%esp),%edx		# jmp_ptr address
	movl 8(%esp),%eax		# Return value
	movl (%edx),%ebx
	movl 4(%edx),%esp
	movl 8(%edx),%ebp
	movl 12(%edx),%esi
	movl 16(%edx),%edi
	jmp *20(%edx)
