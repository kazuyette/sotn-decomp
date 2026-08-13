#!/usr/bin/env python3
# Check if st_init_psp.c's compiled .text sits at the very START of dre.bin
# (VRAM 0x80180000, file offset 0x0) -- i.e. it's the overlay's entry point,
# never called by name from other C files, just jumped-to after CD load.
import subprocess, os

DRE_BIN = "disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/dre.bin"
OBJ = "/tmp/probe_src_st_dre_st_init_psp.o"

if not os.path.exists(OBJ):
    print(f"missing {OBJ} -- run probe_dre_compile6.sh first")
    raise SystemExit(1)

BINOUT = "/tmp/probe_stinit_text.bin"
subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".text", OBJ, BINOUT], check=True)

with open(BINOUT, "rb") as f:
    data = f.read()
with open(DRE_BIN, "rb") as f:
    rom = f.read()

def score_at(off):
    nz = [i for i, b in enumerate(data) if b != 0]
    if not nz:
        return 0.0, 0
    m = sum(1 for i in nz if 0 <= off + i < len(rom) and rom[off + i] == data[i])
    return m / len(nz), len(nz)

print(f".text size = {len(data):#x} ({len(data)} bytes)")
for off in [0x0]:
    s, n = score_at(off)
    print(f"offset={hex(off)} (vram {hex(0x80180000+off)}) score={s:.2%} n_nonzero={n}")

# also scan a small window near 0x0 in case of small misalignment
print("\nscanning [0x0, 0x400) for best match:")
best = (None, 0)
for off in range(0x0, 0x400):
    s, n = score_at(off)
    if s > best[1]:
        best = (off, s)
print(f"best: offset={hex(best[0])} score={best[1]:.2%}")
