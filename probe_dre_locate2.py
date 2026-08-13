#!/usr/bin/env python3
import subprocess, os

DRE_BIN = "disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/dre.bin"

def base_for(src):
    b = src.replace("/", "_")
    if b.endswith(".c"):
        b = b[:-2]
    return "_" + b

candidates = [
    "src/st/dre/header.c",
    "src/st/dre/gen/e_laydef.c",
    "src/st/dre/background_block_init.c",
    "src/st/dre/gen/e_layout.c",
    "src/st/dre/gen/rooms.c",
]

with open(DRE_BIN, "rb") as f:
    rom = f.read()

def masked_search(data, search_range_start=0, search_range_end=None):
    """Slide `data` over rom within [search_range_start, search_range_end), scoring by
    fraction of non-zero bytes in `data` that match rom at that offset. Zero bytes in
    `data` are ignored (could be unrelocated pointers)."""
    if search_range_end is None:
        search_range_end = len(rom) - len(data)
    nz_positions = [i for i, b in enumerate(data) if b != 0]
    if not nz_positions:
        return []
    results = []
    for off in range(search_range_start, min(search_range_end, len(rom) - len(data))):
        matches = sum(1 for i in nz_positions if rom[off + i] == data[i])
        score = matches / len(nz_positions)
        if score > 0.9:
            results.append((off, score, len(nz_positions)))
    return results

for src in candidates:
    base = base_for(src)
    obj = f"/tmp/probe{base}.o"
    if not os.path.exists(obj):
        print(f"{src}: NO OBJECT, skip")
        continue
    binout = f"/tmp/probe{base}_data.bin"
    subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".data", obj, binout], check=True)
    with open(binout, "rb") as f:
        data = f.read()
    print(f"=== {src} === size={len(data):#x}")
    # search the whole file this time
    hits = masked_search(data, 0, len(rom))
    if not hits:
        print("  no >90% match found anywhere in dre.bin")
    for off, score, n in hits:
        print(f"  offset={hex(off)} score={score:.2%} (n_nonzero={n})")
