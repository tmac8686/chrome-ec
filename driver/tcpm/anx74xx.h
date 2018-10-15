/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Author : Analogix Semiconductor.
 */

/* USB Power delivery port management */

#ifndef __CROS_EC_USB_PD_TCPM_ANX74XX_H
#define __CROS_EC_USB_PD_TCPM_ANX74XX_H

/* I2C interface */
#define ANX74XX_I2C_ADDR1 0x50
#define ANX74XX_I2C_ADDR2 0x72
#define ANX74XX_I2C_ADDR3 0x7C
#define ANX74XX_I2C_ADDR4 0x80

#define ANX74XX_REG_IRQ_POL_LOW		0x00
#define ANX74XX_REG_IRQ_POL_HIGH		0x02

#define ANX74XX_REG_VENDOR_ID_L         0x00
#define ANX74XX_REG_VENDOR_ID_H         0x01
#define ANX74XX_VENDOR_ID               0xAAAA

/* ANX F/W version：0x50:0x44 which contains otp firmware version */
#define ANX74XX_REG_FW_VERSION          0x44

#define ANX74XX_REG_IRQ_STATUS		0x53

#define ANX74XX_REG_INTP_VCONN_CTRL	0x33
#define ANX74XX_REG_VCONN_DISABLE		0x0f
#define ANX74XX_REG_VCONN_1_ENABLE		(1 << 4)
#define ANX74XX_REG_VCONN_2_ENABLE		(1 << 5)
#define ANX74XX_REG_R_INTERRUPT_OPEN_DRAIN	(1 << 2)

#define ANX74XX_STANDBY_MODE		(0)
#define ANX74XX_NORMAL_MODE		(1)

#define ANX74XX_REG_TX_CTRL_1		0x81
#define ANX74XX_REG_TX_HARD_RESET_REQ	(1 << 1)
#define ANX74XX_REG_TX_CABLE_RESET_REQ	(1 << 2)

#define ANX74XX_REG_TX_CTRL_2		0x82
#define ANX74XX_REG_TX_WR_FIFO		0x83
#define ANX74XX_REG_TX_FIFO_CTRL		0x9a
#define ANX74XX_REG_TX_HEADER_L		0x2c
#define ANX74XX_REG_TX_HEADER_H		0x2d
#define ANX74XX_REG_TX_START_ADDR_0		0x6d
#define ANX74XX_REG_TX_START_ADDR_1		0xd0

#define ANX74XX_REG_CTRL_COMMAND		0xdb
#define ANX74XX_REG_TX_SEND_DATA_REQ	(1 << 0)
#define ANX74XX_REG_TX_HARD_RST_REQ	(1 << 1)

#define ANX74XX_REG_TX_BIST_CTRL 0x9D
#define ANX74XX_REG_TX_BIST_MODE (1 << 4)
#define ANX74XX_REG_TX_BIST_STOP (1 << 3)
#define ANX74XX_REG_TX_BIXT_FOREVER (1 << 2)
#define ANX74XX_REG_TX_BIST_ENABLE (1 << 1)
#define ANX74XX_REG_TX_BIST_START (1 << 0)

#define ANX74XX_REG_PD_HEADER		0x69
#define ANX74XX_REG_PD_RX_DATA_OBJ		0x11
#define ANX74XX_REG_PD_RX_DATA_OBJ_M	0x4d

#define ANX74XX_REG_ANALOG_STATUS		0x40
#define ANX74XX_REG_VBUS_STATUS		(1 << 4)
#define ANX74XX_REG_CC_PULL_RD		0xfd
#define ANX74XX_REG_CC_PULL_RP		0x02


#define ANX74XX_REG_TX_AUTO_GOODCRC_2	0x94
#define ANX74XX_REG_REPLY_SOP_EN        (1 << 3)
#define ANX74XX_REG_REPLY_SOP_1_EN      (1 << 4)
#define ANX74XX_REG_REPLY_SOP_2_EN      (1 << 5)

#define ANX74XX_REG_TX_AUTO_GOODCRC_1	0x9c
#define ANX74XX_REG_SPEC_REV_BIT_POS	(3)
#define ANX74XX_REG_DATA_ROLE_BIT_POS	(2)
#define ANX74XX_REG_PWR_ROLE_BIT_POS	(1)
#define ANX74XX_REG_AUTO_GOODCRC_EN	(1 << 0)
#define ANX74XX_REG_AUTO_GOODCRC_SET(drole, prole)    \
	((PD_REV20 << ANX74XX_REG_SPEC_REV_BIT_POS) | \
	((drole) << ANX74XX_REG_DATA_ROLE_BIT_POS)  | \
	((prole) << ANX74XX_REG_PWR_ROLE_BIT_POS)   | \
	ANX74XX_REG_AUTO_GOODCRC_EN)


