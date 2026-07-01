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
void msgout_handle_token(const uint8_t *data, uint16_t len);

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
