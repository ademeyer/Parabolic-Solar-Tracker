/* QEMU netduinoplus2 (Cortex-M7) Memory Layout */
MEMORY {
    /* QEMU's netduinoplus2 memory map */
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 16M  /* QEMU maps flash at 0 */
    RAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 16M  /* Abundant RAM in QEMU */
    /* Optional: Add SDRAM if needed */
    SDRAM (xrw) : ORIGIN = 0xC0000000, LENGTH = 8M
}

/* Entry Point */
ENTRY(Reset_Handler)

/* Stack/Heap Configuration */
_Min_Heap_Size  = 0x4000;   /* 16KB heap */
_Min_Stack_Size = 0x4000;   /* 16KB stack */

SECTIONS {
    /* Vector Table at start of FLASH */
    .isr_vector : {
        . = ALIGN(512);     /* Cortex-M7 requires 512-byte alignment */
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Code and read-only data */
    .text : {
        . = ALIGN(4);
        *(.text)            /* .text sections (code) */
        *(.text*)           /* .text* sections (code) */
        *(.glue_7)          /* glue arm to thumb code */
        *(.glue_7t)         /* glue thumb to arm code */
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;         /* Define global symbol at end of code */
    } >FLASH

    /* Constant data */
    .rodata : {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ARM exception handling */
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.arm.extab*) } >FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* Initialized data (loaded to RAM at startup) */
    _sidata = LOADADDR(.data);
    .data : {
        . = ALIGN(4);
        _sdata = .;         /* Create global symbol at data start */
        *(.data)
        *(.data*)
        
        . = ALIGN(4);
        _edata = .;         /* Define global symbol at data end */
    } >RAM AT>FLASH

    /* Uninitialized data (zeroed at startup) */
    .bss : {
        . = ALIGN(4);
        _sbss = .;         /* Define global symbol at bss start */
        *(.bss)
        *(.bss*)
        *(COMMON)
        
        . = ALIGN(4);
        _ebss = .;         /* Define global symbol at bss end */
    } >RAM

    /* User heap section */
    ._user_heap : {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = ALIGN(8);
    } >RAM

    /* User stack section (grows downward) */
    ._user_stack : {
        . = ALIGN(8);
        PROVIDE(_estack = .);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Optional: External SDRAM section */
    .sdram (NOLOAD) : {
        . = ALIGN(4);
        *(.sdram)
        *(.sdram*)
        . = ALIGN(4);
    } >SDRAM

    /* Remove debug info from standard libraries */
    /DISCARD/ : {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }
}