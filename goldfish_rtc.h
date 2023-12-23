// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Christopher Reinwardt <creinwar@student.ethz.ch>

#include <stdint.h>

/* register definitions - taken from Xvisors Goldfish RTC emulator */
/* https://github.com/xvisor/xvisor/blob/master/emulators/rtc/goldfish_rtc.c */

#define GOLDFISH_RTC_TIME_LOW			0x00
#define GOLDFISH_RTC_TIME_HIGH			0x04
#define GOLDFISH_RTC_ALARM_LOW			0x08
#define GOLDFISH_RTC_ALARM_HIGH			0x0c
#define GOLDFISH_RTC_IRQ_ENABLED		0x10
#define GOLDFISH_RTC_CLEAR_ALARM		0x14
#define GOLDFISH_RTC_ALARM_STATUS		0x18
#define GOLDFISH_RTC_CLEAR_INTERRUPT	0x1c

uint64_t goldfish_rtc_read_time(uint8_t *rtcdev);

uint32_t goldfish_rtc_alarm_status(uint8_t *rtcdev);

void goldfish_rtc_set_alarm(uint8_t *rtcdev, uint64_t alarm);

void goldfish_rtc_clear_alarm(uint8_t *rtcdev);

uint32_t goldfish_rtc_interrupt_enabled(uint8_t *rtcdev);

void goldfish_rtc_enable_interrupt(uint8_t *rtcdev);

void goldfish_rtc_disable_interrupt(uint8_t *rtcdev);

void goldfish_rtc_clear_interrupt(uint8_t *rtcdev);
