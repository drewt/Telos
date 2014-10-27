ENTRY (loader)
KERNEL_LOAD_ADDR   = 0x00100000;
KERNEL_PAGE_OFFSET = 0xC0000000;

SECTIONS
{

	. = KERNEL_LOAD_ADDR;

	loader ALIGN (0x1000) :
	{
		boot/loader.o(.text)
	}

	. += KERNEL_PAGE_OFFSET;

	utext ALIGN (0x1000) : AT(ADDR(utext) - KERNEL_PAGE_OFFSET)
	{
		_ustart = .;
		_urostart = .;
		usr/usr.a:*(.text .rodata)
		lib/klib.a:*(.text .rodata)
		_uroend = .;
	}

	udata ALIGN (0x1000) : AT(ADDR(udata) - KERNEL_PAGE_OFFSET)
	{
		_urwstart = .;
		usr/usr.a:*(.data .bss COMMON)
		lib/klib.a:*(.data .bss COMMON)
		_urwend = .;
		_uend = .;
	}

	.text ALIGN (0x1000) : AT(ADDR(.text) - KERNEL_PAGE_OFFSET)
	{
		_kstart = .;
		_krostart = .;
		*(.text)
		*(.rodata)
		_kroend = .;
	}

	.data ALIGN (0x1000) : AT(ADDR(.data) - KERNEL_PAGE_OFFSET)
	{
		_krw_start = .;
		_kinit_set = .;
		*(.set.kinit)
		_kinit_set_end = .;
		_slab_set = .;
		*(.set.slab)
		_slab_set_end = .;
		*(.data)
	}

	.bss ALIGN (0x1000) : AT(ADDR(.bss) - KERNEL_PAGE_OFFSET)
	{
		*(.bss)
		*(COMMON)
		_krw_end = .;
		_kend = .;
	}

	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
	}

}
