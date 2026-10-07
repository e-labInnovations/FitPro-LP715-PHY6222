// Pre-analysis setup for a raw PHY6222 flash dump imported at 0x11000000:
// copies the boot-table SRAM segments into place, adds ROM/peripheral blocks,
// and names ROM functions from the SDK's bb_rom_sym_m0.txt.
// Script args: <path to bb_rom_sym_m0.txt>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.SourceType;
import java.nio.file.*;
import java.util.*;

public class SetupPHY6222 extends GhidraScript {
    Address a(long x) { return toAddr(x); }

    MemoryBlock zeroBlock(String name, long base, long size, boolean x) throws Exception {
        MemoryBlock b = currentProgram.getMemory().createInitializedBlock(name, a(base), size, (byte) 0, monitor, false);
        b.setPermissions(true, true, x);
        return b;
    }

    MemoryBlock uninit(String name, long base, long size, boolean x, boolean vol) throws Exception {
        MemoryBlock b = currentProgram.getMemory().createUninitializedBlock(name, a(base), size, false);
        b.setPermissions(true, !x, x);
        b.setVolatile(vol);
        return b;
    }

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        printf("MD5 %s\n", currentProgram.getExecutableMD5());
        MemoryBlock flash = mem.getBlock(a(0x11000000L));
        flash.setName("flash");
        flash.setPermissions(true, false, true);

        zeroBlock("sram", 0x1fff0000L, 0x10000, true);
        zeroBlock("sram2", 0x20000000L, 0x12800, true);
        uninit("rom", 0x0, 0x40000, true, false);
        uninit("periph", 0x40000000L, 0x100000, false, true);
        uninit("scs", 0xE000E000L, 0x1000, false, true);

        // The ROM boot table at 0x2000 loads the OTA bootloader. The app the
        // watch actually runs is described by the bootloader's table at 0x3000:
        // {flash, run, size, checksum} entries; SRAM entries give flash offsets
        // relative to the app image at 0x11000.
        long entry = 0x1fff1838L;
        int n = mem.getInt(a(0x11003000L));
        for (int i = 0; i < n; i++) {
            Address d = a(0x11003010L + 16L * i);
            long faddr = mem.getInt(d) & 0xffffffffL;
            long load = mem.getInt(d.add(4)) & 0xffffffffL;
            long size = mem.getInt(d.add(8)) & 0xffffffffL;
            printf("app seg %d flash %08x run %08x size %05x\n", i, faddr, load, size);
            if (faddr >= 0x11000000L) continue;              // XIP, already mapped
            byte[] buf = new byte[(int) size];
            mem.getBytes(a(0x11011000L + faddr), buf);
            mem.setBytes(a(load), buf);
            createLabel(a(load), String.format("app_seg%d_%08x", i, load), true);
        }

        String symPath = getScriptArgs().length > 0 ? getScriptArgs()[0] : null;
        int named = 0;
        if (symPath != null) {
            for (String line : Files.readAllLines(Paths.get(symPath))) {
                String[] f = line.trim().split("\\s+");
                if (f.length < 3 || !f[0].startsWith("0x")) continue;
                long v = Long.decode(f[0]);
                Address ad = a(v & ~1L);
                try {
                    if (f[1].equals("T") && (v & 1) == 1) {
                        if (getFunctionAt(ad) == null)
                            currentProgram.getFunctionManager().createFunction(f[2], ad, new AddressSet(ad), SourceType.IMPORTED);
                    } else {
                        createLabel(a(v), f[2], true, SourceType.IMPORTED);
                    }
                    named++;
                } catch (Exception e) { printf("sym %s: %s\n", f[2], e.getMessage()); }
            }
        }
        printf("ROM symbols applied: %d\n", named);

        addEntryPoint(a(entry));
        disassemble(a(entry));
        createFunction(a(entry), "app_entry");
        printf("entry %08x\n", entry);
    }
}
