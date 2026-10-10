#!/usr/bin/env python3
"""Update an LP715 over BLE through the stock OTA bootloader.

The bootloader is the Phyplus SDK's OTA_internal_flash, so this speaks the
SDK's OTA protocol (sdk/phy6222/components/profiles/ota/ota_protocol.c):

  service  5833ff01-9b8b-5191-6142-22a4536ef123
  command  ...ff02  write          01 n burst          start, n partitions
                                   02 i fa ra sz crc   partition info (LE u32s)
                                   04                  reboot
  response ...ff03  notify         [error, response]   81 start, 84 partition
                                                       info, 87 burst received,
                                                       85 partition done,
                                                       83 all done
  data     ...ff04  write w/o rsp  partition data, 20 bytes per packet

Build the app with OTA=1 (XIP code at 0x11020000). XIP partitions are written
where they run; SRAM partitions go to the app bank at flash 0x11000 + offset
and the bootloader copies them to RAM at every boot, as with the stock app.

usage: ble_ota.py app.hex [--address 00:D0:42:91:02:00]

If the watch runs examples/ble_ctrl, it is first asked to reboot into the
bootloader (LP715 Control command 04). The bootloader advertises under its
own address with Phyplus manufacturer data (0x0504) and no name, so it is
found by that. --address picks the app when several are around.
Needs `pip install bleak`.
"""
import argparse
import asyncio
import struct
import sys

from bleak import BleakClient, BleakScanner

OTA_SERVICE = "5833ff01-9b8b-5191-6142-22a4536ef123"
OTA_COMMAND = "5833ff02-9b8b-5191-6142-22a4536ef123"
OTA_RESPONSE = "5833ff03-9b8b-5191-6142-22a4536ef123"
OTA_DATA = "5833ff04-9b8b-5191-6142-22a4536ef123"
LP_CONTROL = "4c500002-3731-3500-b000-000000000000"
LP_NAME = "LP715"
PHYPLUS_ID = 0x0504   # the bootloader advertises only this manufacturer data,
                      # under its own address (not the app's)

PART_MAX = 0x4000   # the bootloader buffers one partition in 16 KB of RAM
PACKET = 20         # fits any MTU; the web page can't read the MTU either
BURST = 0xff        # "no burst acks": the bootloader counts bursts in MTU-sized
                    # packets, so acknowledge per partition only
XIP_BASE, XIP_END = 0x11000000, 0x11080000

_T = [0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
      0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400]


def crc16(data):
    """The SDK's crc16 (components/libraries/crc16), seed 0."""
    crc = 0
    for b in data:
        t = _T[crc & 0xf]; crc = (crc >> 4) & 0xfff; crc ^= t ^ _T[b & 0xf]
        t = _T[crc & 0xf]; crc = (crc >> 4) & 0xfff; crc ^= t ^ _T[b >> 4]
    return crc


def read_hex(path):
    """Contiguous (address, bytes) segments of an Intel hex file."""
    segs, base = [], 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(":"):
            continue
        n, addr, typ = int(line[1:3], 16), int(line[3:7], 16), int(line[7:9], 16)
        data = bytes.fromhex(line[9:9 + 2 * n])
        if typ == 4:
            base = int(line[9:13], 16) << 16
        elif typ == 0:
            a = base + addr
            if segs and segs[-1][0] + len(segs[-1][1]) == a:
                segs[-1][1].extend(data)
            else:
                segs.append([a, bytearray(data)])
    return segs


def partitions(segs):
    """(flash_addr, run_addr, data) per partition, laid out like the stock app."""
    parts, bank_off = [], 0
    for addr, data in segs:
        if len(data) % 4:
            data = data + bytes(4 - len(data) % 4)
        for i in range(0, len(data), PART_MAX):
            chunk, run = bytes(data[i:i + PART_MAX]), addr + i
            if XIP_BASE <= run < XIP_END:
                parts.append((run, run, chunk))
            else:
                parts.append((bank_off, run, chunk))
                bank_off += len(chunk) + 8   # same spacing as the stock table
    return parts


class Ota:
    def __init__(self, client):
        self.client = client
        self.rsp = asyncio.Queue()

    def _notify(self, _, data):
        self.rsp.put_nowait(bytes(data))

    async def expect(self, want, timeout=10):
        while True:
            r = await asyncio.wait_for(self.rsp.get(), timeout)
            err, cmd = r[0], r[1] if len(r) > 1 else 0xff
            if err:
                raise RuntimeError("bootloader error 0x%02x (response 0x%02x)" % (err, cmd))
            if cmd == want:
                return
            if cmd != 0x87:   # a burst ack is harmless; anything else is not
                raise RuntimeError("expected 0x%02x, got 0x%02x" % (want, cmd))

    async def command(self, data):
        await self.client.write_gatt_char(OTA_COMMAND, bytes(data), response=True)

    async def run(self, parts):
        await self.client.start_notify(OTA_RESPONSE, self._notify)
        pkt = PACKET
        await self.command([0x01, len(parts), BURST])
        await self.expect(0x81)
        for i, (fa, ra, data) in enumerate(parts):
            crc = crc16(data)
            print("partition %d/%d: flash 0x%08x run 0x%08x size 0x%05x crc 0x%04x"
                  % (i + 1, len(parts), fa, ra, len(data), crc))
            await self.command(struct.pack("<BBIIII", 0x02, i, fa, ra, len(data), crc))
            await self.expect(0x84)
            for off in range(0, len(data), pkt):
                await self.client.write_gatt_char(OTA_DATA, data[off:off + pkt], response=False)
            await self.expect(0x83 if i == len(parts) - 1 else 0x85, timeout=30)
        print("all partitions written; rebooting into the new app")
        try:
            await self.command([0x04])
        except Exception:
            pass   # it resets before answering


async def find(address, want_app, timeout=15):
    """The app (by name or address) or the bootloader (by manufacturer data)."""
    def match(d, adv):
        if address and d.address.upper() != address.upper():
            return False
        if want_app:
            return bool(address) or adv.local_name == LP_NAME
        return PHYPLUS_ID in adv.manufacturer_data
    return await BleakScanner.find_device_by_filter(match, timeout=timeout)


async def main():
    ap = argparse.ArgumentParser(description="BLE OTA update for the LP715")
    ap.add_argument("hexfile")
    ap.add_argument("--address", help="app address (default: the first LP715)")
    args = ap.parse_args()

    parts = partitions(read_hex(args.hexfile))
    if not any(XIP_BASE + 0x20000 <= ra < XIP_END for _, ra, _ in parts):
        sys.exit("no XIP code at 0x11020000: build the app with OTA=1")

    dev = await find(None, want_app=False, timeout=5)   # already in the bootloader?
    if dev is None:
        app = await find(args.address, want_app=True)
        if app is None:
            sys.exit("watch not found")
        print("app running: asking it to reboot into the bootloader")
        async with BleakClient(app) as client:
            await client.write_gatt_char(LP_CONTROL, b"\x04", response=True)
        await asyncio.sleep(2)
        dev = await find(None, want_app=False)
        if dev is None:
            sys.exit("bootloader not found after the reboot")
    print("bootloader at", dev.address)
    async with BleakClient(dev) as client:
        await Ota(client).run(parts)


if __name__ == "__main__":
    asyncio.run(main())
