/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* System module for Chrome EC : MEC1322 hardware specific implementation */

#include "battery.h"
#include "charge_state_v2.h"
#include "clock.h"
#include "common.h"
#include "console.h"
#include "cpu.h"
#include "gpio.h"
#include "host_command.h"
#include "registers.h"
#include "shared_mem.h"
#include "system.h"
#include "hooks.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "spi.h"

#define CPRINTS(format, args...) cprints(CC_SYSTEM, format, ## args)

/* Indices for hibernate data registers (RAM backed by VBAT) */
enum hibdata_index {
	HIBDATA_INDEX_SCRATCHPAD = 0,    /* General-purpose scratchpad */
	HIBDATA_INDEX_SAVED_RESET_FLAGS  /* Saved reset flags */
};

static void check_reset_cause(void)
{
	uint32_t status = MEC1322_VBAT_STS;
	uint32_t flags = 0;
	uint32_t rst_sts = MEC1322_PCR_CHIP_PWR_RST &
				(MEC1322_PWR_RST_STS_VCC1 |
				MEC1322_PWR_RST_STS_VBAT);

	/* Clear the reset causes now that we've read them */
	MEC1322_VBAT_STS |= status;
	MEC1322_PCR_CHIP_PWR_RST |= rst_sts;

	/*
	* BIT[6] determine VCC1 reset
	*/
	if (rst_sts & MEC1322_PWR_RST_STS_VCC1)
		flags |= RESET_FLAG_RESET_PIN;


	flags |= MEC1322_VBAT_RAM(HIBDATA_INDEX_SAVED_RESET_FLAGS);
	MEC1322_VBAT_RAM(HIBDATA_INDEX_SAVED_RESET_FLAGS) = 0;

	if ((status & MEC1322_VBAT_STS_WDT) && !(flags & (RESET_FLAG_SOFT |
					    RESET_FLAG_HARD |
					    RESET_FLAG_HIBERNATE)))
		flags |= RESET_FLAG_WATCHDOG;

	system_set_reset_flags(flags);
}

/* TODO(crbug.com/40789): Rename this function system_is_reboot_warm */
int gpio_is_reboot_warm(void)
{
	uint32_t reset_flags;
	/*
	* Check reset cause here,
	* gpio_pre_init is executed faster than system_pre_init
	*/
	check_reset_cause();
	reset_flags = system_get_reset_flags();

	if ((reset_flags & RESET_FLAG_RESET_PIN) ||
		(reset_flags & RESET_FLAG_POWER_ON) ||
		(reset_flags & RESET_FLAG_WATCHDOG) ||
		(reset_flags & RESET_FLAG_HARD) ||
		(reset_flags & RESET_FLAG_SOFT))
		return 0;
	else
		return 1;
}

void system_pre_init(void)
{
	/* Enable direct NVIC */
	MEC1322_EC_INT_CTRL |= 1;

	/* Disable ARM TRACE debug port */
	MEC1322_EC_TRACE_EN &= ~1;

	/* Deassert nSIO_RESET */
	MEC1322_PCR_PWR_RST_CTL &= ~(1 << 0);

	spi_enable(1);
}

void _system_reset(int flags, int wake_from_hibernate)
{
	uint32_t save_flags = 0;

	/* Disable interrupts to avoid task swaps during reboot */
	interrupt_disable();

	/* Save current reset reasons if necessary */
	if (flags & SYSTEM_RESET_PRESERVE_FLAGS)
		save_flags = system_get_reset_flags() | RESET_FLAG_PRESERVED;

	if (flags & SYSTEM_RESET_LEAVE_AP_OFF)
		save_flags |= RESET_FLAG_AP_OFF;

	if (wake_from_hibernate)
		save_flags |= RESET_FLAG_HIBERNATE;
	else if (flags & SYSTEM_RESET_HARD)
		save_flags |= RESET_FLAG_HARD;
	else
		save_flags |= RESET_FLAG_SOFT;

	if (MEC1322_INT_SOURCE(17) & MEC1322_INT_SOURCE_HTIMER) {
		save_flags |= RESET_FLAG_TIMER;
		/* Clear the bit */
		MEC1322_INT_SOURCE(17) |= MEC1322_INT_SOURCE_HTIMER;
	}

	MEC1322_VBAT_RAM(HIBDATA_INDEX_SAVED_RESET_FLAGS) = save_flags;

	/* Trigger watchdog in 1ms */
	MEC1322_WDG_LOAD = 1;
	MEC1322_WDG_CTL |= 1;

	/* Spin and wait for reboot; should never return */
	while (1)
		;
}

