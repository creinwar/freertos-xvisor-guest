// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Christopher Reinwardt <creinwar@student.ethz.ch>

#include "goldfish_rtc.h"
#include "riscv-virt.h"

uint64_t goldfish_rtc_read_time(uint8_t *rtcdev)
{
	uint64_t res = 0, prev = 0;

	// Make sure to return the correct high 32-bit in case the
	// low 32-bit overflow between the read of the high bits and the
	// read of the low bits
	while((res = (uint64_t) read32(rtcdev + GOLDFISH_RTC_TIME_HIGH)) != prev){
		prev = res;
	}
	res = (res << 32) | (uint64_t) read32(rtcdev + GOLDFISH_RTC_TIME_LOW);
	return res;
}

uint32_t goldfish_rtc_alarm_status(uint8_t *rtcdev)
{
	return read32(rtcdev + GOLDFISH_RTC_ALARM_STATUS);
}

void goldfish_rtc_set_alarm(uint8_t *rtcdev, uint64_t alarm)
{
	write32(rtcdev + GOLDFISH_RTC_ALARM_HIGH, (uint32_t) (alarm >> 32));
	write32(rtcdev + GOLDFISH_RTC_ALARM_LOW, (uint32_t) alarm);
}

void goldfish_rtc_clear_alarm(uint8_t *rtcdev)
{
	write32(rtcdev + GOLDFISH_RTC_CLEAR_ALARM, 1);
}

uint32_t goldfish_rtc_interrupt_enabled(uint8_t *rtcdev)
{
	return read32(rtcdev + GOLDFISH_RTC_IRQ_ENABLED);
}

void goldfish_rtc_enable_interrupt(uint8_t *rtcdev)
{
	write32(rtcdev + GOLDFISH_RTC_IRQ_ENABLED, 1);
}

void goldfish_rtc_disable_interrupt(uint8_t *rtcdev)
{
	write32(rtcdev + GOLDFISH_RTC_IRQ_ENABLED, 0);
	write32(rtcdev + GOLDFISH_RTC_CLEAR_INTERRUPT, 1);
}

void goldfish_rtc_clear_interrupt(uint8_t *rtcdev)
{
	write32(rtcdev + GOLDFISH_RTC_CLEAR_INTERRUPT, 1);
}
