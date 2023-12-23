// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Christopher Reinwardt <creinwar@student.ethz.ch>
//
// Adapted from the RISC-V blinky example

/* FreeRTOS kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <stdio.h>

#include "riscv-virt.h"
#include "ns16550.h"
#include "goldfish_rtc.h"

/* Priorities used by the tasks. */
#define PROBE_TASK_PRIO	( tskIDLE_PRIORITY + 2 )
#define	PRIME_TASK_PRIO	( tskIDLE_PRIORITY + 1 )

#define NUM_TLB_ENTRIES 16

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t queue = NULL;

//static uint8_t __attribute__((align(4096))) shmem[4096*NUM_TLB_ENTRIES];
static uint8_t shmem[4096*NUM_TLB_ENTRIES];

/*-----------------------------------------------------------*/

//static void __attribute__((bare)) tlb_access(void *base, uint64_t num_pages, uint64_t descending)
static void tlb_access(void *base, uint64_t num_pages, uint64_t descending)
{
    __asm volatile(
        "add a0, x0, %0\n                                                           \
        add a1, x0, %1\n                                                            \
        add a2, x0, %2\n                                                            \
        beq a1, x0, 2f /* Don't touch any pages? Sure! */\n                         \
        /* Default increment: one 4k page in positive direction */\n                \
        addi a3, x0, 1\n                                                            \
        slli a3, a3, 12\n                                                           \
        beq a2, x0, 1f  /* Ascending or do we have to update the base address? */\n \
        addi t0, a1, -1\n                                                           \
        slli t1, t0, 12\n                                                           \
        add a0, a0, t1\n                                                            \
        /* Increment: one 4k page in NEGATIVE direction */\n                        \
        addi a3, x0, -1\n                                                           \
        slli a3, a3, 12\n                                                           \
1:\n                                                                                \
        ld t2, 0(a0)  /* Access the page */\n                                       \
        addi a1, a1, -1\n                                                           \
        add a0, a0, a3\n                                                            \
        blt x0, a1, 1b\n                                                            \
2:\n                                                                                \
        ret\n"
        :: "r"(base), "r"(num_pages), "r"(descending)
        :);
}

static void prime_task(void *pvParameters)
{
	const uint64_t tx_val = 0xDEADBEEFUL;

	vSendString("Entered prime task");

	/* Remove compiler warning about unused parameter. */
	(void) pvParameters;

	while(1){

		// Touch each page once
		tlb_access(shmem, NUM_TLB_ENTRIES, 0);

		// Signal to the probe task that we're done with priming
		xQueueSend(queue, &tx_val, 0U);
	}
}

/*-----------------------------------------------------------*/

static void probe_task( void *pvParameters )
{
	char buf[255];
	uint64_t queue_rx_val = 0;
	uint64_t pre_time = 0, post_time = 0;
	uint64_t diff = 0, prev_diff = 0;
	const uint64_t rx_val = 0xDEADBEEFUL;
	int descending = 0;

	vSendString("Entered probe task");

	/* Remove compiler warning about unused parameter. */
	(void) pvParameters;

	while(1){
		// Wait until the priming was done
		xQueueReceive(queue, &queue_rx_val, portMAX_DELAY);

		// Check whether we got the correct value from the queue
		if(queue_rx_val == rx_val){
			pre_time = goldfish_rtc_read_time(RTC_ADDR_PTR);

			tlb_access(shmem, NUM_TLB_ENTRIES, descending);

			post_time = goldfish_rtc_read_time(RTC_ADDR_PTR);

			descending = !descending;

			diff = post_time - pre_time;

			if(diff >= prev_diff){
				sprintf(buf, "Num cycles: %lu, diff to prev: +%lu", diff, (diff - prev_diff));
			} else {
				sprintf(buf, "Num cycles: %lu, diff to prev: -%lu", diff, (prev_diff - diff));
			}

			vSendString(buf);

			prev_diff = diff;

			queue_rx_val = 0;
		} else {
			vSendString("[probe_task] Read wrong value from queue");
			sprintf(buf, "\tval: 0x%lx", queue_rx_val);
			vSendString(buf);
		}
	}
}

/*-----------------------------------------------------------*/

int isolation_bench(void)
{
	vSendString("Starting isolation benchmark");

	queue = xQueueCreate(1, sizeof(uint64_t));

	if(!queue){
		vSendString("Queue creation failed!");

		return -1;
	}

	xTaskCreate(prime_task, "Prime", configMINIMAL_STACK_SIZE * 2U, NULL, PRIME_TASK_PRIO, NULL);
	xTaskCreate(probe_task, "Probe", configMINIMAL_STACK_SIZE * 2U, NULL, PROBE_TASK_PRIO, NULL);

	vTaskStartScheduler();

	return 0;
}