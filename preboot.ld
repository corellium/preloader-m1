ENTRY(entry);

MEMORY
{
    text : ORIGIN = 0x803040000,  LENGTH = 0x20000
}

SECTIONS {
    .start : {
        *(.start*);
    } > text

    .text : {
        *(.text*);
    } > text

    .rodata : {
        *(.rodata*);
    } > text

    .data : {
        *(.data*);
    } > text

    .bss : {
        *(.bss*);
    } > text

    /DISCARD/ : {
        *(.eh_frame)
    }
}
