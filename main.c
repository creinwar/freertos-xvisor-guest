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
 * https://github.com/FreeRTOS
 *
 */

/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include <stdio.h>
#include <task.h>

#include "goldfish_rtc.h"
#include "isolation_bench.h"

extern void freertos_risc_v_trap_handler( void );
extern void freertos_vector_table( void );

void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
void vApplicationTickHook( void );

void vPortSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

int main( void )
{
	int ret = 0;
	// trap handler initialization
	__asm volatile(
		"csrw stvec, %0\n"
		:: "r"(freertos_risc_v_trap_handler)
		:
	);

	ret = isolation_bench();

	return ret;
}

/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook( void )
{
}
/*-----------------------------------------------------------*/

void vAssertCalled( void )
{
volatile uint32_t ulSetTo1ToExitFunction = 0;

	taskDISABLE_INTERRUPTS();
	while( ulSetTo1ToExitFunction != 1 )
	{
		__asm volatile( "NOP" );
	}
}

extern size_t uxTimerIncrementsForOneTick;

void vPortSetupTimerInterrupt( void )
{
	vSendString("Setting up timer interrupt...");

	// Set the Goldfish RTC timer
	uint64_t cur_time = 0, next_time = 0;

	// Clear the RTC interrupt and alarm
	goldfish_rtc_clear_alarm(RTC_ADDR_PTR);
	goldfish_rtc_clear_interrupt(RTC_ADDR_PTR);

	// Read out the current time
	cur_time = goldfish_rtc_read_time(RTC_ADDR_PTR);
	next_time = cur_time + uxTimerIncrementsForOneTick;

	// Set alarm
	goldfish_rtc_set_alarm(RTC_ADDR_PTR, next_time);

	// Configure the PLIC - RTC is interrupt 11
	// Priority
	PLIC(11*4) = 1;

	// Enable bit: 0 <= 11 <= 31
	PLIC(0x2000) = (1 << 11);

	// Enable RTC interrupt
	goldfish_rtc_enable_interrupt(RTC_ADDR_PTR);

	vSendString("Done");
}

// Generic interrupt handler, here taking care of the
// Goldfish RTC interrupt and software interrupt (i.e. yield)
#if __riscv_xlen == 32
void freertos_risc_v_application_interrupt_handler(uint32_t arch_scause, uint32_t arch_sepc)
#else
void freertos_risc_v_application_interrupt_handler(uint64_t arch_scause, uint64_t arch_sepc)
#endif
{
	uint64_t scause = arch_scause, sepc = arch_sepc;
	uint32_t irq_id = 0;

	// Silence compiler warnings about unused variables
	(void) sepc;

	// Get rid of the IRQ bit
	scause = ((scause << 1) >> 1);

	// Clear the SIP bit
	__asm volatile(
		"csrrc x0, sip, %0"
		:: "r"(1 << scause)
		:
	);

	// Software interrupt means yield
	if(scause == 1){
		vTaskSwitchContext();

	// Was this an external interrupt?
	} else if(scause == 9) {
		// Claim the interrupt from the PLIC
		irq_id = PLIC(0x200004);

		if(irq_id != 11){
			// Just say it's done and halt
			PLIC(0x200004) = irq_id;
			while(1){}

		} else {
			// Set the timer in the future and complete the IRQ
			uint64_t cur_time = 0, next_time = 0;

			// Clear alarm
			goldfish_rtc_clear_alarm(RTC_ADDR_PTR);

			// Clear interrupt
			goldfish_rtc_clear_interrupt(RTC_ADDR_PTR);

			// Take care of the scheduling stuff
			if(xTaskIncrementTick()){
				vTaskSwitchContext();
			}

			// Read out the current time
			cur_time = goldfish_rtc_read_time(RTC_ADDR_PTR);
			next_time = cur_time + uxTimerIncrementsForOneTick;

			// Set alarm
			goldfish_rtc_set_alarm(RTC_ADDR_PTR, next_time);
			PLIC(0x200004) = irq_id;
		}

	} else {
		while(1){};
	}
}
