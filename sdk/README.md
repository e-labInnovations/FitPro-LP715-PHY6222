# PHY6222 SDK Docker Environment

Docker-based build environment for custom FitPro LP715 firmware. The same image
works on Linux, macOS (Intel and Apple Silicon) and Windows.

The image contains:

- Debian bookworm
- `gcc-arm-none-eabi` 12.2 + newlib (packaged for amd64 and arm64, so it runs
  natively on Apple Silicon — no platform pin needed)
- The Phyplus PHY62x2 SDK, taken from
  [amir1387aht/phy6222_smartwatch](https://github.com/amir1387aht/phy6222_smartwatch)
  at a pinned commit, installed at `/opt/phy6222_sdk` (`$SDK`)
- `make`, `python3`

The SDK is downloaded when the image is built, not stored in this repo.

---

## Build the Image

```bash
docker build -t phy6222-sdk sdk/
```

To move to a newer SDK, change `SDK_COMMIT` in the `Dockerfile` (or pass
`--build-arg SDK_COMMIT=<sha>`).

---

## Build an Example

Run from the **repo root**:

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/panel_id phy6222-sdk make
```

Windows (PowerShell): `-v "${PWD}:/src"`; cmd.exe: `-v "%cd%:/src"`.

Mount the repo root, not the example directory — every example uses `lib/core`
and `sdk/phy6222.mk` through `../../`.

The output is `examples/<name>/_build/<name>.hex`.

---

## Flash

Flashing runs on the **host**, not in Docker: Docker Desktop on macOS and Windows
cannot pass USB serial devices into a container. The host needs only
`python3` and `pyserial` (`pip install pyserial`).

```bash
python3 tools/rdwr_phy62x2.py -p /dev/ttyUSB0 -b 500000 -r wh examples/panel_id/_build/panel_id.hex
```

`wh` writes the boot table at `0x2000`, the SRAM image from `0x5000` and the
XIP code from `0x10100`. It does not touch `0x0`–`0x1fff`.

To read the UART log afterwards, open the port with **DTR off** — DTR on holds
the chip's test-mode pin and keeps it in the boot ROM:

```bash
python3 tools/miniterm.py /dev/ttyUSB0 115200 --rts 1 --rtstoggle 100 --exit-char 3 --rtsexit 1
```

### Going back to the stock firmware

```bash
python3 tools/restore_full.py tools/rdwr_phy62x2.py binaries/stock/stock_flash_2.bin /dev/ttyUSB0
```

Do **not** use `rdwr_phy62x2.py we` for this: it erases the whole chip and then
fails (it skips the ROM's flash setup and uses a buffer address that collides
with the boot ROM's RAM). If `restore_full.py` stalls on a block, run it again
with that block's offset as the last argument to resume.

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
| `BLE=1`   | Also link the BLE host stack (GAP/GATT/L2CAP/SMP)         |
| `SYS_CLK` | System clock, default `SYS_CLK_DLL_48M`; `SYS_CLK_XTAL_16M` also works. SPI runs at half of it, so 24 MHz by default |

---

## What Was Fixed Compared with the Upstream Repo

The upstream project builds on Windows with an older GCC. On Linux and GCC ≥ 14
it fails; this setup avoids each problem:

| Problem                                                     | Fix                                                         |
| ----------------------------------------------------------- | ----------------------------------------------------------- |
| `#include "osal.h"` but the file is `OSAL.h`                | The image adds a lowercase copy                             |
| `syscalls.c` calls `LOG_INFO`, which no SDK header defines (an error in GCC 14) | `lib/core/syscalls.c` uses `dbg_printf` |
| Stale `build/*.d` files committed upstream                  | Only `sdk/` is taken from upstream                          |
| Plain `make` flashes to `COM19` and opens a terminal        | `make` here only builds                                     |
