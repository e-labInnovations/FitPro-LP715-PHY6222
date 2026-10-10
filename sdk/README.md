# PHY6222 Build Environment

Everything needed to build firmware for the FitPro LP715 (Phyplus PHY6222):

| Path          | What                                                          |
| ------------- | ------------------------------------------------------------- |
| `phy6222/`    | Phyplus PHY62x2 SDK, from amir1387aht/phy6222_smartwatch       |
| `phy6222.mk`  | Shared build rules every example includes                     |
| `Dockerfile`  | Build image: ARM GCC and `make`, nothing else                 |

The SDK lives in the repo, not in the image, so builds don't depend on the
upstream repo staying up, and an SDK patch is an ordinary commit that takes
effect without rebuilding the image.

---

## The Image

```
ghcr.io/e-labinnovations/phy6222-sdk
```

- Debian bookworm-slim
- `gcc-arm-none-eabi` 12.2 + newlib, trimmed to the Cortex-M0 libraries
  (`thumb/v6-m/nofp`). The libraries for every other ARM core, C++ and LTO
  are removed, which takes the image from 1.4 GB to about 245 MB unpacked
  (56 MB to download).
- `make`

It is built for `linux/amd64` and `linux/arm64`, so it runs natively on Apple
Silicon, Linux and Windows hosts.

### Building it locally

```bash
docker build -t ghcr.io/e-labinnovations/phy6222-sdk sdk/
```

### Publishing

[.github/workflows/sdk-image.yml](../.github/workflows/sdk-image.yml) builds
the image when `sdk/Dockerfile` changes on `main` and pushes it to GHCR, tagged
`latest` and with the commit's short SHA. It then builds every example in the
new image. It can also be run by hand from the Actions tab.

---

## Build an Example

Run from the **repo root**:

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/panel_id ghcr.io/e-labinnovations/phy6222-sdk make
```

Windows (PowerShell): `-v "${PWD}:/src"`; cmd.exe: `-v "%cd%:/src"`.

Mount the repo root, not the example directory — every example uses `lib/core`,
`sdk/phy6222.mk` and the SDK through `../../`.

The output is `examples/<name>/_build/<name>.hex`.

---

## Flash

Flashing runs on the host, not in Docker: see
[docs/flashing.md](../docs/flashing.md).

---

## Writing an Example

```
examples/blink/
├── Makefile
└── main.c
```

`Makefile`:

```makefile
TARGET = blink
SRC    = main.c
include ../../sdk/phy6222.mk
```

`main.c` provides `app_init()` (runs once) and `app_update()` (called in a
loop). `lib/core` does the system bring-up (clocks, power manager, BLE
controller, UART log on P9/P10 at 115200) before calling them.

The number in brackets at the start of each log line is `hal_systick()`,
which counts 625 µs BLE slots, not milliseconds:

```c
#include "app.h"

void app_init(void)
{
    LOG("hello");
}

void app_update(void)
{
    WaitMs(1000);
}
```

`app.h` pulls in `board.h`, the LP715 pin map read out of the stock firmware.

Options for `phy6222.mk`:

| Variable  | Meaning                                                   |
| --------- | --------------------------------------------------------- |
| `TARGET`  | Output name                                               |
| `SRC`     | The example's own sources                                 |
| `LIB_SRC` | Extra sources from `lib/`, e.g. `display/jd9850.c`        |
| `SDK_EXTRA` | Extra SDK driver sources, e.g. `components/driver/adc/adc.c` |
| `BLE=1`   | Run the BLE stack under OSAL (see `lib/ble/ble.h`); `app_update()` is then called every 10 ms from a timer and must not block |
| `SYS_CLK` | System clock, default `SYS_CLK_DLL_48M`; `SYS_CLK_XTAL_16M` also works. SPI runs at half of it, so 24 MHz by default |
| `DEBUG_INFO` | Default 1: `LOG()` prints. 3 also enables the SDK's `AT_LOG`/`LOG_DEBUG`, which print from the radio interrupt and break BLE connections |
| `SLEEP_MODE` | Default `PWR_MODE_NO_SLEEP`. `PWR_MODE_SLEEP` sleeps between OSAL events and works only with `SYS_CLK=SYS_CLK_XTAL_16M` (with the DLL clocks every wake-up resets the chip). Drivers that use a peripheral must restore it after sleep, as `lib/display` does for SPI; see `examples/ble_ctrl` |

---

## SDK Contents

`phy6222/` is the `sdk/` directory of
[amir1387aht/phy6222_smartwatch](https://github.com/amir1387aht/phy6222_smartwatch)
at commit `02cc0c961d926506ee577454d7bc1286bdb3d803`, unchanged apart from what
the build never uses:

- `example/` (Phyplus sample projects, 12 MB)
- `components/ethermind/` (BLE mesh, 6 MB) and `components/coremark/`
- the Keil `.lib` archives (this build compiles the BLE host from source)

The SDK is © Phyplus Microelectronics; see `phy6222/SDK_LICENSE`.

### Compared with the upstream repo

The upstream project builds on Windows with an older GCC. On Linux and GCC ≥ 14
it fails; this setup avoids each problem:

| Problem                                                     | Fix                                                         |
| ----------------------------------------------------------- | ----------------------------------------------------------- |
| `syscalls.c` calls `LOG_INFO`, which no SDK header defines (an error in GCC 14) | `lib/core/syscalls.c` uses `dbg_printf` |
| Stale `build/*.d` files committed upstream                  | Only `sdk/` is taken from upstream                          |
| Plain `make` flashes to `COM19` and opens a terminal        | `make` here only builds                                     |
