/* linker.ld for my_hypervisor */
ENTRY(_start)

SECTIONS
{
    /* QEMU virt machine RAM starts at 0x40000000. 
     * We load the hypervisor at 0x40080000, leaving space for device tree / bootloaders.
     */
    . = 0x40080000;

    .text : {
        KEEP(*(.text.boot))
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .data : {
        *(.data .data.*)
    }

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    }
    . = ALIGN(16);
    __bss_end = .;

    /* Reserve 16KB for early boot stack */
    . = ALIGN(16);
    . = . + 0x4000;
    __stack_top = .;

    _end = .;
}
