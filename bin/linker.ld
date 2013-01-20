ENTRY (loader)

SECTIONS
{
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