#define ANX74XX_REG_ANALOG_CTRL_0		0x41
#define ANX74XX_REG_R_PIN_CABLE_DET		(1 << 7)

#define ANX74XX_REG_ANALOG_CTRL_1		0x42
#define ANX74XX_REG_ANALOG_CTRL_5		0x46
#define ANX74XX_REG_ANALOG_CTRL_6		0x47
#define ANX74XX_REG_CC_PULL_RP_36K	0x00
#define ANX74XX_REG_CC_PULL_RP_12K	0x01
#define ANX74XX_REG_CC_PULL_RP_4K	0x02

#define ANX74XX_REG_R_SWITCH_CC_CLR	0x0f
#define ANX74XX_REG_R_SWITCH_CC2_SET	0x10
#define ANX74XX_REG_R_SWITCH_CC1_SET	0x20
#define ANX74XX_REG_AUX_SWAP_SET_CC1	0x30
#define ANX74XX_REG_AUX_SWAP_SET_CC2	0xc0

#define ANX74XX_REG_ANALOG_CTRL_11		0x4c
#define ANX74XX_REG_ANALOG_CTRL_12		0x4d

#define ANX74XX_REG_MUX_ML0_RX2		(1 << 0)
#define ANX74XX_REG_MUX_ML0_RX1		(1 << 1)
#define ANX74XX_REG_MUX_ML3_RX2		(1 << 2)
#define ANX74XX_REG_MUX_ML3_RX1		(1 << 3)
#define ANX74XX_REG_MUX_SSRX_RX2	(1 << 4)
#define ANX74XX_REG_MUX_SSRX_RX1	(1 << 5)
#define ANX74XX_REG_MUX_ML1_TX2		(1 << 6)
#define ANX74XX_REG_MUX_ML1_TX1		(1 << 7)

#define ANX74XX_REG_MUX_ML2_TX2		(1 << 4)
#define ANX74XX_REG_MUX_ML2_TX1		(1 << 5)
#define ANX74XX_REG_MUX_SSTX_TX2	(1 << 6)
#define ANX74XX_REG_MUX_SSTX_TX1	(1 << 7)

#define ANX74XX_REG_CC_SOFTWARE_CTRL	0x4a
#define ANX74XX_REG_CC_SW_CTRL_ENABLE	0x01
#define ANX74XX_REG_TX_MODE_ENABLE		0x04

#define ANX74XX_REG_SELECT_CC1		0x02

#define ANX74XX_REG_GPIO_CTRL_4_5		0x3f
#define ANX74XX_REG_VBUS_OP_ENABLE		0x04
#define ANX74XX_REG_VBUS_GPIO_MODE		0xfe

#define ANX74XX_REG_IRQ_EXT_MASK_1		0x3b
#define ANX74XX_REG_IRQ_EXT_MASK_2		0x3c
#define ANX74XX_REG_IRQ_EXT_SOURCE_1	0x3e
#define ANX74XX_REG_IRQ_EXT_SOURCE_2	0x4e
#define ANX74XX_REG_EXT_HARD_RST		(1 << 2)
#define ANX74XX_REG_IRQ_EXT_SOURCE_3	0x4f
#define ANX74XX_REG_CLEAR_SOFT_IRQ		(1 << 2)

#define ANX74XX_REG_IRQ_SOURCE_RECV_MSG	0x6b
#define ANX74XX_REG_IRQ_CC_MSG_INT		(1 << 0)
#define ANX74XX_REG_IRQ_CC_STATUS_INT	(1 << 1)
#define ANX74XX_REG_IRQ_GOOD_CRC_INT	(1 << 2)
#define ANX74XX_REG_IRQ_TX_FAIL_INT	(1 << 3)
#define ANX74XX_REG_IRQ_SOURCE_RECV_MSG_MASK	0x6c

