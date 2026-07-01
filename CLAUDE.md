# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Joypad OS (formerly **USBRetro**) is firmware for RP2040 and ESP32-S3 based adapters that provides universal controller I/O. Old code/commits may reference `USBR_BUTTON_*` or `usbretro` naming.

**Inputs:**
- USB HID controllers, keyboards, mice
- USB X-input (Xbox controllers)
- Bluetooth controllers (via USB BT dongle or Pico W)
- WiFi controllers (via JOCP protocol on Pico W)
- Native controllers (SNES, N64, GameCube via joybus, LodgeNet via PIO)

**Outputs:**
- Retro consoles: PCEngine, GameCube, Dreamcast, Nuon, 3DO, Loopy
- USB Device: HID gamepad, XInput, DirectInput, PS3/PS4/Switch modes
- UART: ESP32 Bluetooth bridge

Uses TinyUSB for USB, BTstack for Bluetooth, LWIP for WiFi networking, and RP2040 PIO for timing-critical console protocols. ESP32-S3 support uses ESP-IDF with FreeRTOS for BLE-to-USB applications.

## Build Commands

### Quick Start

```bash
# One-time setup
brew install --cask gcc-arm-embedded cmake git

# Clone and initialize
git clone https://github.com/joypad-ai/joypad-os.git
cd joypad-os
make init

# Build apps (append _kb2040, _rp2040zero, etc. for specific boards)
make usb2pce_kb2040    # USB/BT → PCEngine
make usb2gc_kb2040     # USB/BT → GameCube
make usb2dc_kb2040     # USB/BT → Dreamcast
make usb2nuon_kb2040   # USB/BT → Nuon
make usb23do_rp2040zero # USB/BT → 3DO
make usb2usb_feather   # USB/BT → USB HID
make snes2usb_kb2040   # SNES → USB HID
make n642usb_kb2040    # N64 → USB HID
make gc2usb_kb2040     # GameCube → USB HID
make lodgenet2usb_pico # LodgeNet → USB HID (Pico)
make lodgenet2usb_pico2 # LodgeNet → USB HID (Pico 2)
make n642dc_kb2040     # N64 → Dreamcast
make bt2usb_pico_w     # BT-only → USB HID (Pico W)
make bt2usb_xiao_esp32s3    # BLE-only → USB HID (ESP32-S3, requires ESP-IDF)
make bt2usb_seeed_xiao_nrf52840   # BLE-only → USB HID (Seeed XIAO nRF52840, requires NCS)
make mouthpad_aprbrother_nrf52840 # Augmental MouthPad BLE → USB HID + NUS relay (April Brother dongle, NCS)
make mouthpad_pico_w              # Augmental MouthPad BLE → USB HID + NUS relay (Pico W)
make wifi2usb_pico_w   # WiFi → USB HID (Pico W)
make controller_btusb_pico_w        # GPIO+JoyWing → BLE+USB HID (Pico W)
make controller_btusb_rp2040_abb    # GPIO+USB Host → USB HID (ABB Passthrough)

# Build all (RP2040 targets only)
make all
make clean

# Flash (macOS - looks for /Volumes/RPI-RP2)
make flash              # Flash most recent build
make flash-usb2pce_kb2040  # Flash specific app

# ESP32-S3 (requires ESP-IDF, see .dev/docs/esp32-port.md)
make flash-bt2usb_xiao_esp32s3  # Flash via esptool
```

Output: `releases/joypad_<commit>_<app>_<board>.uf2`

### App Build Matrix

| App | Board | Input | Output |
|-----|-------|-------|--------|
| `usb2pce` | KB2040 | USB/BT | PCEngine |
| `usb2gc` | KB2040 | USB/BT | GameCube |
| `usb2dc` | KB2040 | USB/BT | Dreamcast |
| `usb2nuon` | KB2040 | USB/BT | Nuon |
| `usb23do` | RP2040-Zero | USB/BT | 3DO |
| `usb2loopy` | KB2040 | USB/BT | Loopy |
| `usb2usb` | Feather/RP2040-Zero | USB/BT | USB HID |
| `bt2usb` | Pico W/Pico 2 W/ESP32-S3/XIAO nRF52840 | BT/BLE | USB HID |
| `mouthpad` | April Brother nRF52840/Pico W/Pico 2 W | BLE (Augmental MouthPad) | USB HID (SInput) + NUS relay (CDC) |
| `wifi2usb` | Pico W/Pico 2 W | WiFi (JOCP) | USB HID |
| `snes2usb` | KB2040 | SNES | USB HID |
| `n642usb` | KB2040 | N64 | USB HID |
| `gc2usb` | KB2040/RP2040-Zero/Pico | GameCube | USB HID |
| `lodgenet2usb` | Pico/Pico 2 | LodgeNet (N64/GC/SNES) | USB HID |
| `n642dc` | KB2040 | N64 | Dreamcast |
| `snes23do` | RP2040-Zero | SNES | 3DO |
| `usb2uart` | KB2040 | USB | UART/ESP32 |
| `controller_*` | Various | GPIO | USB HID |
| `controller_btusb` | Pico W/Pico 2 W/ESP32-S3/nRF52840 | GPIO | BLE + USB HID |
| `controller_btusb_rp2040_abb` | RP2040 ABB (Passthrough) | GPIO + USB Host | USB HID |

