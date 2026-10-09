# Ghidra Project — LP715_RE

Open `LP715_RE.gpr` in Ghidra (12.x). It holds two programs:

| Program             | Use it? | SRAM contents                                                     |
| ------------------- | ------- | ----------------------------------------------------------------- |
| `stock_flash_2.bin` | **Yes** | The **app** (table at flash `0x3000`)                             |
| `stock_flash.bin`   | No      | The OTA **bootloader** (table at `0x2000`) — wrong for app work   |

`stock_flash.bin` was the first import and mapped the boot ROM's table, which
loads the bootloader, not the app. It is kept only so older notes that quote its
addresses can be checked.

## Memory map (`stock_flash_2.bin`)

| Block    | Address                   | Contents                                          |
| -------- | ------------------------- | ------------------------------------------------- |
| `rom`    | `0x00000000` (256 KB)     | Mask ROM, uninitialised; 1001 names from the SDK  |
| `flash`  | `0x11000000` (512 KB)     | The whole dump; app XIP code at `0x11020000`–`0x1103c450` |
| `sram`   | `0x1fff0000` (64 KB)      | App SRAM image from flash `0x11000`: `0x1fff0000` (0x40c), `0x1fff1838` (0x4000), `0x1fff5838` (0x311c) |
| `sram2`  | `0x20000000`              | Zero-filled                                       |
| `periph` | `0x40000000` (1 MB)       | Peripherals, volatile                             |

The decompiler leaves literal-pool loads as `DAT_110xxxxx` instead of their
values; read the u32 at that address in the dump to resolve them.

## Rebuilding

The scripts here recreate the program from the dump, headless:

```bash
analyzeHeadless <abs>/ghidra_project LP715_RE \
  -import <abs>/binaries/stock/stock_flash_2.bin \
  -processor ARM:LE:32:Cortex -loader BinaryLoader -loader-baseAddr 0x11000000 \
  -scriptPath <abs>/ghidra_project \
  -preScript SetupPHY6222.java <abs>/bb_rom_sym_m0.txt

analyzeHeadless <abs>/ghidra_project LP715_RE -process stock_flash_2.bin -noanalysis \
  -scriptPath <abs>/ghidra_project \
  -postScript FixupPHY6222.java -postScript DumpAll.java /tmp/lp715_out
```

- `SetupPHY6222.java` — memory blocks, copies the app SRAM segments, applies the
  ROM names from the SDK's `misc/bb_rom_sym_m0.txt` (in this repo at
  `sdk/phy6222/misc/`).
- `FixupPHY6222.java` — seeds functions at every `push {…, lr}` the auto-analysis
  missed, then re-analyses.
- `DumpAll.java <dir>` — writes `decomp.c` and `functions.txt`.

Paths must be absolute (Ghidra rejects ones starting with `.`), and Ghidra 12
needs JDK 21+.
