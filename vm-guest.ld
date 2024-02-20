OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    ram (rxa) : ORIGIN = 0x60000000, LENGTH = 512K
    dspm (rwa) : ORIGIN = 0x61000000, LENGTH = 16k
    ispm (rwx) : ORIGIN = 0x62000000, LENGTH = 8k
}

SECTIONS
{
    .init : ALIGN(16)
    {
        _text = .;
        __link_start = .;
        KEEP (*(SORT_NONE(.init)))
    } > ram

    .text : ALIGN(16)
    {
        *(.text.unlikely .text.unlikely.*)
        *(.text.startup .text.startup.*)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
    } > ram

    .fini : ALIGN(16)
    {
        KEEP (*(SORT_NONE(.fini)))
        _etext = .;
    } > ram

    .rodata.align : ALIGN(16)
    {
        _rodata = .;
    } > ram

    .rodata.start :
    {
        _rodata_lma = LOADADDR(.rodata.start);
    } > ram

    .rodata :
    {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)

        . = ALIGN(16);
        _erodata = .;
    } > ram

    .data.align :
    {
        . = ALIGN(16);
        _data = .;
    } > ram

    .data.start :
    {
        _data_lma = LOADADDR(.data.start);
    } > ram

    .data :
    {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(8);
        PROVIDE( __global_pointer$ = . + 0x800 );
        *(.sdata .sdata.*)
        *(.sdata2 .sdata2.*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(8);
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)

        . = ALIGN(16);
        _edata = .;
    } > ram

    . = ALIGN(16);

    __link_end = .;

    .ispm ORIGIN(ispm) : ALIGN(16)
    {
        __ispm_start = .;
        __ispm_load_start = LOADADDR(.ispm);
        *(.ispm)
        *(.ispm.*)
        . = ALIGN(16);
        __ispm_end = .;
        __ispm_load_end = LOADADDR(.ispm) + SIZEOF(.ispm);
    } AT>ram

    .bss.align :
    {
        . = ALIGN(16);
        _bss = .;
    } > ram

    .bss.start :
    {
        _bss_lma = LOADADDR(.bss.start);
    } > ram

    .bss :
    {
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)

        . = ALIGN(16);
        _ebss = .;
    } > ram

    . = ALIGN(16);
    _end = .;

    .stack ORIGIN(dspm) : ALIGN(16)
    {
        . += __stack_size;
        . = ALIGN(16);
        _stack_top = .;
    }

}
