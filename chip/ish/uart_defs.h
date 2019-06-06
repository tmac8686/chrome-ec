/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* UART module for ISH */

#ifndef __CROS_EC_UART_DEFS_H_
#define __CROS_EC_UART_DEFS_H_

#include <stdint.h>
#include <stddef.h>

#define UART_ERROR		-1
#define UART_BUSY		-2
#define HSU_BASE		ISH_UART_BASE
#define UART0_OFFS		(0x80)
#define UART0_BASE		(ISH_UART_BASE + UART0_OFFS)
#define UART0_SIZE		(0x80)

#define UART1_OFFS		(0x100)
#define UART1_BASE		(ISH_UART_BASE + UART1_OFFS)
#define UART1_SIZE		(0x80)

#define UART2_OFFS		(0x180)
#define UART2_BASE		(ISH_UART_BASE + UART2_OFFS)
#define UART2_SIZE		(0x80)

#define UART_REG(size, name, n)					\
	REG##size(uart_ctx[n].base +					\
		  UART_OFFSET_##name * uart_ctx[n].addr_interval)

/* Register accesses */
#define LSR(n)			UART_REG(8, LSR, n)
#define THR(n)			UART_REG(8, THR, n)
#define FOR(n)			UART_REG(32, FOR, n)
#define RBR(n)			UART_REG(8, RBR, n)
#define DLL(n)			UART_REG(8, DLL, n)
#define DLH(n)			UART_REG(8, DLH, n)
#define DLD(n)			UART_REG(8, DLD, n)
#define IER(n)			UART_REG(8, IER, n)
#define IIR(n)			UART_REG(8, IIR, n)
#define FCR(n)			UART_REG(8, FCR, n)
#define LCR(n)			UART_REG(8, LCR, n)
#define MCR(n)			UART_REG(8, MCR, n)
#define MSR(n)			UART_REG(8, MSR, n)
#define ABR(n)			UART_REG(32, ABR, n)
#define PS(n)			UART_REG(32, PS, n)
#define MUL(n)			UART_REG(32, MUL, n)
#define DIV(n)			UART_REG(32, DIV, n)

/* RBR: Receive Buffer register     (BLAB bit = 0)  */
#define UART_OFFSET_RBR	(0)
/* THR: Transmit Holding register   (BLAB bit = 0)  */
#define UART_OFFSET_THR	(0)
/* IER: Interrupt Enable register   (BLAB bit = 0)  */
#define UART_OFFSET_IER	(1)

#define FCR_FIFO_SIZE_16	(0x00)
#define FCR_FIFO_SIZE_64	(0x20)
#define FCR_ITL_FIFO_64_BYTES_1 (0x00)

/* FCR: FIFO Control register */
#define UART_OFFSET_FCR	(2)
#define FCR_FIFO_ENABLE	BIT(0)
#define FCR_RESET_RX		BIT(1)
#define FCR_RESET_TX		BIT(2)

/* LCR: Line Control register */
#define UART_OFFSET_LCR	(3)
#define LCR_DLAB		(0x80)
#define LCR_5BIT_CHR		(0x00)
#define LCR_6BIT_CHR		(0x01)
#define LCR_7BIT_CHR		(0x02)
#define LCR_8BIT_CHR		(0x03)
#define LCR_BIT_CHR_MASK	(0x03)
#define LCR_SB			(0x40)	/* Set Break */

/* MCR: Modem Control register */
#define UART_OFFSET_MCR	(4)
#define MCR_DTR		BIT(0)
#define MCR_RTS		BIT(1)
#define MCR_LOO		BIT(4)
#define MCR_INTR_ENABLE	BIT(3)
#define MCR_AUTO_FLOW_EN	BIT(5)

/* LSR: Line Status register */
#define UART_OFFSET_LSR	(5)
#define LSR_DR			BIT(0)	/* Data Ready */
#define LSR_OE			BIT(1)	/* Overrun error */
#define LSR_PE			BIT(2)	/* Parity error */
#define LSR_FE			BIT(3)	/* Framing error */
#define LSR_BI			BIT(4)	/* Breaking interrupt */
#define LSR_THR_EMPTY		BIT(5)	/* Non FIFO mode: Transmit holding
					 * register empty
					 */
#define LSR_TDRQ		BIT(5)	/* FIFO mode: Transmit Data request */
#define LSR_TEMT		BIT(6)	/* Transmitter empty */

#define FCR_ITL_FIFO_64_BYTES_56 (BIT(6) | BIT(7))

#define IER_RECV		BIT(0)
#define IER_TDRQ		BIT(1)
#define IER_LINE_STAT		BIT(2)

