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

## Going back to the stock firmware

```bash
python3 tools/restore_full.py tools/rdwr_phy62x2.py binaries/stock/stock_flash_2.bin /dev/ttyUSB0
```

Do **not** use `rdwr_phy62x2.py we` for this: it erases the whole chip and then
fails (it skips the ROM's flash setup and uses a buffer address that collides
with the boot ROM's RAM). If `restore_full.py` stalls on a block, run it again
with that block's offset as the last argument to resume.

A full read of the 512 KB flash takes about 35 minutes over this loader (one
32-bit word per command), so reuse `binaries/stock/stock_flash_2.bin` rather
than dumping again.