#define ANX74XX_REG_CLEAR_SET_BITS		0xff
#define ANX74XX_REG_ALERT_HARD_RST_RECV		(1 << 6)
#define ANX74XX_REG_ALERT_MSG_RECV		(1 << 5)
#define ANX74XX_REG_ALERT_TX_MSG_ERROR	(1 << 4)
#define ANX74XX_REG_ALERT_TX_ACK_RECV	(1 << 3)
#define ANX74XX_REG_ALERT_TX_CABLE_RESETOK	(1 << 2)
#define ANX74XX_REG_ALERT_TX_HARD_RESETOK	(1 << 1)
#define ANX74XX_REG_ALERT_CC_CHANGE	(1 << 0)

#define ANX74XX_REG_ANALOG_CTRL_2		0x43
#define ANX74XX_REG_MODE_TRANS		0x01

#define ANX74XX_REG_SET_VBUS		0x20

#define ANX74XX_REG_ANALOG_CTRL_7		0x48
#define ANX74XX_REG_STATUS_CC_RD		0x01
#define ANX74XX_REG_STATUS_CC_RA		0x03
#define ANX74XX_REG_STATUS_CC1(reg)	((reg & 0x0C) >> 2)
#define ANX74XX_REG_STATUS_CC2(reg)	((reg & 0x03) >> 0)

#define ANX74XX_REG_HPD_CONTROL		0xfd

#define ANX74XX_REG_HPD_CTRL_0		0x36
#define ANX74XX_REG_DISCHARGE_CTRL     0x80
#define ANX74XX_REG_HPD_OP_MODE		0x08
#define ANX74XX_REG_HPD_DEFAULT		0x00
#define ANX74XX_REG_HPD_OUT_DATA		0x10

#define ANX74XX_REG_RECVD_MSG_INT 0x98
#define ANX74XX_REG_CC_STATUS     0x99
#define ANX74XX_REG_CTRL_FW  0x2E
#define CLEAR_RX_BUFFER (1)
#define ANX74XX_REG_POWER_DOWN_CTRL	0x0d
#define ANX74XX_REG_STATUS_CC1_VRD_USB	(1 << 7)
#define ANX74XX_REG_STATUS_CC1_VRD_1P5	(1 << 6)
#define ANX74XX_REG_STATUS_CC1_VRD_3P0	(1 << 5)
#define ANX74XX_REG_STATUS_CC2_VRD_USB	(1 << 4)
#define ANX74XX_REG_STATUS_CC2_VRD_1P5	(1 << 3)
#define ANX74XX_REG_STATUS_CC2_VRD_3P0	(1 << 2)

/* defined in the inter-bock Spec: 4.2.10 CC Detect Status */
#define ANX74XX_REG_CC_STATUS_MASK  0xf
#define BIT_VALUE_OF_SRC_CC_RD      0x01
#define BIT_VALUE_OF_SRC_CC_RA      0x02
#define BIT_VALUE_OF_SNK_CC_DEFAULT 0x04
#define BIT_VALUE_OF_SNK_CC_1_P_5   0x08
#define BIT_VALUE_OF_SNK_CC_3_P_0   0x0C
#define ANX74XX_CC_RA_MASK (BIT_VALUE_OF_SRC_CC_RA | \
			    (BIT_VALUE_OF_SRC_CC_RA << 4))
#define ANX74XX_CC_RD_MASK (BIT_VALUE_OF_SRC_CC_RD | \
			    (BIT_VALUE_OF_SRC_CC_RD << 4))

/*
 * RESETN low to PWR_EN low delay
 */
#define ANX74XX_RST_L_PWR_L_DELAY_MS	1
/*
 * minimum power off-to-on delay to reset chip
 */
#define ANX74XX_PWR_L_PWR_H_DELAY_MS	10
/*
 * parameter T4: PWR_EN high to RESETN high delay
 */
#define ANX74XX_PWR_H_RST_H_DELAY_MS	10

extern const struct tcpm_drv anx74xx_tcpm_drv;
extern const struct usb_mux_driver anx74xx_tcpm_usb_mux_driver;
void anx74xx_tcpc_set_vbus(int port, int enable);
void anx74xx_tcpc_update_hpd_status(int port, int hpd_lvl, int hpd_irq);
void anx74xx_tcpc_clear_hpd_status(int port);

#ifdef CONFIG_CMD_I2C_STRESS_TEST_TCPC
extern struct i2c_stress_test_dev anx74xx_i2c_stress_test_dev;
#endif

#endif /* __CROS_EC_USB_PD_TCPM_ANX74XX_H */