## Architecture

### Repository Structure

```
src/
├── main.c                      # RP2040 entry point, main loop
├── CMakeLists.txt              # RP2040 build configuration
├── platform/
│   ├── platform.h              # Platform HAL (time, identity, reboot)
│   ├── rp2040/platform_rp2040.c
│   └── esp32/platform_esp32.c
├── core/                       # Shared firmware infrastructure
│   ├── buttons.h               # JP_BUTTON_* definitions (W3C order)
│   ├── input_event.h           # Unified input event structure
│   ├── output_interface.h      # Output abstraction
│   ├── router/                 # Input→Output routing
│   │   └── router.c/h          # SIMPLE/MERGE/BROADCAST modes
│   └── services/
│       ├── players/            # Player slot management + feedback
│       ├── profiles/           # Button remapping profiles
│       ├── codes/              # Button sequence detection
│       ├── hotkeys/            # Button combo detection
│       ├── leds/               # NeoPixel LED control
│       └── storage/            # Flash persistence
├── apps/                       # App configurations
│   ├── usb2pce/                # USB/BT → PCEngine
│   ├── usb2gc/                 # USB/BT → GameCube
│   ├── usb2nuon/               # USB/BT → Nuon
│   ├── usb23do/                # USB/BT → 3DO
│   ├── usb2loopy/              # USB/BT → Loopy
│   ├── usb2usb/                # USB/BT → USB HID
│   ├── wifi2usb/               # WiFi → USB HID (JOCP protocol)
│   ├── usb2uart/               # USB → UART bridge
│   ├── snes2usb/               # SNES → USB HID
│   ├── snes23do/               # SNES → 3DO
│   ├── lodgenet2usb/           # LodgeNet → USB HID
│   └── controller/             # Custom GPIO controllers
├── usb/
│   ├── usbh/                   # USB Host (input)
│   │   ├── hid/                # HID device drivers
│   │   │   └── devices/        # Vendor-specific drivers
│   │   └── xinput/             # X-input protocol
│   └── usbd/                   # USB Device (output)
│       ├── usbd.c/h            # USB device stack
│       ├── tud_xinput.*        # XInput output mode
│       └── descriptors/        # USB descriptors
├── bt/                         # Bluetooth support
│   ├── bthid/                  # BT HID device drivers
│   │   └── devices/            # BT controller drivers
│   ├── btstack/                # BTstack host integration
│   │   └── btstack_host.c      # HID host, scanning, pairing
│   └── transport/              # BT transport layer
│       ├── bt_transport_cyw43.c  # Pico W transport
│       └── bt_transport_esp32.c  # ESP32-S3 transport
├── wifi/                       # WiFi support (Pico W)
│   └── jocp/                   # JOCP protocol implementation
│       ├── jocp.h              # Protocol definitions
│       ├── jocp_input.c        # Packet parsing/conversion
│       ├── wifi_transport.c/h  # WiFi AP, UDP/TCP servers
│       └── dhcpserver.c/h      # DHCP server
└── native/
    ├── device/                 # Console outputs (we emulate devices)
    │   ├── pcengine/           # PCEngine multitap (PIO)
    │   ├── gamecube/           # GameCube joybus (PIO)
    │   ├── dreamcast/          # Dreamcast maple bus (PIO)
    │   ├── nuon/               # Nuon polyface (PIO)
    │   ├── 3do/                # 3DO controller (PIO)
    │   ├── loopy/              # Casio Loopy (PIO)
    │   └── uart/               # UART output
    └── host/                   # Native inputs (we read controllers)
        ├── snes/               # SNES controller reading
        ├── n64/                # N64 controller reading (joybus)
        ├── gc/                 # GameCube controller reading (joybus)
        └── lodgenet/           # LodgeNet hotel controllers (PIO, N64/GC/SNES)
esp/                                # ESP-IDF build directory (ESP32-S3)
├── CMakeLists.txt                  # ESP-IDF project file
├── Makefile                        # Build/flash/monitor shortcuts
├── env.sh                          # ESP-IDF environment activation
├── sdkconfig.defaults              # Common ESP32-S3 config
├── sdkconfig.board.devkit          # Board-specific overrides
└── main/
    ├── CMakeLists.txt              # Component registration + shared sources
    ├── main.c                      # FreeRTOS entry point
    ├── flash_esp32.c               # NVS-based flash persistence
    ├── button_esp32.c              # GPIO button driver
    ├── ws2812_esp32.c              # NeoPixel stub
    ├── btstack_hal_esp32.c         # BTstack HAL glue
    ├── btstack_config.h            # BLE-only BTstack config
    └── tusb_config_esp32.h         # ESP32-S3 TinyUSB config
nrf/                                # nRF Connect SDK build directory (nRF52840)
├── CMakeLists.txt                  # Zephyr app project file
├── Makefile                        # Build/flash/monitor shortcuts
├── prj.conf                        # Zephyr Kconfig (BT HCI raw, no Zephyr USB)
├── boards/                         # Board-specific overlays
│   ├── xiao_ble.overlay            # Disable Zephyr USBD node
│   └── xiao_ble.conf               # XIAO nRF52840 board config
└── src/
    ├── main.c                      # Zephyr entry point
    ├── flash_nrf.c                 # NVS-based flash persistence
    ├── button_nrf.c                # Button stub (no user button)
    ├── ws2812_nrf.c                # NeoPixel stub
    ├── btstack_config.h            # BLE-only BTstack config wrapper
    └── tusb_config_nrf.h           # nRF5x TinyUSB config
```

