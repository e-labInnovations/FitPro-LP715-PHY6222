# Flashing

The PHY6222's boot ROM has a UART loader, so a USB-serial adapter is all the
hardware needed.

## Wiring

The loader talks on the chip's UART, **P09 (TX)** and **P10 (RX)**, at 3.3 V.
The tools here expect the adapter's **DTR** on the chip's TM (test-mode) pin
(DTR on = stay in the boot ROM loader, DTR off = run the firmware) and
**RTS** on reset.

## Flash an example

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

## Over the air (OTA)

Apps built with `OTA=1` (e.g. `examples/ble_ctrl`) run from the **stock OTA
bootloader**, which is the Phyplus SDK's `OTA_internal_flash` and speaks the
SDK's OTA protocol (`sdk/phy6222/components/profiles/ota`). Once it is on the
watch, updates need no cable.

### Flash layout

| Flash | What |
| ----- | ---- |
| `0x2000` | Boot table: the boot ROM loads the bootloader |
| `0x3000` | App table, written by the bootloader after an update |
| `0x4000` | The watch's MAC |
| `0x5000`–`0x9fff`, `0xa000`–`0x10a77` | Bootloader (SRAM image, XIP code) |
| `0x11000`– | App bank: the app's SRAM image, copied to RAM at every boot |
| `0x20000`– | App XIP code (`OTA=1` links it here) |

### Putting the bootloader on the watch (once, over UART)

```bash
python3 tools/bootloader_hex.py binaries/stock/stock_flash_2.bin bootloader.hex
python3 tools/rdwr_phy62x2.py -p /dev/ttyUSB0 -b 500000 wh bootloader.hex
python3 tools/rdwr_phy62x2.py -p /dev/ttyUSB0 -b 500000 -r er 0x3000 0x1000
```

The second command restores the bootloader from the stock backup; the third
erases any app table, so the bootloader starts in updater mode. Then send an
app as below.

### Sending an app

From the BLE web remote (`tools/ble_ctrl.html`, Chrome or Edge): choose the
`.hex`, press **Update firmware**, and pick the updater in the device list. Or
from a computer with Python and `bleak`:

```bash
docker run --rm -v "$(pwd)":/src -w /src/examples/ble_ctrl ghcr.io/e-labinnovations/phy6222-sdk make   # sets OTA=1
python3 tools/ble_ota.py examples/ble_ctrl/_build/ble_ctrl.hex
```

Both ask a running `ble_ctrl` to restart into the updater (LP715 Control
command `04`, see `lib/core/ota.h`), then send the image: about 45 s from
Python, 20–30 s from the web page.

Good to know:

- The updater is a separate BLE device: no name, its own address
  (`D0:00:00:02:91:43` on this watch), Phyplus manufacturer data `0x0504`.
  Both tools find it by that.
- If an update fails, the watch stays in the updater: just send again. A
  reset after a successful update starts the app.
- Only apps that handle command `04` can be updated without the cable. To
  reach the updater from any other app, reset the watch with the app table
  erased (`er 0x3000 0x1000`).
- Writing any example with `wh` replaces the bootloader's boot table; put the
  bootloader back as above to use OTA again.

## Going back to the stock firmware

```bash
python3 tools/restore_full.py tools/rdwr_phy62x2.py binaries/stock/stock_flash_2.bin /dev/ttyUSB0
```

Do **not** use `rdwr_phy62x2.py we` for this: it erases the whole chip and then
fails (it skips the ROM's flash setup and uses a buffer address that collides
with the boot ROM's RAM). If `restore_full.py` stalls on a block, run it again
with that block's offset as the last argument to resume. On 2026-10-10 it
twice got no checksum for the very first block right after the chip erase;
the bootloader alone can always be written with `wh` (see above).

A full read of the 512 KB flash takes about 35 minutes over this loader (one
32-bit word per command), so reuse `binaries/stock/stock_flash_2.bin` rather
than dumping again.
