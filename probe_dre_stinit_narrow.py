#!/usr/bin/env python3
# Structural parallel found: in the pspeu (native PSP) split of dre, st_init_psp.c
# sits immediately after prim_helpers and before e_breakable. In hd's dre.yaml,
# prim_helpers is immediately followed by an unclaimed "hd" catch-all blob
# [0x21B10, 0x220E8) (2096 bytes) before e_background_house. Hypothesis: that
# catch-all blob contains st_init_psp.c's Load()/InitEntityIds() code.
# Narrow-anchored masked search in exactly that window.
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

nz = [i for i, b in enumerate(data) if b != 0]

def score_at(off):
    m = sum(1 for i in nz if 0 <= off + i < len(rom) and rom[off + i] == data[i])
    return m / len(nz)

LO, HI = 0x21B10, 0x220E8
print(f".text size = {len(data):#x} ({len(data)} bytes), n_nonzero={len(nz)}")
print(f"searching window [{hex(LO)}, {hex(HI)}) ({HI-LO} bytes)")

best = (None, 0)
hits = []
for off in range(LO, min(HI - len(data), len(rom) - len(data)) + 1):
    s = score_at(off)
    if s > best[1]:
        best = (off, s)
    if s > 0.85:
        hits.append((off, s))

print(f"best: offset={hex(best[0])} (vram {hex(0x80180000+best[0])}) score={best[1]:.2%}")
for off, s in hits[:10]:
    print(f"  hit: offset={hex(off)} (vram {hex(0x80180000+off)}) score={s:.2%}")
