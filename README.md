# dmx-fixture

DMX512 relay actuator firmware for the Waveshare ESP32-S3(-POE)-ETH-8DI-8RO
board. Receives DMX512 slots from the RS485 bus and drives 8 relays via a
PCA9538 GPIO expander.

## Hardware

- **MCU**: ESP32-S3 (procpu)
- **DMX in**: UART1 RX (GPIO18) from the RS485 bus; TX (GPIO17) held idle-high
  so the auto-DE transceiver stays in receive mode
- **Relays**: 8 outputs via PCA9538 I²C GPIO expander

## DMX mapping

The fixture occupies 8 consecutive slots starting at
`CONFIG_DMX_FIXTURE_START_ADDR` (default **19**):

| DMX slot | Relay | Threshold |
|----------|-------|-----------|
| 19 + 0 | relay 1 | `>= 128` → ON, `< 128` → OFF |
| 19 + 1 | relay 2 | … |
| … | … | … |
| 19 + 7 | relay 8 | … |

## Startup & system dependencies

Boot order in `src/main.c`:

| # | Step | Depends on |
|---|------|-----------|
| 1 | USB-serial console (`usb_serial`) | — (aborts if absent) |
| 2 | `relay_fixture_init()` — configures PCA9538, 8 relay outputs | I²C bus + PCA9538 (`tca` alias) |
| 3 | `dmx_rx_init()` — UART1 receiver, 250000 baud 8N2 | RS485 bus (upstream: club-ctrl TX) |
| 4 | relay thread (prio 5) polls `dmx_rx_frame_ready()` | DMX frames arriving on the bus |

**No network.** The dmx-fixture has no Ethernet/Wi-Fi and no MQTT — it is a
pure DMX512 receiver.

**Key external dependencies**:
- **DMX512 bus**: the fixture is a downstream receiver in the club-ctrl daisy
  chain (`club-ctrl TX → MH #1 → MH #2 → dmx-fixture`). It only reacts when a
  valid DMX frame reaches its UART1 RX.
- **Relay state is only logged/logically changed on slot *transitions***, so a
  single DMX command produces exactly one `relay N -> ON/OFF` log line.

## Shell commands

Standard Zephyr shell (no custom commands). Connect the serial console and use
`help` to list built-ins.

## Build

Requires a Zephyr workspace at `~/zephyrproject`.

```bash
source ~/zephyrproject/.venv/bin/activate
west blobs fetch hal_espressif

west build --board esp32s3_eth_8di_8ro/esp32s3/procpu \
           --build-dir /tmp/b-fixture dmx-fixture

west flash --build-dir /tmp/b-fixture --esp-device /dev/ttyACM0
```

## License

SPDX-License-Identifier: Apache-2.0

## Copyright

Copyright (C) 2025 - 2026 Wolfenbütteler Kultur- und Veranstaltungsgesellschaft
gemeinnützige UG (haftungsbeschränkt)
