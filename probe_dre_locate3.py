#!/usr/bin/env python3
import subprocess, os

DRE_BIN = "disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/dre.bin"

def base_for(src):
    b = src.replace("/", "_")
    if b.endswith(".c"):
        b = b[:-2]
    return "_" + b

with open(DRE_BIN, "rb") as f:
    rom = f.read()

def masked_search(data, lo, hi):
    nz = [i for i, b in enumerate(data) if b != 0]
    if not nz:
        return [], (None, 0, len(nz))
    out = []
    best = (None, 0, len(nz))
    for off in range(lo, min(hi, len(rom) - len(data))):
        m = sum(1 for i in nz if rom[off + i] == data[i])
        score = m / len(nz)
        if score > best[1]:
            best = (off, score, len(nz))
        if score > 0.85:
            out.append((off, score, len(nz)))
    return out, best

jobs = [
    ("src/st/dre/st_init_psp.c", 0x0, 0x23290, ".text"),
]

for src, lo, hi, sect in jobs:
    base = base_for(src)
    obj = f"/tmp/probe{base}.o"
    if not os.path.exists(obj):
        print(f"{src}: NO OBJECT (compile step must run first), skipping")
        continue
    binout = f"/tmp/probe{base}_text.bin"
    subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", sect, obj, binout], check=True)
    with open(binout, "rb") as f:
        data = f.read()
    print(f"=== {src} === size={len(data):#x}, searching [{hex(lo)},{hex(hi)})")
    hits, best = masked_search(data, lo, hi)
    if not hits:
        print("  no >85% match found in range")
        print(f"  best partial match: offset={hex(best[0]) if best[0] is not None else 'n/a'} score={best[1]:.2%} n_nonzero={best[2]}")
    for off, score, n in hits[:10]:
        print(f"  offset={hex(off)} (vram {hex(0x80180000+off)}) score={score:.2%} n_nonzero={n}")
