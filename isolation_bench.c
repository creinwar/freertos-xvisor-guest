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
#define PROBE_TASK_PRIO	( tskIDLE_PRIORITY )

#define NUM_TLB_ENTRIES 64
#define NUM_TEST_ROUNDS 10000
#define TIMESLICE_THRESH 1000000

/*-----------------------------------------------------------*/

static uint8_t shmem[4096*(NUM_TLB_ENTRIES+1)];

extern void tlb_access(void *base, uint64_t num_pages, uint64_t descending);

/*-----------------------------------------------------------*/

static inline uint64_t rdcycle(void)
{
	uint64_t cyc = 0;
	__asm volatile(
		"csrrs %0, cycle, x0\n"
		: "=r"(cyc)
		::
	);
	return cyc;
}

static inline uint64_t read_ctxt_swtch(void)
{
	uint64_t cyc = 0;
	__asm volatile(
		"csrrs %0, 0x5DB, x0\n"
		: "=r"(cyc)
		::
	);
	return cyc & 1;
}

static __attribute__((noinline)) void new_timeslice_rdcycle(void)
{
	uint64_t first = 0, second = 0;

	first = rdcycle();
	while(1) {
		second = rdcycle();

		if(second-first > TIMESLICE_THRESH)
			return;

		first = second;
	}
}

static __attribute__((noinline)) void new_timeslice_ctx_swtch(void)
{
	__asm volatile (
		"csrrsi x0, 0x5DB, 2\n"
		:::
	);

	while(!read_ctxt_swtch()){}

	__asm volatile (
		"csrrw x0, 0x5DB, x0\n"
		:::
	);
}

/*-----------------------------------------------------------*/

static void probe_task( void *pvParameters )
{
	char buf[256];
	uint64_t pre_time = 0, post_time = 0;
	uint64_t diff = 0, prev_diff = 0;
	uint8_t *mem = (uint8_t *) (((uint64_t) shmem + 4095) & ~4095);

	(void) pvParameters;

	vSendString("[probe_task] Starting");

	for(int i = 0; i < NUM_TEST_ROUNDS; i++){

		// Prime the TLB with our mappings
		// always in ascending order
		tlb_access(mem, NUM_TLB_ENTRIES, 0);

		// Wait for the adversary to run
		(void)new_timeslice_rdcycle;
		new_timeslice_ctx_swtch();

		// Take the before measurement
		pre_time = rdcycle();

		// Touch all pages again
		// always in descending order, to maximize the
		// overlap with the primed entries (no self-eviction)
		tlb_access(mem, NUM_TLB_ENTRIES, 1);

		// Take the after measurement
		post_time = rdcycle();

		// Send TLB dump trigger
		// The trigger is decremented on every context switch
		// so this sets a timeout to stop dumping once the
		// RTOS VM is killed
		/*__asm volatile(
			"csrrw x0, 0x802, %0\n"
			:: "r"(5)
			:
		);*/

		diff = (post_time - pre_time);

		if(diff >= prev_diff){
			sprintf(buf, "cycles: %lu, diff to prev: +%lu", diff, (diff - prev_diff));
		} else {
			sprintf(buf, "cycles: %lu, diff to prev: -%lu", diff, (prev_diff - diff));
		}

		vSendString(buf);

		prev_diff = diff;
	}

	vSendString("[probe_task] Done!");

	// We finished what we wanted to do
	// Wait for read-out of statistics
	while(1){}
}

/*-----------------------------------------------------------*/

int isolation_bench(void)
{
	vSendString("Starting isolation benchmark");

	uint64_t cyc1 = 0, cyc2 = 0;
	__asm volatile(
		"csrrs %0, cycle, x0\n	\
		 csrrs %1, cycle, x0\n"
		 : "=r"(cyc1), "=r"(cyc2)
		 ::
	);

	char buf[256];
	sprintf(buf, "cyc1 = 0x%lx, cyc2 = 0x%lx, diff = 0x%lx", cyc1, cyc2, cyc2-cyc1);
	vSendString(buf);

	//xTaskCreate(probe_task, "Probe", configMINIMAL_STACK_SIZE * 2U, NULL, PROBE_TASK_PRIO, NULL);

	//vTaskStartScheduler();
	probe_task(NULL);

	return 0;
}