### Data Flow

```
Input Sources                    Router                      Output Targets
─────────────                    ──────                      ──────────────
USB HID ──────┐                                              ┌──→ PCEngine
USB X-input ──┤                                              ├──→ GameCube
Bluetooth ────┼──→ router_submit_input() ──→ router ──→ ────┼──→ Dreamcast
WiFi (JOCP) ──┤                              │               ├──→ Nuon, 3DO
Native SNES ──┤                              │               ├──→ USB Device
Native N64 ───┤                              │               └──→ UART
Native GC ────┤
LodgeNet ─────┘
                                    profile_apply()
                                    (button remapping)
```

### Key Abstractions

#### input_event_t (`core/input_event.h`)
```c
typedef struct {
    uint8_t dev_addr;           // Device address
    int8_t instance;            // Instance number
    input_device_type_t type;   // GAMEPAD, MOUSE, KEYBOARD
    uint32_t buttons;           // Button bitmap (JP_BUTTON_*)
    uint8_t analog[8];          // Analog axes (0-255, 128=center)
    int8_t delta_x, delta_y;    // Mouse deltas
} input_event_t;
```

#### OutputInterface (`core/output_interface.h`)
```c
typedef struct {
    const char* name;
    void (*init)(void);
    void (*core1_entry)(void);      // Runs on Core 1
    void (*task)(void);             // Periodic task on Core 0
    uint8_t (*get_rumble)(void);
    uint8_t (*get_player_led)(void);
} OutputInterface;
```

#### Router Modes
- **SIMPLE**: 1:1 mapping (device N → slot N)
- **MERGE**: All inputs merged to single output
- **BROADCAST**: All inputs to all outputs

#### Profile System
Apps can define button remapping profiles in `profiles.h`:
- SELECT + D-pad Up/Down cycles profiles (after 2s hold)
- Visual feedback via NeoPixel LED
- Haptic feedback via rumble
- Profile selection persisted to flash
- Apps without `profiles.h` pass buttons through unchanged

### Button Definitions (`core/buttons.h`)

W3C Gamepad API order - bit position = button index:
```c
#define JP_BUTTON_B1 (1 << 0)   // A         B         Cross
#define JP_BUTTON_B2 (1 << 1)   // B         A         Circle
#define JP_BUTTON_B3 (1 << 2)   // X         Y         Square
#define JP_BUTTON_B4 (1 << 3)   // Y         X         Triangle
#define JP_BUTTON_L1 (1 << 4)   // LB        L         L1
#define JP_BUTTON_R1 (1 << 5)   // RB        R         R1
#define JP_BUTTON_L2 (1 << 6)   // LT        ZL        L2
#define JP_BUTTON_R2 (1 << 7)   // RT        ZR        R2
#define JP_BUTTON_S1 (1 << 8)   // Back      -         Select
#define JP_BUTTON_S2 (1 << 9)   // Start     +         Start
#define JP_BUTTON_L3 (1 << 10)  // LS        LS        L3
#define JP_BUTTON_R3 (1 << 11)  // RS        RS        R3
#define JP_BUTTON_DU (1 << 12)  // D-Up
#define JP_BUTTON_DD (1 << 13)  // D-Down
#define JP_BUTTON_DL (1 << 14)  // D-Left
#define JP_BUTTON_DR (1 << 15)  // D-Right
#define JP_BUTTON_A1 (1 << 16)  // Guide     Home      PS
```

