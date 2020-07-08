/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Drawcia board-specific configuration */

#include "button.h"
#include "charge_manager.h"
#include "charge_state_v2.h"
#include "charger.h"
#include "driver/accel_lis2dh.h"
#include "driver/accelgyro_lsm6dsm.h"
#include "driver/bc12/pi3usb9201.h"
#include "driver/charger/sm5803.h"
#include "driver/sync.h"
#include "driver/temp_sensor/thermistor.h"
#include "driver/tcpm/it83xx_pd.h"
#include "driver/tcpm/ps8xxx.h"
#include "driver/usb_mux/it5205.h"
#include "gpio.h"
#include "hooks.h"
#include "intc.h"
#include "keyboard_scan.h"
#include "lid_switch.h"
#include "power.h"
#include "power_button.h"
#include "pwm.h"
#include "pwm_chip.h"
#include "switch.h"
#include "system.h"
#include "tablet_mode.h"
#include "task.h"
#include "tcpci.h"
#include "temp_sensor.h"
#include "uart.h"
#include "usb_charge.h"
#include "usb_mux.h"
#include "usb_pd.h"
#include "usb_pd_tcpm.h"

#define CPRINTUSB(format, args...) cprints(CC_USBCHARGE, format, ## args)

#define INT_RECHECK_US 5000

/* GPIO to enable/disable the USB Type-A port. */
const int usb_port_enable[USB_PORT_COUNT] = {
	GPIO_EN_USB_A_5V,
};

/* C0 interrupt line shared by BC 1.2 and charger */
static void check_c0_line(void);
DECLARE_DEFERRED(check_c0_line);

static void notify_c0_chips(void)
{
	task_set_event(TASK_ID_USB_CHG_P0, USB_CHG_EVENT_BC12, 0);
	sm5803_interrupt(0);
}

static void check_c0_line(void)
{
	/*
	 * If line is still being held low, see if there's more to process from
	 * one of the chips
	 */
	if (!gpio_get_level(GPIO_USB_C0_INT_ODL)) {
		notify_c0_chips();
		hook_call_deferred(&check_c0_line_data, INT_RECHECK_US);
	}
}

static void usb_c0_interrupt(enum gpio_signal s)
{
	/* Cancel any previous calls to check the interrupt line */
	hook_call_deferred(&check_c0_line_data, -1);

	/* Notify all chips using this line that an interrupt came in */
	notify_c0_chips();

	/* Check the line again in 5ms */
	hook_call_deferred(&check_c0_line_data, INT_RECHECK_US);
}

/* C1 interrupt line shared by BC 1.2, TCPC, and charger */
static void check_c1_line(void);
DECLARE_DEFERRED(check_c1_line);

static void notify_c1_chips(void)
{
	schedule_deferred_pd_interrupt(1);
	task_set_event(TASK_ID_USB_CHG_P1, USB_CHG_EVENT_BC12, 0);
	sm5803_interrupt(1);
}

static void check_c1_line(void)
{
	/*
	 * If line is still being held low, see if there's more to process from
	 * one of the chips.
	 */
	if (!gpio_get_level(GPIO_USB_C1_INT_ODL)) {
		notify_c1_chips();
		hook_call_deferred(&check_c1_line_data, INT_RECHECK_US);
	}
}

static void usb_c1_interrupt(enum gpio_signal s)
{
	/* Cancel any previous calls to check the interrupt line */
	hook_call_deferred(&check_c1_line_data, -1);

	/* Notify all chips using this line that an interrupt came in */
	notify_c1_chips();

	/* Check the line again in 5ms */
	hook_call_deferred(&check_c1_line_data, INT_RECHECK_US);
}

static void c0_ccsbu_ovp_interrupt(enum gpio_signal s)
{
	cprints(CC_USBPD, "C0: CC OVP, SBU OVP, or thermal event");
	pd_handle_cc_overvoltage(0);
}