void system_reset(int flags)
{
	_system_reset(flags, 0);
}

const char *system_get_chip_vendor(void)
{
	return "smsc";
}

const char *system_get_chip_name(void)
{
	switch (MEC1322_CHIP_DEV_ID) {
	case 0x15:
		return "mec1322";
	default:
		return "unknown";
	}
}

static char to_hex(int x)
{
	if (x >= 0 && x <= 9)
		return '0' + x;
	return 'a' + x - 10;
}

const char *system_get_chip_revision(void)
{
	static char buf[3];
	uint8_t rev = MEC1322_CHIP_DEV_REV;

	buf[0] = to_hex(rev / 16);
	buf[1] = to_hex(rev & 0xf);
	buf[2] = '\0';
	return buf;
}

int system_get_vbnvcontext(uint8_t *block)
{
	return EC_ERROR_UNIMPLEMENTED;
}

int system_set_vbnvcontext(const uint8_t *block)
{
	return EC_ERROR_UNIMPLEMENTED;
}

int system_set_scratchpad(uint32_t value)
{
	MEC1322_VBAT_RAM(HIBDATA_INDEX_SCRATCHPAD) = value;
	return EC_SUCCESS;
}

uint32_t system_get_scratchpad(void)
{
	return MEC1322_VBAT_RAM(HIBDATA_INDEX_SCRATCHPAD);
}

/*
 * This runs in a single thread environment. Interrupts are disabled.
 */
static void check_battery(void)
{
	int battery_is_low = 0;
	struct batt_params batt;
	int batt_level_shutdown;
	const struct battery_info *batt_info;

	/* Gather info */
	batt_level_shutdown = board_set_battery_level_shutdown();
	batt_info = battery_get_info();
	battery_get_params(&batt);

	/*
	 * Check soc & VBAT.
	 *
	 * We shouldn't need to check if AC is plugged or a battery is being
	 * charged because we get here only by timer wake-up.
	 *
	 * That is, if AC is plugged, EC doesn't enter the battery probe loop.
	 * If AC is plugged after entering the loop, EC wakes up by AC instead
	 * of timer.
	 */
	if (!(batt.flags & BATT_FLAG_BAD_STATE_OF_CHARGE) &&
			batt.state_of_charge < batt_level_shutdown)
		battery_is_low = 1;
	else if (!(batt.flags & BATT_FLAG_BAD_VOLTAGE) &&
			batt.voltage <= batt_info->voltage_min)
		battery_is_low = 1;

	CPRINTS("Battery %s (%d%%, %dmV)", battery_is_low ? "low" : "ok",
			batt.state_of_charge, batt.voltage);

	if (!battery_is_low)
		return;

	board_cut_off_battery();
	CPRINTS("Battery cutoff");
	/*
	 *  While looping here causes WDT to trigger. So, we let rest of the
	 *  code run until EC naturally loses power (as done in other places).
	 */
}

