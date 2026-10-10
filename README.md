# FitPro LP715/716 (M6) — Custom Firmware

Reverse engineering and custom firmware for the **FitPro LP715/716** smartwatch,
built on the **Phyplus PHY6222** SoC.

<!-- Photo of the watch: add docs/images/lp715.jpg, then uncomment.
![FitPro LP715](docs/images/lp715.jpg)
-->

## Hardware

| Part    | Details                                                     |
| ------- | ----------------------------------------------------------- |
| SoC     | Phyplus PHY6222QC (QFN32): Cortex-M0, 512 KB flash, BLE 5   |
| Display | 80×160 TFT, JD9850-type controller                          |
| Input   | Touch button                                                |
| Sensors | Ball/tilt switch for motion (no accelerometer)              |
| Other   | Vibrator, heart-rate LED, battery and charger sensing       |

Full pin map and board notes: [docs/hardware.md](docs/hardware.md).

## Status

- ✅ Display, backlight, graphics and fonts
- ✅ Touch button (tap, long, very long press)
- ✅ Vibrator and heart-rate LED
- ✅ Battery voltage, percentage, charger detection and fast charging
- ✅ Motion (shake) detection
- ✅ BLE: battery level, remote buzz / LED / backlight, button and shake events
  ([examples/ble_ctrl](examples/ble_ctrl/), with a [web remote](tools/ble_ctrl.html))
- ✅ Firmware updates over BLE through the stock OTA bootloader, from the web
  remote or `tools/ble_ota.py` ([docs/flashing.md](docs/flashing.md#over-the-air-ota))
- 🟡 Sleep between events (16 MHz builds, e.g. `ble_ctrl`); screen-off and wake-on-button still to do
- ⏳ A real watch app

## Quick Start

You need Docker and a 3.3 V USB-serial adapter (see
[docs/flashing.md](docs/flashing.md)).

Build an example (from the repo root):

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/status ghcr.io/e-labinnovations/phy6222-sdk make
```

Flash it:

```bash
pip install pyserial
python3 tools/rdwr_phy62x2.py -p /dev/ttyUSB0 -b 500000 -r wh examples/status/_build/status.hex
```

Writing your own example: [sdk/README.md](sdk/README.md).

### BLE web remote

Flash `examples/ble_ctrl`, then serve the remote and open it in Chrome or Edge
(Web Bluetooth needs `localhost` or HTTPS; Safari and Firefox lack it):

```bash
python3 -m http.server 8000 --directory tools
# open http://localhost:8000/ble_ctrl.html
```

## Repository

| Path              | What                                                      |
| ----------------- | --------------------------------------------------------- |
| `examples/`       | Small firmware projects, one per feature                  |
| `lib/`            | Board bring-up (`core`), drivers and BLE                  |
| `sdk/`            | Build image, shared make rules, PHY62x2 SDK               |
| `tools/`          | UART and BLE (OTA) flashing, stock restore, UART log, image converter, BLE web remote |
| `ghidra_project/` | Ghidra database and scripts for the stock firmware        |
| `binaries/`       | Stock firmware backup                                     |

## Documentation

| Doc                                          | About                                       |
| -------------------------------------------- | ------------------------------------------- |
| [Hardware](docs/hardware.md)                 | SoC, pin map, peripherals, flash layout     |
| [Flashing](docs/flashing.md)                 | Wiring, flashing, UART log, stock restore   |
| [SDK and build](sdk/README.md)               | Build image, make options, writing an example |
| [Stock firmware RE](ghidra_project/README.md) | The Ghidra project and its scripts         |

## References

| Resource                       | Link                                               |
| ------------------------------ | -------------------------------------------------- |
| amir1387aht/phy6222_smartwatch | https://github.com/amir1387aht/phy6222_smartwatch  |
| pvvx/PHY62x2                   | https://github.com/pvvx/PHY62x2                    |
| PHY6222 datasheet              | [docs/](docs/PHY6222_BLE_SoC_Datasheet_v1.5_20231101.pdf) |