/* Must come after other header files and interrupt handler declarations */
#include "gpio_list.h"

/* BC 1.2 chips */
const struct pi3usb9201_config_t pi3usb9201_bc12_chips[] = {
	{
		.i2c_port = I2C_PORT_USB_C0,
		.i2c_addr_flags = PI3USB9201_I2C_ADDR_3_FLAGS,
		.flags = PI3USB9201_ALWAYS_POWERED,
	},
	{
		.i2c_port = I2C_PORT_SUB_USB_C1,
		.i2c_addr_flags = PI3USB9201_I2C_ADDR_3_FLAGS,
		.flags = PI3USB9201_ALWAYS_POWERED,
	},
};

/* Charger chips */
const struct charger_config_t chg_chips[] = {
	[CHARGER_PRIMARY] = {
		.i2c_port = I2C_PORT_USB_C0,
		.i2c_addr_flags = SM5803_ADDR_CHARGER_FLAGS,
		.drv = &sm5803_drv,
	},
	[CHARGER_SECONDARY] = {
		.i2c_port = I2C_PORT_SUB_USB_C1,
		.i2c_addr_flags = SM5803_ADDR_CHARGER_FLAGS,
		.drv = &sm5803_drv,
	},
};
const unsigned int chg_cnt = ARRAY_SIZE(chg_chips);

/* TCPCs */
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_MAX_COUNT] = {
	{
		.bus_type = EC_BUS_TYPE_EMBEDDED,
		.drv = &it83xx_tcpm_drv,
	},
	{
		.bus_type = EC_BUS_TYPE_I2C,
		.i2c_info = {
			.port = I2C_PORT_SUB_USB_C1,
			.addr_flags = PS8751_I2C_ADDR1_FLAGS,
		},
		.drv = &ps8xxx_tcpm_drv,
		.flags = TCPC_FLAGS_TCPCI_REV2_0,
	},
};

/* USB Muxes */
const struct usb_mux usb_muxes[CONFIG_USB_PD_PORT_MAX_COUNT] = {
	{
		.usb_port = 0,
		.i2c_port = I2C_PORT_USB_C0,
		.i2c_addr_flags = IT5205_I2C_ADDR1_FLAGS,
		.driver = &it5205_usb_mux_driver,
	},
	{
		.usb_port = 1,
		.i2c_port = I2C_PORT_SUB_USB_C1,
		.i2c_addr_flags = PS8751_I2C_ADDR1_FLAGS,
		.driver = &tcpci_tcpm_usb_mux_driver,
		.hpd_update = &ps8xxx_tcpc_update_hpd_status,
	},
};

