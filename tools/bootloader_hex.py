#!/usr/bin/env python3
"""Extract the stock OTA bootloader from a full flash dump as an Intel hex
file that `rdwr_phy62x2.py wh` can write.

The boot table at 0x2000 lists the bootloader's segments (flash, size, run
address); `wh` rebuilds that table and writes each segment. The app table at
0x3000 is left out, so the bootloader starts in OTA mode until an app is
sent over BLE (tools/ble_ota.py).

The MAC at 0x4000 is not included: `wh` places the SRAM segments at 0x5000
only while they end below the lowest flash segment, and a segment at 0x4000
would push them up into the app's SRAM bank at 0x11000, where the first OTA
update would overwrite them. `wh` never erases 0x4000, so the MAC stays.

usage: bootloader_hex.py stock_flash.bin out.hex
"""
import struct
import sys


def hex_records(addr, data):
    out, upper = [], None
    for off in range(0, len(data), 16):
        a = addr + off
        if a >> 16 != upper:
            upper = a >> 16
            rec = bytes([2, 0, 0, 4, upper >> 8, upper & 0xff])
            out.append(':' + rec.hex().upper() + '%02X' % (-sum(rec) & 0xff))
        chunk = data[off:off + 16]
        rec = bytes([len(chunk), (a >> 8) & 0xff, a & 0xff, 0]) + chunk
        out.append(':' + rec.hex().upper() + '%02X' % (-sum(rec) & 0xff))
    return out


def main():
    img = open(sys.argv[1], 'rb').read()
    count = struct.unpack_from('<I', img, 0x2000)[0]
    lines = []
    for i in range(count):
        fa, size, run, _ = struct.unpack_from('<IIII', img, 0x2100 + 16 * i)
        off = fa & 0x7ffff
        print('segment run %08x  flash %06x  size %05x' % (run, off, size))
        lines += hex_records(run, img[off:off + size])
    lines.append(':00000001FF')
    open(sys.argv[2], 'w').write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