void system_hibernate(uint32_t seconds, uint32_t microseconds)
{
	int i;
	int htimer = 0;

	CPRINTS("%s(%d, %d)", __func__, seconds, microseconds);
	if (seconds || microseconds) {
		if (seconds > 2) {
			MEC1322_HTIMER_CONTROL = 1;
			htimer = seconds * 8 + microseconds / 125000;
		} else {
			MEC1322_HTIMER_CONTROL = 0;
			htimer = (seconds * 1000000 + microseconds) * 2 / 71;
		}
		if (htimer > UINT16_MAX) {
			CPRINTS("Invalid HTIMER_PRELOAD");
			return;
		}
	}
	cflush();

	if (board_hibernate)
		board_hibernate();

	interrupt_disable();
	prepare_for_deep_sleep();

	/* Setup GPIOs for hibernate */
	if (board_set_gpio_hibernate_state)
		board_set_gpio_hibernate_state();

#ifdef CONFIG_USB_PD_PORT_COUNT
	/*
	 * Leave USB-C charging enabled in hibernate, in order to
	 * allow wake-on-plug. 5V enable must be pulled low.
	 */
#if CONFIG_USB_PD_PORT_COUNT > 0
	gpio_set_flags(GPIO_USB_C0_5V_EN, GPIO_PULL_DOWN | GPIO_INPUT);
	gpio_set_level(GPIO_USB_C0_CHARGE_EN_L, 0);
#endif
#if CONFIG_USB_PD_PORT_COUNT > 1
	gpio_set_flags(GPIO_USB_C1_5V_EN, GPIO_PULL_DOWN | GPIO_INPUT);
	gpio_set_level(GPIO_USB_C1_CHARGE_EN_L, 0);
#endif
#endif /* CONFIG_USB_PD_PORT_COUNT */

	if (hibernate_wake_pins_used > 0) {
		for (i = 0; i < hibernate_wake_pins_used; ++i) {
			const enum gpio_signal *pin = &hibernate_wake_pins[i];
			gpio_set_flags_by_mask(gpio_list[*pin].port,
					       gpio_list[*pin].mask,
					       gpio_list[*pin].flags);
			gpio_enable_interrupt(*pin);
		}

		task_enable_irq(MEC1322_IRQ_GIRQ8);
		task_enable_irq(MEC1322_IRQ_GIRQ9);
		task_enable_irq(MEC1322_IRQ_GIRQ10);
		task_enable_irq(MEC1322_IRQ_GIRQ11);
		task_enable_irq(MEC1322_IRQ_GIRQ20);
	}

	if (htimer) {
		MEC1322_INT_BLK_EN |= 1 << 17;
		MEC1322_INT_ENABLE(17) |= MEC1322_INT_SOURCE_HTIMER;
		task_enable_irq(MEC1322_IRQ_HTIMER);
		MEC1322_HTIMER_PRELOAD = htimer;
		/* Clear source register so that we will know for sure we
		 * woke up by *NEW* timer expiration. */
		MEC1322_INT_SOURCE(17) |= MEC1322_INT_SOURCE_HTIMER;
	}

	asm("wfi");

	while (MEC1322_INT_SOURCE(17) & MEC1322_INT_SOURCE_HTIMER) {
		resume_from_deep_sleep();
		interrupt_enable();
		check_battery();
		cflush();
		interrupt_disable();
		prepare_for_deep_sleep();
		MEC1322_HTIMER_PRELOAD = htimer;
		MEC1322_INT_SOURCE(17) |= MEC1322_INT_SOURCE_HTIMER;
		asm("wfi");
	}

	/* Use 48MHz clock to speed through wake-up */
	MEC1322_PCR_PROC_CLK_CTL = 1;

	/* Reboot */
	_system_reset(0, 1);

	/* We should never get here. */
	while (1)
		;
}

void htimer_interrupt(void)
{
	/* Time to wake up */
}
DECLARE_IRQ(MEC1322_IRQ_HTIMER, htimer_interrupt, 1);

enum system_image_copy_t system_get_shrspi_image_copy(void)
{
	return MEC1322_VBAT_RAM(MEC1322_IMAGETYPE_IDX);
}

uint32_t system_get_lfw_address(void)
{
	uint32_t * const lfw_vector = (uint32_t * const) CONFIG_FLASH_BASE;

	return *(lfw_vector + 1);
}

void system_set_image_copy(enum system_image_copy_t copy)
{
	MEC1322_VBAT_RAM(MEC1322_IMAGETYPE_IDX) = (copy == SYSTEM_IMAGE_RW) ?
				SYSTEM_IMAGE_RW : SYSTEM_IMAGE_RO;
}
