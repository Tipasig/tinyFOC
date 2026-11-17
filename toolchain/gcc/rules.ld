/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

__flash_start__         = ORIGIN(flash);
__flash_size__          = LENGTH(flash);
__flash_end__           = __flash_start__ + __flash_size__;
__ram0_start__          = ORIGIN(ram0);
__ram0_size__           = LENGTH(ram0);
__ram0_end__            = __ram0_start__ + __ram0_size__;
__ram1_start__          = ORIGIN(ram1);
__ram1_size__           = LENGTH(ram1);
__ram1_end__            = __ram1_start__ + __ram1_size__;

ENTRY(Reset_Handler)

SECTIONS
{
    . = ORIGIN(flash);
    _text = .;

    startup : ALIGN(16) SUBALIGN(16)
    {
        PROVIDE(__vectors_start = .);
        KEEP(*(.vectors))
        KEEP(*(.nds32_init))
        . = ALIGN(4);
        PROVIDE(__vectors_end = .);
        KEEP(*crt0_n10.o(.reset))
        . = ALIGN(4);
    } > flash

    PROVIDE(__initcode_lma_start = .);
    initcode : ALIGN(4)
    {
        PROVIDE(__initcode_vma_start = .);
        *(.setup_text)
        . = ALIGN(4);
        PROVIDE(__initcode_vma_end = .);
    } > CODE_RAM AT > flash

    constructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    } > flash

    destructors : ALIGN(4) SUBALIGN(4)
    {
        PROVIDE(__fini_array_start = .);
        KEEP(*(.fini_array))
        KEEP(*(SORT(.fini_array.*)))
        PROVIDE(__fini_array_end = .);
    } > flash

    .text : ALIGN(4) SUBALIGN(4)
    {
        *(.reset)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
    } > flash

    .mesh_cfg : {
        PROVIDE(__mesh_cfg_begin__ = .);
        KEEP(*(SORT(.mesh_cfg.*)))
        PROVIDE(__mesh_cfg_end__ = .);
    } > flash

    .eh_frame_hdr :
    {
        *(.eh_frame_hdr)
    } > flash

    .eh_frame : ONLY_IF_RO
    {
        *(.eh_frame)
    } > flash

    .textalign : ONLY_IF_RO
    {
        . = ALIGN(8);
    } > flash

    . = ALIGN(4);
    _etext = .;
    _textdata = _etext;

    .mstack :
    {
        . = ALIGN(8);
        __main_stack_base__ = .;
        . += __main_stack_size__;
        . = ALIGN(8);
        __main_stack_end__ = .;
    } > MAIN_STACK_RAM

    .pstack :
    {
        __process_stack_base__ = .;
        __main_thread_stack_base__ = .;
        . += __process_stack_size__;
        . = ALIGN(8);
        __process_stack_end__ = .;
        __main_thread_stack_end__ = .;
    } > PROCESS_STACK_RAM

    .data : ALIGN(4)
    {
        . = ALIGN(4);
        PROVIDE(_data = .);
        *(.data)
        *(.data.*)
    } > DATA_RAM AT > flash

    .ramtext : ALIGN(4)
    {
        . = ALIGN(4);
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
    } > DATA_RAM AT > flash

    .bttext : ALIGN(4)
    {
        PROVIDE(__textram_vma_start = .);
        . = ALIGN(4);
        *(.bttext)
    } > TEXT_RAM AT > flash

    __textram_lma_start = LOADADDR(.bttext);
    __textram_lma_end   = LOADADDR(.bttext) + SIZEOF(.bttext);

    __rw_vma_start = _data;
    __rw_vma_end   = _edata;
    __rw_lma_start = _etext;
    __rw_lma_end = _textdata + (_edata - _data);
    _stack = __main_thread_stack_end__;
    __main_stack__ = __main_stack_end__;

    .bss : ALIGN(4)
    {
        . = ALIGN(4);
        PROVIDE(_bss_start = .);
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        PROVIDE(_bss_end = .);
        PROVIDE(end = .);
    } > BSS_RAM
    __bss_start = _bss_start;
    _end = _bss_end;

    .ram0 (NOLOAD) : ALIGN(4)
    {
        . = ALIGN(4);
        *(.ram0)
        *(.ram0.*)
        . = ALIGN(4);
        __ram0_free__ = .;
    } > ram0

    .ram1 (NOLOAD) : ALIGN(4)
    {
        . = ALIGN(4);
        *(.ram1)
        *(.ram1.*)
        . = ALIGN(4);
        __ram1_free__ = .;
    } > ram1
}

/* Heap default boundaries, it is defaulted to be the non-used part
   of ram0 region.*/
__heap_base__   = __ram0_free__;
__heap_end__    = __ram0_end__;
/* hsble compatible */
__heap_base  = __heap_base__;
__heap_limit = __heap_end__;
