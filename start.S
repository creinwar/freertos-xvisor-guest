/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://www.github.com/FreeRTOS
 *
 */

#include "riscv-virt.h"

	.section .init
	.globl _start
	.type _start,@function
_start:
	.cfi_startproc
	.cfi_undefined ra
.option push
.option norelax
	la gp, gp_load
	LOAD gp, 0(gp)
.option pop

	// Continue primary hart
	// expect hart id in a0
	li   a1, PRIM_HART
	bne  a0, a1, secondary

	// If we need to relocate, do that now
	la t0, __link_start	// t0 contains the address the code was loaded to
	la t1, link_start_val
	LOAD t1, 0(t1)		// t1 contains the address the code was linked to

	beq t0, t1, _relocation_done	// If both addresses are the same,
									// we don't have to relocate wuh

	la t2, __link_end	// We need this to know when to stop

_relocate:
	LOAD t3, 0(t0)	// Load from loaded address
	STOR t3, 0(t1)	// Write to link address
	addi t0, t0, REGSIZE
	addi t1, t1, REGSIZE
	blt t0, t2, _relocate

	fence.i

	la t0, relocation_end_exec
	LOAD t0, 0(t0)	// Get the link address of the next label
	jalr x0, t0		// and jump to it

_relocation_done:

	// Primary hart
	la sp, sp_load
	LOAD sp, 0(sp)

	// Load data section
	la a0, _data_lma
	la a1, _data
	la a2, _edata
	bgeu a1, a2, 2f
1:
	LOAD t0, (a0)
	STOR t0, (a1)
	addi a0, a0, REGSIZE
	addi a1, a1, REGSIZE
	bltu a1, a2, 1b
2:

	// Clear bss section
	la a0, _bss
	la a1, _ebss
	bgeu a0, a1, 2f
1:
	STOR zero, (a0)
	addi a0, a0, REGSIZE
	bltu a0, a1, 1b
2:

	// argc, argv, envp is 0
	li  a0, 0
	li  a1, 0
	li  a2, 0
	jal main
1:
	wfi
	j 1b

secondary:
	// TODO: Multicore is not supported
	wfi
	j secondary
	.cfi_endproc

gp_load:
	.dword __global_pointer$
sp_load:
	.dword _stack_top
link_start_val:
	.dword __link_start
link_end_val:
	.dword __link_end
relocation_end_exec:
	.dword _relocation_done