### Dual-Core Architecture

- **Core 0**: USB/BT polling, input processing, main loop
- **Core 1**: Console output protocol (timing-critical PIO)

### PIO State Machines

Console protocols use RP2040 PIO for precise timing:
- **PCEngine**: `plex.pio`, `clock.pio`, `select.pio`
- **GameCube**: `joybus.pio` (130MHz clock required)
- **Dreamcast**: `maple.pio` (maple bus protocol)
- **Nuon**: `polyface_read.pio`, `polyface_send.pio`
- **3DO**: `sampling.pio`, `output.pio`
- **Loopy**: `loopy.pio`
- **N64/GC Host**: `joybus.pio` (shared with GC device)
- **LodgeNet Host**: `lodgenet.pio` (MCU + SR programs, swapped at runtime)

## Development Workflow

### Adding a New App

1. Create `src/apps/<appname>/` with:
   - `app.c` - App initialization, router/player config
   - `app.h` - Version, config constants
   - `profiles.h` - Button mapping profiles (optional)

2. Add to `CMakeLists.txt` and `Makefile`

3. Build: `make <appname>`

### Adding a New USB Device Driver

1. Create `src/usb/usbh/hid/devices/vendors/<vendor>/<device>.c/h`

2. Implement:
   ```c
   bool <device>_is_device(uint16_t vid, uint16_t pid);
   void <device>_init(uint8_t dev_addr, uint8_t instance);
   void <device>_process(uint8_t dev_addr, uint8_t instance,
                         uint8_t const* report, uint16_t len);
   void <device>_disconnect(uint8_t dev_addr, uint8_t instance);
   ```

3. Register in `hid_registry.c`

### Adding a Bluetooth Device Driver

1. Create `src/bt/bthid/devices/vendors/<vendor>/<device>.c/h`

2. Similar interface to USB HID drivers

3. Register in BT device registry

### Adding a Native Controller Host (like gc_host, n64_host)

1. Create `src/native/host/<protocol>/<protocol>_host.c/h`

2. Implement `HostInterface` and `InputInterface`:
   ```c
   extern const HostInterface <protocol>_host_interface;
   extern const InputInterface <protocol>_input_interface;
   ```

3. Add `INPUT_SOURCE_NATIVE_<PROTOCOL>` to `router.h`

4. Key functions:
   - `<protocol>_host_init()` - Initialize PIO/GPIO
   - `<protocol>_host_task()` - Poll controller, submit to router
   - Use `router_submit_input()` with dev_addr 0xD0+ range

5. Remember to invert Y-axis if protocol uses non-HID convention

### LodgeNet Host (reference implementation)

The LodgeNet host (`src/native/host/lodgenet/`) is a good reference for PIO-based native input with:
- **Multi-protocol auto-detection**: MCU (N64/GC) and SR (SNES) protocols on a single PIO SM, with programs swapped at runtime via `pio_remove_program`/`pio_add_program`
- **Device type detection**: N64 vs GC distinguished by protocol flags (byte[1] bit 6)
- **Controller layout reporting**: Sets `event.layout` to `LAYOUT_NINTENDO_N64`, `LAYOUT_GAMECUBE`, or `LAYOUT_NINTENDO_4FACE` for SInput face style
- **VCC pin**: Drives a GPIO high to power the controller (RJ11 connector)
- **Poll throttling**: MCU at ~60Hz, SR at ~131Hz (matching reference hardware framework)
- **Encoded virtual buttons**: Impossible d-pad combos (SOCD) decoded as LodgeNet system buttons (Menu, Select, etc.)

## ESP32-S3 Development

The `bt2usb` app also runs on ESP32-S3, using BLE (no Classic BT) for controller input and USB OTG for HID output. See `.dev/docs/esp32-port.md` for full details.

```bash
# Prerequisites: ESP-IDF v6.0+ installed at ~/esp-idf
make bt2usb_xiao_esp32s3          # Build
make flash-bt2usb_xiao_esp32s3    # Flash via esptool
make monitor-bt2usb_xiao_esp32s3  # UART serial monitor
```

