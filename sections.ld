ENTRY (loader)
KERNEL_LOAD_ADDR   = 0x00100000;
KERNEL_PAGE_OFFSET = 0xC0000000;

SECTIONS
{
    /*
     * We put the userspace programs at 2MB.
     * Sections are grouped according to the page
     * attributes they should have.
     */
    . = 0x00200000;

    utext ALIGN (0x1000) :
    {
        ustart = .;
        _urostart = .;
        usr/usr.a:*(.text .rodata)
        lib/klib.a:*(.text .rodata)
        _uroend = .;
    }

    udata ALIGN (0x1000) :
    {
        usr/usr.a:*(.data .bss COMMON)
        lib/klib.a:*(.data .bss COMMON)
        uend = .;
    }

    /*
     * The kernel proper begins at 1MB.
     */
    . = KERNEL_LOAD_ADDR;

    loader ALIGN (0x1000) :
    {
        kstart = .;
        boot/loader.o
    }

    .text ALIGN (0x1000) :
    {
        _krostart = .;
        *(.text)
        *(.rodata)
        _kroend = .;
    }

    .data ALIGN (0x1000) :
    {
        _krw_start = .;
        *(.data)
    }

    .bss ALIGN (0x1000) :
    {
        *(.bss)
        *(COMMON)
        _krw_end = .;
        kend = .;
    }

    /DISCARD/ :
    {
        /**(.comment)*/
        *(.eh_frame)
    }

}
