#ifndef MSGOUT_H__
#define MSGOUT_H__

#include <stdbool.h>
#include <stdint.h>

// MouthPad message-output: parse one ASCII button token and drive the router.
//
// Token grammar (case-insensitive, whitespace trimmed):
//   <name>[+|-]      button: A B X Y L R ZL ZR PLUS MINUS L3 R3 HOME CAPTURE
//                            DUP DDOWN DLEFT DRIGHT
//                    '+' hold, '-' release, no suffix = tap (90 ms auto-release)
//   <axis>=<0..255>  analog stick (128 = center, held until changed): LX LY RX RY
//   CENTER           recenter all sticks
//
// Two parallel input paths feed this single handler:
//   - BLE: the message-output characteristic 6E40FE01 (see ble_output.c)
//   - UART: a USB-to-UART adapter on the msgout UART pins (see msgout_uart.c)
//
// NOTE: this parses exactly ONE token. Do not hand it a whole multi-token frame
// (it strips whitespace rather than splitting on it, collapsing e.g.
// "B- X- CENTER" into one garbage token). Feed raw received bytes through
// msgout_feed_bytes() instead, which splits on whitespace and calls this per
// token — both transports use it so BLE and UART parse identically.
void msgout_handle_token(const uint8_t *data, uint16_t len);

// Feed received message-output bytes into the shared whitespace tokenizer
// (buffered, split on whitespace, and dispatched to msgout_handle_token() from
// the system workqueue). Used by both transports:
//   - UART: msgout_uart.c streams bytes in as the RX ISR receives them.
//   - BLE:  ble_output.c hands each ATT write to it, terminated with a newline
//           so the frame's final token flushes.
// A complete frame must end in whitespace/newline for its last token to flush
// (a bare trailing token stays buffered until the next delimiter arrives).
void msgout_feed_bytes(const uint8_t *data, uint16_t len);

// Bring up the wired-UART message-output path. No-op unless a UART is selected
// via the devicetree chosen node `joypad,msgout-uart`. Call once the router /
// output side is ready (e.g. from ble_output_late_init()).
void msgout_uart_init(void);

// Message-output -> Switch status LED (nRF DK/XIAO discrete RGB): blue =
// advertising, green = a central connected, red flash = UART token received.
// All no-op unless MSGOUT_SWITCH_BUILD (and a non-Feather board). Implemented in
// ws2812_nrf.c; drive from the BLE connect/disconnect + advertise paths and the
// UART RX handler.
void status_led_start(void);
void status_led_set_connected(bool connected);
void status_led_note_uart(void);

#endif // MSGOUT_H__
