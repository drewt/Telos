ENTRY(_start)
INPUT(lib/crt0.o ../lib/klib.a)

/* Telos (ELF) executables are loaded at 0x08048000 */
SECTIONS
{
	. = 0x08048000;
	.text   ALIGN (0x1000) : { *(.text) }
	.rodata ALIGN (0x1000) : { *(.rodata) }
	.data   ALIGN (0x1000) : { *(.data) }
	.bss    ALIGN (0x1000) : { *(.bss) }
}
