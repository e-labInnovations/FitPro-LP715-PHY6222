#!/usr/bin/env python3
# Write a full raw flash image (e.g. binaries/stock/stock_flash_2.bin) back to a PHY62x2.
# rdwr_phy62x2.py's `we` omits the boot ROM's flash setup (spifs/sfmod), so the ROM
# accepts each block but never answers. It also passes 0x1FFF0000+offset as the
# cpbin buffer address, which for blocks near 0x0 lands on the boot ROM's own SRAM
# variables and hangs it. This does the setup, uses a fixed buffer, compares the ROM's
# per-block byte-sum checksum with ours, and reads back words from every block.
# usage: restore.py <rdwr_phy62x2.py> <image.bin> [port] [resume_offset]
# resume_offset: continue an interrupted restore from that block (skips the erase).
import sys,struct,time,importlib.util
spec=importlib.util.spec_from_file_location('r',sys.argv[1]); r=importlib.util.module_from_spec(spec); spec.loader.exec_module(r)
img=open(sys.argv[2],'rb').read()
port=sys.argv[3] if len(sys.argv)>3 else '/dev/ttyUSB0'
start=int(sys.argv[4],0) if len(sys.argv)>4 else 0
assert len(img)==0x80000, 'expected a 512 KB image'
BLK=0x2000
BUF=0x1FFF8000   # cpbin receive buffer, clear of the boot ROM's data
# At 500000 baud the ROM intermittently drops a block; 115200 with short
# pauses is reliable and still sends 512 KB in about a minute.
BAUD=115200
f=r.phyflasher(port); f.Connect(BAUD)
assert f.ExpFlashSize() and f.HexStartSend(), 'flash setup failed'
f.SetAutoErase(False)
if start == 0:
    assert f.EraseSectorsFlash(0, len(img)), 'erase failed'
p=f._port; p.timeout=5
assert f.write_cmd('cpnum %d ' % ((len(img)-start)//BLK)), 'cpnum failed'
for n,off in enumerate(range(start,len(img),BLK)):
    blk=img[off:off+BLK]
    p.write(('cpbin c%d %X %X %X' % (n, off|r.MAX_FLASH_SIZE, BLK, BUF)).encode())
    assert p.read(12)==b'by hex mode:', 'no prompt at %#x' % off
    time.sleep(0.02)
    p.write(blk)
    rep=p.read(23)
    assert rep[:15]==b'checksum is: 0x', 'no checksum at %#x: %r' % (off,rep)
    rom=int(rep[15:],16)
    assert rom==sum(blk), 'checksum mismatch at %#x: rom %#x local %#x' % (off,rom,sum(blk))
    p.write(rep[15:])
    assert p.read(6)==b'#OK>>:', 'no ack at %#x' % off
    time.sleep(0.02)
    print('block %#07x ok (sum %#x)' % (off,rom))
bad=0
for off in range(0,len(img),BLK):
    for o in (off, off+0x800, off+0x1000, off+BLK-4):
        if f.read_reg(0x11000000+o)!=struct.unpack_from('<I',img,o)[0]: bad+=1; print('readback DIFF at %#x' % o)
print('readback: %d words checked, %d differ' % (4*len(img)//BLK, bad))
print('RESTORE OK' if bad==0 else 'RESTORE FAILED')