Key differences from RP2040:
- **BLE only** - ESP32-S3 has no Classic BT; only BLE controllers work (Xbox BLE, 8BitDo BLE, etc.)
- **FreeRTOS** - BTstack runs in its own task; main loop must not block (`tud_task_ext(1, false)` not `tud_task()`)
- **Platform HAL** - Shared code uses `platform/platform.h` instead of pico-sdk APIs directly
- **Build system** - ESP-IDF/CMake under `esp/`, separate from the RP2040 pico-sdk build under `src/`

See `docs/ESP32.md` for full setup, architecture, and board details.

## nRF52840 Development

The `bt2usb` app also runs on Seeed XIAO nRF52840 (xiao_ble), using BLE (no Classic BT) for controller input and USB for HID output. Uses nRF Connect SDK (Zephyr) with BTstack + TinyUSB (not Zephyr native stacks) to maximize shared code.

```bash
# Prerequisites: nRF Connect SDK v3.1.0+ (installed via make init-nrf)
make init-nrf                # One-time NCS workspace setup
make bt2usb_seeed_xiao_nrf52840         # Build
make flash-bt2usb_seeed_xiao_nrf52840   # Flash via UF2 bootloader
make monitor-bt2usb_seeed_xiao_nrf52840 # UART serial monitor
```

Key differences from RP2040:
- **BLE only** - nRF52840 supports Classic BT but this port is BLE-only like ESP32; only BLE controllers work
- **Zephyr RTOS** - BTstack runs in its own Zephyr thread (`k_thread_create`); main loop yields via `k_msleep(1)`
- **Raw HCI** - Zephyr provides raw HCI passthrough (`CONFIG_BT_HCI_RAW=y`), BTstack acts as BLE host
- **TinyUSB owns USB** - Zephyr USB stack disabled; TinyUSB's `dcd_nrf5x.c` drives hardware directly
- **RTT console** - Since TinyUSB owns USB, debug output uses Segger RTT (not CDC)
- **Build system** - nRF Connect SDK/west/CMake under `nrf/`, separate from `src/` and `esp/`

## Common Pitfalls

- **GameCube requires 130MHz** - `set_sys_clock_khz(130000, true)`
- **PIO has 32 instruction limit** - Optimize or split programs
- **Use `__not_in_flash_func`** - For timing-critical code
- **Y-axis convention** - All input drivers MUST normalize to HID standard: 0=up, 128=center, 255=down
  - Sony/Xbox/8BitDo controllers: Native HID, no inversion needed
  - Nintendo controllers: Invert Y (Nintendo uses 0=down, 255=up)
  - Native GC/N64: Invert Y when reading
- **ESP32 `tud_task()` blocks forever** - On FreeRTOS, `tud_task()` = `tud_task_ext(UINT32_MAX, false)`. Always use `tud_task_ext(1, false)` on ESP32
- **ESP32 BTstack threading** - All BTstack API calls must happen in the BTstack FreeRTOS task, not the main task
- **ESP32 Classic BT guards** - `gap_inquiry_*`, `gap_set_class_of_device()`, `gap_discoverable_control()` are Classic-only; guard with `#ifndef BTSTACK_USE_ESP32`
- **nRF Classic BT guards** - Same Classic-only APIs must be guarded with `#ifndef BTSTACK_USE_NRF`
- **nRF BTstack threading** - All BTstack API calls must happen in the BTstack Zephyr thread, not the main thread
- **nRF Zephyr USB disabled** - `CONFIG_USB_DEVICE_STACK=n` and `&usbd { status = "disabled"; }` required so TinyUSB can own the USB peripheral

## External Dependencies

Submodules in `src/lib/`:
- **pico-sdk** (2.2.0): Raspberry Pi Pico SDK
- **tinyusb** (0.19.0): USB host/device stack
- **tusb_xinput**: X-input controller support
- **joybus-pio**: GameCube/N64 joybus protocol
- **btstack**: Bluetooth stack (for BT support)

## CI/CD

GitHub Actions (`.github/workflows/build.yml`):
- Builds all apps on push to `main`
- Docker-based for consistency
- Artifacts in `releases/` directory

---

# MouthPad message-output → Nintendo Switch (nRF52840 DK bring-up)

> SFP-746 "BLE routing": drive a Nintendo Switch from the MouthPad companion app
> through a joypad-os board. **Status: BLE write → button injection VERIFIED on
> hardware.** Remaining: strip debug instrumentation, confirm the companion
> (btleplug) write end-to-end, verify the USB HID report.