void board_init(void)
{
	int on;

	gpio_enable_interrupt(GPIO_USB_C0_INT_ODL);
	gpio_enable_interrupt(GPIO_USB_C1_INT_ODL);
	gpio_enable_interrupt(GPIO_USB_C0_CCSBU_OVP_ODL);

	/* Charger on the MB will be outputting PROCHOT_ODL and OD CHG_DET */
	sm5803_configure_gpio0(CHARGER_PRIMARY, GPIO0_MODE_PROCHOT, 1);
	sm5803_configure_chg_det_od(CHARGER_PRIMARY, 1);

	/* Charger on the sub-board will be a push-pull GPIO */
	sm5803_configure_gpio0(CHARGER_SECONDARY, GPIO0_MODE_OUTPUT, 0);

	/* Turn on 5V if the system is on, otherwise turn it off */
	on = chipset_in_state(CHIPSET_STATE_ON | CHIPSET_STATE_ANY_SUSPEND);
	board_power_5v_enable(on);
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

void board_reset_pd_mcu(void)
{
	/*
	 * Nothing to do.  TCPC C0 is internal, TCPC C1 reset pin is not
	 * connected to the EC.
	 */
}

__override void board_power_5v_enable(int enable)
{
	/*
	 * Motherboard has a GPIO to turn on the 5V regulator, but the sub-board
	 * sets it through the charger GPIO.
	 */
	gpio_set_level(GPIO_EN_PP5000, !!enable);
	if (sm5803_set_gpio0_level(1, !!enable))
		CPRINTUSB("Failed to %sable sub rails!", enable ? "en" : "dis");
}

uint16_t tcpc_get_alert_status(void)
{
	/*
	 * TCPC 0 is embedded in the EC and processes interrupts in the chip
	 * code (it83xx/intc.c)
	 */

	uint16_t status = 0;
	int regval;

	/* Check whether TCPC 1 pulled the shared interrupt line */
	if (!gpio_get_level(GPIO_USB_C1_INT_ODL)) {
		if (!tcpc_read16(1, TCPC_REG_ALERT, &regval)) {
			if (regval)
				status = PD_STATUS_TCPC_ALERT_1;
		}
	}

	return status;
}

int extpower_is_present(void)
{
	int chg0 = 0;
	int chg1 = 0;

	sm5803_get_chg_det(0, &chg0);
	sm5803_get_chg_det(1, &chg1);

	return chg0 || chg1;
}

void board_set_charge_limit(int port, int supplier, int charge_ma, int max_ma,
			    int charge_mv)
{
	int icl = MAX(charge_ma, CONFIG_CHARGER_INPUT_CURRENT);

	/*
	 * TODO(b/151955431): Characterize the input current limit in case a
	 * scaling needs to be applied here
	 */
	charge_set_input_current_limit(icl, charge_mv);
}

int board_set_active_charge_port(int port)
{
	int is_valid_port = (port >= 0 && port < CONFIG_USB_PD_PORT_MAX_COUNT);
	int p0_otg, p1_otg;

	if (!is_valid_port && port != CHARGE_PORT_NONE)
		return EC_ERROR_INVAL;

	/* TODO(b/147440290): charger functions should take chgnum */
	p0_otg = chg_chips[0].drv->is_sourcing_otg_power(0, 0);
	p1_otg = chg_chips[1].drv->is_sourcing_otg_power(1, 1);

	if (port == CHARGE_PORT_NONE) {
		CPRINTUSB("Disabling all charge ports");

		if (!p0_otg)
			chg_chips[0].drv->set_mode(0,
						   CHARGE_FLAG_INHIBIT_CHARGE);
		if (!p1_otg)
			chg_chips[1].drv->set_mode(1,
						   CHARGE_FLAG_INHIBIT_CHARGE);

		return EC_SUCCESS;
	}

	CPRINTUSB("New chg p%d", port);

	/*
	 * Charger task will take care of enabling charging on the new charge
	 * port.  Here, we ensure the other port is not charging by changing
	 * CHG_EN
	 */
	if (port == 0) {
		if (p0_otg) {
			CPRINTUSB("Skip enable p%d", port);
			return EC_ERROR_INVAL;
		}
		if (!p1_otg) {
			chg_chips[1].drv->set_mode(1,
						   CHARGE_FLAG_INHIBIT_CHARGE);
		}
	} else {
		if (p1_otg) {
			CPRINTUSB("Skip enable p%d", port);
			return EC_ERROR_INVAL;
		}
		if (!p0_otg) {
			chg_chips[0].drv->set_mode(0,
						   CHARGE_FLAG_INHIBIT_CHARGE);
		}
	}

	return EC_SUCCESS;
}

/* Vconn control for integrated ITE TCPC */
void board_pd_vconn_ctrl(int port, enum usbpd_cc_pin cc_pin, int enabled)
{
	/* Vconn control is only for port 0 */
	if (port)
		return;

	if (cc_pin == USBPD_CC_PIN_1)
		gpio_set_level(GPIO_EN_USB_C0_CC1_VCONN, !!enabled);
	else
		gpio_set_level(GPIO_EN_USB_C0_CC2_VCONN, !!enabled);
}

__override void typec_set_source_current_limit(int port, enum tcpc_rp_value rp)
{
	int current;

	if (port < 0 || port > CONFIG_USB_PD_PORT_MAX_COUNT)
		return;

	current = (rp == TYPEC_RP_3A0) ? 3000 : 1500;

	chg_chips[port].drv->set_otg_current_voltage(port, current, 5000);
}

/* PWM channels. Must be in the exactly same order as in enum pwm_channel. */
const struct pwm_t pwm_channels[] = {
	[PWM_CH_KBLIGHT] = {
		.channel = 0,
		.flags = PWM_CONFIG_DSLEEP,
		.freq_hz = 10000,
	},

	[PWM_CH_LED_RED] = {
		.channel = 1,
		.flags = PWM_CONFIG_DSLEEP | PWM_CONFIG_ACTIVE_LOW,
		.freq_hz = 2400,
	},

	[PWM_CH_LED_GREEN] = {
		.channel = 2,
		.flags = PWM_CONFIG_DSLEEP | PWM_CONFIG_ACTIVE_LOW,
		.freq_hz = 2400,
	},

	[PWM_CH_LED_BLUE] = {
		.channel = 3,
		.flags = PWM_CONFIG_DSLEEP | PWM_CONFIG_ACTIVE_LOW,
		.freq_hz = 2400,
	}

};
BUILD_ASSERT(ARRAY_SIZE(pwm_channels) == PWM_CH_COUNT);

/* Sensor Mutexes */
static struct mutex g_lid_mutex;
static struct mutex g_base_mutex;

/* Sensor Data */
static struct stprivate_data g_lis2dh_data;
static struct lsm6dsm_data lsm6dsm_data = LSM6DSM_DATA;

/* Drivers */
struct motion_sensor_t motion_sensors[] = {
	[LID_ACCEL] = {
		.name = "Lid Accel",
		.active_mask = SENSOR_ACTIVE_S0_S3,
		.chip = MOTIONSENSE_CHIP_LIS2DE,
		.type = MOTIONSENSE_TYPE_ACCEL,
		.location = MOTIONSENSE_LOC_LID,
		.drv = &lis2dh_drv,
		.mutex = &g_lid_mutex,
		.drv_data = &g_lis2dh_data,
		.port = I2C_PORT_SENSOR,
		.i2c_spi_addr_flags = LIS2DH_ADDR1_FLAGS,
		.rot_standard_ref = NULL,
		.default_range = 2, /* g */
		/* We only use 2g because its resolution is only 8-bits */
		.min_frequency = LIS2DH_ODR_MIN_VAL,
		.max_frequency = LIS2DH_ODR_MAX_VAL,
		.config = {
			[SENSOR_CONFIG_EC_S0] = {
				.odr = 10000 | ROUND_UP_FLAG,
			},
			[SENSOR_CONFIG_EC_S3] = {
				.odr = 10000 | ROUND_UP_FLAG,
			},
		},
	},
	[BASE_ACCEL] = {
		.name = "Base Accel",
		.active_mask = SENSOR_ACTIVE_S0_S3,
		.chip = MOTIONSENSE_CHIP_LSM6DSM,
		.type = MOTIONSENSE_TYPE_ACCEL,
		.location = MOTIONSENSE_LOC_BASE,
		.drv = &lsm6dsm_drv,
		.mutex = &g_base_mutex,
		.drv_data = LSM6DSM_ST_DATA(lsm6dsm_data,
				MOTIONSENSE_TYPE_ACCEL),
		.int_signal = GPIO_BASE_SIXAXIS_INT_L,
		.flags = MOTIONSENSE_FLAG_INT_SIGNAL,
		.port = I2C_PORT_SENSOR,
		.i2c_spi_addr_flags = LSM6DSM_ADDR0_FLAGS,
		.rot_standard_ref = NULL,
		.default_range = 4,  /* g */
		.min_frequency = LSM6DSM_ODR_MIN_VAL,
		.max_frequency = LSM6DSM_ODR_MAX_VAL,
		.config = {
			[SENSOR_CONFIG_EC_S0] = {
				.odr = 13000 | ROUND_UP_FLAG,
				.ec_rate = 100 * MSEC,
			},
			[SENSOR_CONFIG_EC_S3] = {
				.odr = 10000 | ROUND_UP_FLAG,
				.ec_rate = 100 * MSEC,
			},
		},
	},
	[BASE_GYRO] = {
		.name = "Base Gyro",
		.active_mask = SENSOR_ACTIVE_S0_S3,
		.chip = MOTIONSENSE_CHIP_LSM6DSM,
		.type = MOTIONSENSE_TYPE_GYRO,
		.location = MOTIONSENSE_LOC_BASE,
		.drv = &lsm6dsm_drv,
		.mutex = &g_base_mutex,
		.drv_data = LSM6DSM_ST_DATA(lsm6dsm_data,
				MOTIONSENSE_TYPE_GYRO),
		.int_signal = GPIO_BASE_SIXAXIS_INT_L,
		.flags = MOTIONSENSE_FLAG_INT_SIGNAL,
		.port = I2C_PORT_SENSOR,
		.i2c_spi_addr_flags = LSM6DSM_ADDR0_FLAGS,
		.default_range = 1000 | ROUND_UP_FLAG, /* dps */
		.rot_standard_ref = NULL,
		.min_frequency = LSM6DSM_ODR_MIN_VAL,
		.max_frequency = LSM6DSM_ODR_MAX_VAL,
	},
	[VSYNC] = {
		.name = "Camera VSYNC",
		.active_mask = SENSOR_ACTIVE_S0,
		.chip = MOTIONSENSE_CHIP_GPIO,
		.type = MOTIONSENSE_TYPE_SYNC,
		.location = MOTIONSENSE_LOC_CAMERA,
		.drv = &sync_drv,
		.default_range = 0,
		.min_frequency = 0,
		.max_frequency = 1,
	},
};

const unsigned int motion_sensor_count = ARRAY_SIZE(motion_sensors);

/* Thermistors */
const struct temp_sensor_t temp_sensors[] = {
	[TEMP_SENSOR_1] = {.name = "Memory",
			   .type = TEMP_SENSOR_TYPE_BOARD,
			   .read = get_temp_3v3_51k1_47k_4050b,
			   .idx = ADC_TEMP_SENSOR_1},
	[TEMP_SENSOR_2] = {.name = "Ambient",
			   .type = TEMP_SENSOR_TYPE_BOARD,
			   .read = get_temp_3v3_51k1_47k_4050b,
			   .idx = ADC_TEMP_SENSOR_2},
};
BUILD_ASSERT(ARRAY_SIZE(temp_sensors) == TEMP_SENSOR_COUNT);

#ifndef TEST_BUILD
/* This callback disables keyboard when convertibles are fully open */
void lid_angle_peripheral_enable(int enable)
{
	int chipset_in_s0 = chipset_in_state(CHIPSET_STATE_ON);

	/*
	 * If the lid is in tablet position via other sensors,
	 * ignore the lid angle, which might be faulty then
	 * disable keyboard.
	 */
	if (tablet_get_mode())
		enable = 0;

	if (enable) {
		keyboard_scan_enable(1, KB_SCAN_DISABLE_LID_ANGLE);
	} else {
		/*
		 * Ensure that the chipset is off before disabling the keyboard.
		 * When the chipset is on, the EC keeps the keyboard enabled and
		 * the AP decides whether to ignore input devices or not.
		 */
		if (!chipset_in_s0)
			keyboard_scan_enable(0, KB_SCAN_DISABLE_LID_ANGLE);
	}
}
#endif
