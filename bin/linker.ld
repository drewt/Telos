ENTRY (loader)

SECTIONS
{
    /*
     * We put the userspace programs at 2MB
     */
    . = 0x00200000;

    .user ALIGN (0x1000) :
    {
        ustart = .;
        bin/userspace.a:*
        lib/klib.a:*
        uend = .;
    }

    /*
     * The kernel proper begins at 1MB
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

}