## Goal / data flow
MouthPad gesture → companion `.aaMouthpadMessageOutput("A")` → BLE central writes
the ASCII token to the board's message-output service → firmware maps token →
`JP_BUTTON_*` → `router_submit_input` → `switch_mode` USB HID (Switch Pro).
```
companion (central) --write "A"--> 6E40FE01 --> ble_output service handler
  --> msgout_handle_token() --> JP_BUTTON_B2 --> router_submit_input
  --> switch_mode USB HID --> Switch
```

## Hardware / identifiers
- **DK board**: nRF52840 DK, J-Link serial **1050287076** (build/flash/RTT target).
- **MouthPad**: J-Link serial **1050222688** — DO NOT flash.
- **Board BLE address**: `DE:B7:53:18:22:29` (static-random; FICR DEVICEADDR0=
  0x53182229, DEVICEADDR1=0xD84A1EB7, byte5 0x1E|0xC0=0xDE). Hosts may show a
  cached name **"MouthPad^USB"** (this dev chip previously ran MouthPad fw — name
  stale, address correct).
- NCS **v3.3.0** at `C:\ncs`; west via `nrfutil toolchain-manager launch
  --ncs-version v3.3.0 -- west ...`.

## Build & flash (APP_TYPE=controller_btusb — the nRF BLE-peripheral app)
```bash
cd /c/ncs
# pristine (needed when EXTRA_CONF_FILE changes; use conf names RELATIVE to the
# app dir — west does not MSYS-translate the semicolon list):
nrfutil toolchain-manager launch --ncs-version v3.3.0 -- \
  west build -b nrf52840dk/nrf52840 /c/Users/suraj/code/joypad-os/nrf \
    -d /c/Users/suraj/code/joypad-os/nrf/build_dk_ctrl --pristine --no-sysbuild -- \
    -DDTC_OVERLAY_FILE=/c/Users/suraj/code/joypad-os/nrf/boards/nrf52840dk_nrf52840.overlay \
    -DEXTRA_CONF_FILE='prj_controller_btusb.conf;dk_rtt.conf'
# incremental (no conf change):  west build -d .../nrf/build_dk_ctrl
nrfutil toolchain-manager launch --ncs-version v3.3.0 -- \
  west flash -d /c/Users/suraj/code/joypad-os/nrf/build_dk_ctrl --dev-id 1050287076
```

## Building for the XIAO (shipping board) + env gotchas

The DK was the bring-up rig; **`xiao_ble` is the shipping board** and now builds
the same message-output → Switch firmware. Learnings from getting it green:

**Build environment (Windows).** There is NO west workspace in `joypad-os/nrf`
— the NCS workspace is `C:\ncs`, so build from there through the toolchain
launcher. Two failure modes if you don't: `west: unknown command "build"` (not
in a workspace) and a Python `_type_` AttributeError during CMake reconfigure.
**Never `ninja -C <builddir>` to reconfigure** — bare ninja/cmake picks up the
wrong Python. The command that works:

```bash
cd /c/ncs
nrfutil toolchain-manager launch --ncs-version v3.3.0 -- \
  west build -b xiao_ble -d /c/Users/suraj/code/joypad-os/nrf/build_xiao_ctrl \
    /c/Users/suraj/code/joypad-os/nrf --pristine=always -- \
    -DAPP_TYPE=controller_btusb \
    -DDTC_OVERLAY_FILE=/c/Users/suraj/code/joypad-os/nrf/boards/xiao_ble.overlay \
    -DEXTRA_CONF_FILE="/c/Users/suraj/code/joypad-os/nrf/boards/xiao_ble.conf;/c/Users/suraj/code/joypad-os/nrf/prj_controller_btusb.conf"
```
Incremental (source-only edit): `west build -d <builddir>`. A conf/Kconfig or
CMakeLists change reconfigures automatically — still via `west build`, not ninja.

**One marker, both boards.** `MSGOUT_SWITCH_BUILD` (renamed from the DK-only
`MSGOUT_DK_BUILD`) is set in CMakeLists for `controller_btusb` on both
`nrf52840dk` and `xiao_ble`. It gates forced Switch USB mode + trimmed adv, and
via `app.h` (`#if defined(MSGOUT_SWITCH_BUILD)` → `REQUIRE_BT_INPUT=0`) the
BLE-central-off. **app.h is authoritative** — the CMake `-DREQUIRE_BT_INPUT=0`
override loses the "last -D wins" race to controller_btusb's `=1`, so pinning it
in the header is the reliable way.

**XIAO porting fixes** (controller_btusb had never built for it):
- **MDK include + nrfx compat shim** — TinyUSB `dcd_nrf5x.c` needs `nrf.h` and
  `nrf_clock_hf_is_running()` (dropped in v3.3.0 nrfx). NOT DK-specific: a shared
  `if(BOARD MATCHES nrf52840dk OR xiao_ble)` block in CMakeLists adds both.
