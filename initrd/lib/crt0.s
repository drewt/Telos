.global _start

_start:
	call main
	mov $1, %eax
	int $0x80
end:
	nop
	jmp end
