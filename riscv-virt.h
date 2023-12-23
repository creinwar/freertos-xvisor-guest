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

#ifndef RISCV_VIRT_H_
#define RISCV_VIRT_H_

#include "riscv-reg.h"

#ifdef __ASSEMBLER__
#define CONS(NUM, TYPE)NUM
#else
#define CONS(NUM, TYPE)NUM##TYPE
#endif /* __ASSEMBLER__ */

#define PRIM_HART			0

#define PLIC_ADDR       CONS(0x0c000000, UL)
#define PLIC_ADDR_PTR   ((uint8_t *) PLIC_ADDR)
#define PLIC(offset)    *((volatile uint32_t *) (((uint64_t) PLIC_ADDR) + (offset)))

#define RTC_ADDR        CONS(0x10003000, UL)
#define RTC_ADDR_PTR    ((uint8_t *) RTC_ADDR)
#define RTC(offset)     *((volatile uint32_t *) (((uint64_t) RTC_ADDR) + (offset)))

#define NS16550_ADDR    CONS(0x10000000, UL)

#ifndef __ASSEMBLER__

int xGetCoreID( void );
void vSendString( const char * s );
void write32(void *addr, uint32_t val);
uint32_t read32(void *addr);

#endif /* __ASSEMBLER__ */

#endif /* RISCV_VIRT_H_ */