- **`platform_i2c_nrf.c`** — use `DEVICE_DT_GET_OR_NULL` for i2c0 (not
  `DEVICE_DT_GET`, which is a *compile* error where the bus is disabled, as on the
  XIAO). The JoyWing seesaw just no-ops (unused on this path).
- **`xiao_ble.conf: CONFIG_BOARD_SERIAL_BACKEND_CDC_ACM=n`** — the XIAO defaults
  to a USB-CDC console, which pulls in the device-next USB stack +
  `cdc_acm_serial.c` and binds the UDC the overlay disables for TinyUSB.
  (`CONFIG_USBD` / `CONFIG_USB_DEVICE_STACK` are the WRONG symbols — `USBD` is
  undefined here and aborts on the Kconfig warning.) Console → uart0 / RTT.
- **`xiao_ble.overlay`** — `uart1` msgout on **D6 = P1.11 (TX) / D7 = P1.12 (RX)**,
  115200, plus the `joypad,msgout-uart` chosen node. Adapter TX → D7, RX → D6, GND.

**Flashing.** DK = J-Link (`nrfjprog --program <builddir>/zephyr/zephyr.hex
--sectorerase --verify --reset --snr 1050287076`; on "Cannot connect to J-Link"
error -102, power-cycle the DK and retry). XIAO = **no debugger**: double-tap
reset → `XIAO-SENSE` USB drive → drag `build_xiao_ctrl/nrf/zephyr/zephyr.uf2`.
`build*/` is gitignored — stage source explicitly, never `git add -A`.

## Files changed
- `nrf/boards/nrf52840dk_nrf52840.{overlay,conf}` — DK port: enable `i2c1`
  (display init resolves), disable `usbd` (TinyUSB owns USB).
- `nrf/dk_rtt.conf` — RTT console overlay (note: printk-to-RTT is NOT actually
  working on this build — see gotchas).
- `nrf/CMakeLists.txt` — `nrf52840dk` board case: `python3`→`${PYTHON_EXECUTABLE}`
  for the GATT generator; MDK include (`nrfx/bsp/stable/mdk`) for TinyUSB `nrf.h`;
  `nrfx_compat_shim.c`; **`REQUIRE_BT_INPUT=0`** (so BTstack post-init is
  `ble_output_late_init`/advertise, not `bt_central_post_init`); `MSGOUT_DK_BUILD=1`.
- `nrf/src/nrfx_compat_shim.c` — `nrf_clock_hf_is_running` (removed in v3.3.0 nrfx).
- `src/bt/ble_output/ble_gamepad.gatt` — adds `6E40FE00` service + writable
  `6E40FE01` char (value handle **0x4b**, service range 0x49–0x4b).
- `src/bt/ble_output/ble_output.c`:
  - `scan_resp_standard` — advertises `6E40FE00` in the SCAN RESPONSE (the
    companion connects only to advertisers of this UUID; adv packet is full).
  - `adv_data_dk` (MSGOUT_DK_BUILD) — 24-byte adv data (stock 32B is >31B limit).
  - **KEY FIX: `msgout_service_write_callback` + `att_server_register_service_handler`**
    for the 6E40FE00 range. BTstack delivers a custom DYNAMIC char's write PAYLOAD
    only to a registered service handler — the generic `att_write_callback` only
    sees handle-0 / transaction-EXECUTE housekeeping, NOT the data. Same mechanism
    the NUS char uses (`nordic_spp_service_server_init`).
  - `msgout_handle_token` — parser: `A B X Y L R ZL ZR PLUS MINUS L3 R3 HOME
    CAPTURE DUP DDOWN DLEFT DRIGHT`; optional `+`/`-` = hold/release; bare = tap
    (press + 90 ms auto-release). `A`→B2,`B`→B1,`X`→B3,`Y`→B4 (Switch ABXY),
    `ZL`→L2, etc. Analog sticks via `LX=`/`LY=`/`RX=`/`RY=` (0..255, 128=center,
    held until changed); `CENTER` recenters. (Buttons + sticks; both transports.)
  - Temp debug instrumentation (`msgout_dbg_*`, `msgout_svc_*`,
    `msgout_held_buttons`, J-Link-readable) — **strip before merge**.
- `gesture_profile_lib/examples/switch-radial.mkprofile` (tauri-app) — radial
  profile mapping gestures → button tokens via `.aaMouthpadMessageOutput`.

