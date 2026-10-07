// Second pass for LP715_RE: make flash read-only so the decompiler folds
// literal-pool constants, then seed functions at every Thumb "push {..,lr}"
// that analysis missed inside the code ranges, and re-run auto-analysis.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;

public class FixupPHY6222 extends GhidraScript {
    long[][] CODE = {
        {0x11020000L, 0x1103d000L},
        {0x1fff1838L, 0x1fff8954L},
    };

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        for (MemoryBlock b : mem.getBlocks())
            printf("%-7s %s-%s r%s w%s x%s\n", b.getName(), b.getStart(), b.getEnd(), b.isRead(), b.isWrite(), b.isExecute());
        MemoryBlock flash = mem.getBlock(toAddr(0x11000000L));
        flash.setWrite(false);
        Listing lst = currentProgram.getListing();
        int made = 0;
        for (int pass = 0; pass < 3; pass++) {
            for (long[] r : CODE) {
                for (long x = r[0]; x < r[1]; x += 2) {
                    Address ad = toAddr(x);
                    if ((mem.getByte(ad.add(1)) & 0xff) != 0xb5) continue;
                    if (getFunctionContaining(ad) != null) continue;
                    if (lst.getDefinedDataContaining(ad) != null) continue;
                    if (lst.getInstructionContaining(ad) != null && lst.getInstructionAt(ad) == null) continue;
                    if (!disassemble(ad)) continue;
                    if (createFunction(ad, null) != null) made++;
                }
            }
            analyzeAll(currentProgram);
        }
        printf("seeded %d functions, total %d\n", made, currentProgram.getFunctionManager().getFunctionCount());
    }
}
