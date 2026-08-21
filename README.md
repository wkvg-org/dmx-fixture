# dmx-fixture

DMX512 relay actuator firmware for the Waveshare
**ESP32-S3(-POE)-ETH-8DI-8RO** board, running Zephyr RTOS. It receives DMX512
slots from an RS485 bus and drives 8 relays through a PCA9538 I²C GPIO
expander.

This is part of the Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
venue/home-automation system. In the installed daisy chain the fixture is a
downstream receiver:

```
club-ctrl (DMX TX master) → Moving Head #1 → Moving Head #2 → dmx-fixture
```

## What it is

A pure DMX512 receiver: it has **no network stack, no Wi-Fi and no MQTT**. It
listens on the RS485 bus, decodes incoming frames and switches relays based on
slot values. Each DMX slot maps to one relay; a value `≥ 128` switches the
relay ON, `< 128` switches it OFF.

## Hardware

| Function | Detail |
|----------|--------|
| MCU | ESP32-S3 (procpu core) |
| Board | Waveshare ESP32-S3(-POE)-ETH-8DI-8RO |
| DMX in | UART1 RX (GPIO18) from the RS485 bus |
| DMX out (unused) | UART1 TX (GPIO17) held idle-high so the auto-DE transceiver stays in receive mode |
| Relays | 8 × relay outputs via PCA9538 I²C GPIO expander (`tca` alias) |

The devicetree overlay
[`boards/esp32s3_eth_8di_8ro_procpu.overlay`](boards/esp32s3_eth_8di_8ro_procpu.overlay)
defines the `dmx-uart` alias (→ `uart1`), the RX/TX pin muxing and the
250000 baud rate.

## DMX mapping

The fixture occupies 8 consecutive slots starting at
`CONFIG_DMX_FIXTURE_START_ADDR` (default **19**, set in `prj.conf`):

| DMX slot | Relay | Behaviour |
|----------|-------|-----------|
| start + 0 | relay 1 | `≥ 128` → ON, `< 128` → OFF |
| start + 1 | relay 2 | … |
| … | … | … |
| start + 7 | relay 8 | … |

Relay state is only written (and logged) on slot **transitions**, so a single
DMX command produces exactly one `relay N -> ON/OFF` log line.

### Changing the start address

Set `CONFIG_DMX_FIXTURE_START_ADDR` in `prj.conf` (or via `west build -- -DCONFIG_DMX_FIXTURE_START_ADDR=…`). It must satisfy
`1 ≤ start ≤ 512 - 8`.

## Project structure

```
dmx-fixture/
├── boards/esp32s3_eth_8di_8ro_procpu.overlay   # UART1 + PCA9538 devicetree
├── src/
│   ├── main.c                                   # entry point / boot order
│   └── modules/relay/                           # relay fixture module
│       ├── relay_fixture.c
│       └── relay_fixture.h
├── CMakeLists.txt                               # pulls in dmx-z + relay module
├── Kconfig                                      # sources dmx-z Kconfig
└── prj.conf                                     # app + driver configuration
```

## Startup & system dependencies

Boot order in `src/main.c`:

| # | Step | Depends on |
|---|------|-----------|
| 1 | USB-serial console (`usb_serial`) | — (aborts if absent) |
| 2 | `relay_fixture_init()` — configures PCA9538, 8 relay outputs | I²C bus + PCA9538 (`tca` alias) |
| 3 | `dmx_rx_init()` — UART1 receiver, 250000 baud 8N2 | RS485 bus (upstream club-ctrl TX) |
| 4 | Relay thread (prio 5) polls `dmx_rx_frame_ready()` and applies `dmx_fixture_process()` | DMX frames arriving on the bus |

The relay fixture is initialised *before* the DMX RX polling thread so the RX
thread's UART register access cannot interfere with the I²C transactions done
during PCA9538 setup.

**Key external dependencies:**

- **DMX512 bus**: the fixture only reacts when a valid DMX frame reaches its
  UART1 RX from the upstream transmitter.
- **`dmx-z`**: the shared DMX512 driver (receiver half). See *Dependencies*.

## Shell

Standard Zephyr shell (no custom commands). Connect the USB-serial console at
115200 baud and run `help` to list built-ins.

## Dependencies

This application builds against the shared [`dmx-z`](https://github.com/wkvg-org/dmx-z)
driver, which must be checked out as a **sibling** directory (the
`CMakeLists.txt` references `../dmx-z`):

```
<workspace>/
├── dmx-z/          # git clone https://github.com/wkvg-org/dmx-z.git
└── dmx-fixture/    # this repository
```

## Build

Requires a Zephyr workspace at `~/zephyrproject`.

```bash
source ~/zephyrproject/.venv/bin/activate
west blobs fetch hal_espressif

west build --board esp32s3_eth_8di_8ro/esp32s3/procpu \
           --build-dir /tmp/b-fixture dmx-fixture
```

## Flash

```bash
west flash --build-dir /tmp/b-fixture --esp-device /dev/ttyACM0
```

Replace `/dev/ttyACM0` with the board's serial device (use the stable
`/dev/serial/by-id/…` path if you have multiple boards connected).

## License

SPDX-License-Identifier: Apache-2.0

## Copyright

Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
gemeinnützige UG (haftungsbeschränkt)
