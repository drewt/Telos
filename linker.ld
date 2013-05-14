ENTRY (loader)

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
        bin/userspace.a:*(.text)
        lib/klib.a:*(.text)
    }

    urodata ALIGN (0x1000) :
    {
        bin/userspace.a:*(.rodata)
        lib/klib.a:*(.rodata)
    }

    udata ALIGN (0x1000) :
    {
        bin/userspace.a:*(.data .bss COMMON)
        lib/klib.a:*(.data .bss COMMON)
        uend = .;
    }

    /*
     * The kernel proper begins at 1MB.
     */
    . = 0x00100000;

    .text ALIGN (0x1000) :
    {
        kstart = .;
        *(.text)
        *(.rodata*)
    }

    .data ALIGN (0x1000) :
    {
        *(.data)
    }

    .bss ALIGN (0x1000) :
    {
        sbss = .;
        *(.bss)
        scomm = .;
        *(COMMON)
        kend = .;
    }

    /DISCARD/ :
    {
        /**(.comment)*/
        *(.eh_frame)
    }

}