## Wired-UART message-output (parallel input path)

A second transport for the **same tokens**, alongside the BLE `6E40FE01` char —
for bench/CI without BLE: a USB-to-UART adapter writes tokens straight to the board.

- **Seam:** both paths converge on `msgout_handle_token()` (now non-static; declared
  in `src/bt/ble_output/msgout.h`). The UART path adds no button logic — identical
  grammar and hold/release/tap semantics.
- **`src/bt/ble_output/msgout_uart.c`** — Zephyr interrupt-driven UART RX; frames
  whitespace-delimited tokens (`"A+ ZL+\n"`, `"A\nB\n"`, …) and calls
  `msgout_handle_token()` per token. Compiles to a no-op unless the board's
  devicetree selects a `joypad,msgout-uart` chosen node.
- **`ble_output_late_init()`** calls `msgout_uart_init()` after registering the BLE
  service handler — both paths live at once.
- **DK wiring** (`nrf52840dk_nrf52840.overlay`): `uart1` @115200 8N1, TX=P1.01,
  RX=P1.02. Adapter TX → board RX (P1.02), GND common. Drive it with e.g.
  `printf 'A+ ZL+\n' > /dev/ttyUSB0` (or any serial terminal).
- **Build:** `msgout_uart.c` added to the controller_btusb source list;
  `CONFIG_UART_INTERRUPT_DRIVEN=y` in `prj_controller_btusb.conf`.
- Not reusing `src/native/host/uart/uart_host.c` (Pico-SDK-only); this is a Zephyr
  binding. `uart_protocol.h`'s binary packet framing is for the generic
  `input_event` UART path, not these ASCII tokens.

## VERIFIED on hardware (via the Pi, clean BlueZ)
Pi `pi@10.0.0.97`: board advertises 6E40FE00, connects, GATT shows 6e40fe00 +
6e40fe01. A **raw ATT Write Command** (`tools/att_write_raw.py`, L2CAP CID 4) of
`A+` then `ZL+` → service handler got `handle=0x4b, mode=NONE, size=3, data` →
`msgout_held_buttons = 0x42` (B2|L2 = A|ZL). ✓

## Test tools (`nrf/tools/`)
- `att_write_raw.py <addr> <handle_hex> <token>` — raw ATT Write Command over
  L2CAP. THE reliable Linux test (e.g. `... DE:B7:53:18:22:29 0x004b "A+"`).
- `cw.py` / `msgout_ble_test.py` — bleak. NOTE: bleak's `write_gatt_char` on
  **Linux/BlueZ** does NOT put a normal ATT Write on the wire (btmon = 0 Write
  PDUs); use `att_write_raw.py`. On **Windows/WinRT** bleak's write should be a
  real Write Command.
- `scan_all.py`, `toggle_bt.py` (Windows radio reset), `README_msgout_switch.md`.

## J-Link debug recipe (printk-to-RTT is silent — read RAM directly)
```
JLINK="/c/Program Files/SEGGER/JLink_V818/JLink.exe"  -SelectEmuBySN 1050287076
# read global:  halt / mem32 0xADDR, N / go / exit   (do NOT 'r' — clears RAM)
# breakpoint:   r / setbp 0xADDR / g / sleep 20000 / halt / regs / exit
# arm-zephyr-eabi-addr2line -f -e build_dk_ctrl/zephyr/zephyr.elf 0xADDR
# re-nm after every build — global addresses move.
```

## Gotchas / dead-ends (don't repeat)
- **bleak-on-Linux write doesn't deliver** (AcquireWrite path). Use att_write_raw.py / btleplug.
- **Windows stale GATT cache** for DE:B7:53:18:22:29 (shows old MouthPad GATT, no
  6e40fe00). Clear via elevated `pnputil /remove-device` or reboot.
- **WinRT BLE stack wedges** after failed bleak sessions (scanner won't start).
  `toggle_bt.py` helps; reboot is the reliable reset.
- **A registered service handler is REQUIRED** for a custom DYNAMIC write char;
  the generic att_write_callback never gets its data. (The core lesson.)
- controller_btusb is RP2040-oriented; its Pico/sensor init is `#ifdef`-guarded
  out on the DK and it idles fine once `REQUIRE_BT_INPUT=0`.

## Remaining work
1. Strip debug instrumentation; tidy diff.
2. Confirm companion (btleplug) connect+write end-to-end (Windows after reboot, or
   macOS) — companion sends a normal Write Command → service handler receives it.
3. Verify USB HID report on the wire; default USB output mode to Switch.
4. Validate switch-radial.mkprofile; full HIL: MouthPad gesture → companion → Switch.
