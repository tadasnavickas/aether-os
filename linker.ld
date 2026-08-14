OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(kmain)

PHDRS
{
    text    PT_LOAD FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD FLAGS(1 << 2) ;             /* Read Only */
    data    PT_LOAD FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(4K);

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(4K);

    .data : {
        *(.data .data.*)
        *(.requests .requests.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    . = ALIGN(4K);
}