#define UART_OFFSET_IIR	(2)
/* MSR: Modem Status register */
#define UART_OFFSET_MSR	(6)

/* DLL: Divisor Latch Reg. low byte  (BLAB bit = 1) */
#define UART_OFFSET_DLL	(0)

/* DLH: Divisor Latch Reg. high byte (BLAB bit = 1) */
#define UART_OFFSET_DLH	(1)

/* DLH: Divisor Latch Fractional. (BLAB bit = 1) */
#define UART_OFFSET_DLD	(2)

/* FOR: Fifo O Register (ISH only) */
#define UART_OFFSET_FOR	(0x20)
#define FOR_OCCUPANCY_OFFS	0
#define FOR_OCCUPANCY_MASK	0x7F

/* ABR: Auto-Baud Control Register (ISH only) */
#define UART_OFFSET_ABR	(0x24)
#define ABR_UUE		BIT(4)

/* Pre-Scalar Register (ISH only) */
#define UART_OFFSET_PS		(0x30)

/* DDS registers (ISH only) */
#define UART_OFFSET_MUL	(0x34)
#define UART_OFFSET_DIV	(0x38)

/* G_IEN: Global Interrupt Enable  (ISH only) */
#define HSU_REG_GIEN		REG32(HSU_BASE + 0x0)
#define HSU_REG_GIST		REG32(HSU_BASE + 0x4)

#define GIEN_PWR_MGMT		BIT(24)
#define GIEN_DMA_EN		BIT(5)
#define GIEN_UART2_EN		BIT(2)
#define GIEN_UART1_EN		BIT(1)
#define GIEN_UART0_EN		BIT(0)
#define GIST_DMA_EN		BIT(5)
#define GIST_UART2_EN		BIT(2)
#define GIST_UART1_EN		BIT(1)
#define GIST_UART0_EN		BIT(0)
#define GIST_UARTx_EN		(GIST_UART0_EN|GIST_UART1_EN|GIST_UART2_EN)

/* UART config flag, send to sc_io_control if the current UART line has HW
 * flow control lines connected.
 */
#define UART_CONFIG_HW_FLOW_CONTROL		BIT(0)

/* UART config flag for sc_io_control.  If defined a sc_io_event_rx_msg is
 * raised only when the rx buffer is completely full. Otherwise, the event
 * is raised after a timeout is received on the UART line,
 * and all data received until now is provided.
 */
#define UART_CONFIG_DELIVER_FULL_RX_BUF	BIT(1)

/* UART config flag for sc_io_control.  If defined a sc_io_event_rx_buf_depleted
 * is raised when all rx buffers that were added are full. Otherwise, no
 * event is raised.
 */
#define UART_CONFIG_ANNOUNCE_DEPLETED_BUF	BIT(2)

#define UART_INT_DEVICES		3
#define UART_EXT_DEVICES		8
#define UART_DEVICES			UART_INT_DEVICES
#define UART_ISH_ADDR_INTERVAL		1

#define B9600				0x0000d
#define B57600				0x00000018
#define B115200			0x00000011
#define B921600			0x00000012
#define B2000000			0x00000013
#define B3000000			0x00000014
#define B3250000			0x00000015
#define B3500000			0x00000016
#define B4000000			0x00000017
#define B19200				0x0000e
#define B38400				0x0000f

/* KHZ, MHZ */
#define KHZ(x)				((x) * 1000)
#define MHZ(x)				(KHZ(x) * 1000)
#if defined(CHIP_FAMILY_ISH3) || defined(CHIP_FAMILY_ISH5)
#define UART_ISH_INPUT_FREQ		MHZ(120)
#elif defined(CHIP_FAMILY_ISH4)
#define UART_ISH_INPUT_FREQ		MHZ(100)
#endif
#define UART_DEFAULT_BAUD_RATE		115200
#define UART_STATE_CG			BIT(UART_OP_CG)

enum UART_PORT {
	UART_PORT_0,
	UART_PORT_1,
	UART_PORT_MAX
};

enum UART_OP {
	UART_OP_READ,
	UART_OP_WRITE,
	UART_OP_CG,
	UART_OP_MAX
};

enum {
	BAUD_IDX,
	BAUD_SPEED,
	BAUD_TABLE_MAX
};

struct uart_ctx {
	uint32_t id;
	uint32_t base;
	uint32_t addr_interval;
	uint32_t uart_state;
	uint32_t is_open;
	uint32_t baud_rate;
	uint32_t input_freq;
	uint32_t client_flags;
};

#endif /* _CROS_EC_UART_DEFS_H_ */
