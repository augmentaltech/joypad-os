// Wired-UART message-output input — a parallel path to the BLE 6E40FE01 service.
//
// A USB-to-UART adapter wired to the board's msgout UART pins writes the SAME
// ASCII tokens the BLE characteristic accepts, whitespace-delimited (one token
// per field, e.g. "A+ RX=200 ZL+\n"). Each token is handed to the shared
// msgout_handle_token(), so both transports share the token grammar and the
// button hold/release/tap + analog-stick semantics.
//
// Enabled per-board by a devicetree chosen node:
//     chosen { joypad,msgout-uart = &uart1; };
// Absent -> msgout_uart_init() is a no-op (the BLE path is unaffected).
//
// The UART RX ISR only buffers bytes into a ring buffer; framing + dispatch run
// in a workqueue item, because msgout_handle_token() submits to the router and
// logs — neither is safe from ISR context.

#include "msgout.h"

#include <zephyr/kernel.h>

#if DT_HAS_CHOSEN(joypad_msgout_uart)

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

static const struct device *const msgout_uart_dev =
	DEVICE_DT_GET(DT_CHOSEN(joypad_msgout_uart));

RING_BUF_DECLARE(msgout_rx_rb, 256);

// Runs in the system workqueue (serialized), so the framing state is safe and
// msgout_handle_token() executes in thread context.
static void msgout_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	static uint8_t tok[24];   // matches the tok[] cap in msgout_handle_token()
	static size_t  tok_len;
	uint8_t c;

	while (ring_buf_get(&msgout_rx_rb, &c, 1) == 1) {
		if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
			if (tok_len > 0) {
				msgout_handle_token(tok, (uint16_t)tok_len);
				status_led_note_uart();
				tok_len = 0;
			}
		} else if (tok_len < sizeof(tok)) {
			tok[tok_len++] = c;
		} else {
			tok_len = 0;  // oversized token: drop, resync on next delimiter
		}
	}
}
static K_WORK_DEFINE(msgout_work, msgout_work_handler);

static void msgout_uart_isr(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);
	if (!uart_irq_update(dev)) {
		return;
	}
	bool got = false;
	uint8_t c;
	while (uart_irq_rx_ready(dev) && uart_fifo_read(dev, &c, 1) == 1) {
		ring_buf_put(&msgout_rx_rb, &c, 1);  // overflow drops the byte
		got = true;
	}
	if (got) {
		k_work_submit(&msgout_work);
	}
}

void msgout_uart_init(void)
{
	if (!device_is_ready(msgout_uart_dev)) {
		printk("[msgout-uart] %s not ready\n", msgout_uart_dev->name);
		return;
	}
	uart_irq_callback_user_data_set(msgout_uart_dev, msgout_uart_isr, NULL);
	uart_irq_rx_enable(msgout_uart_dev);
	printk("[msgout-uart] ready on %s (parallel to BLE message-output)\n",
	       msgout_uart_dev->name);
}

#else  // no msgout UART selected for this board

void msgout_uart_init(void)
{
	// Wired-UART message-output path not configured (no joypad,msgout-uart chosen).
}

#endif
