#!/usr/bin/env python3
import subprocess, sys, os

DRE_BIN = "disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/dre.bin"

def base_for(src):
    b = src.replace("/", "_")
    if b.endswith(".c"):
        b = b[:-2]
    return "_" + b

candidates = [
    ("src/st/dre/header.c", base_for("src/st/dre/header.c")),
    ("src/st/dre/gen/e_laydef.c", base_for("src/st/dre/gen/e_laydef.c")),
    ("src/st/dre/background_block_init.c", base_for("src/st/dre/background_block_init.c")),
    ("src/st/dre/gen/e_layout.c", base_for("src/st/dre/gen/e_layout.c")),
    ("src/st/dre/gen/rooms.c", base_for("src/st/dre/gen/rooms.c")),
]

with open(DRE_BIN, "rb") as f:
    rom = f.read()

for src, base in candidates:
    obj = f"/tmp/probe{base}.o"
    if not os.path.exists(obj):
        print(f"{src}: NO OBJECT FILE, skipping")
        continue
    binout = f"/tmp/probe{base}_data.bin"
    subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".data", obj, binout],
                    check=True)
    with open(binout, "rb") as f:
        data = f.read()
    lead_zeros = len(data) - len(data.lstrip(b"\x00"))
    trimmed = data.lstrip(b"\x00")
    print(f"=== {src} === size={len(data):#x} leading_zeros={lead_zeros:#x}")
    if len(trimmed) < 4:
        print("  (all zero or too short, skip search)")
        continue
    # search for the largest contiguous non-zero run to use as a fingerprint,
    # since relocated pointer fields will be zero in the unlinked object.
    best = b""
    cur = b""
    for b in trimmed:
        if b != 0:
            cur += bytes([b])
            if len(cur) > len(best):
                best = cur
        else:
            cur = b""
    if len(best) < 4:
        print(f"  no usable non-zero run (longest={len(best)}), trying full trimmed data anyway")
        best = trimmed
    idx = rom.find(best)
    count = rom.count(best)
    print(f"  fingerprint_len={len(best)} first_match_offset={hex(idx) if idx>=0 else 'NOT FOUND'} total_matches={count}")
    if idx >= 0:
        # figure out offset of the fingerprint within the original data, to back-compute segment start
        fp_off_in_data = data.find(best)
        seg_start = idx - fp_off_in_data
        print(f"  => implied segment start in dre.bin = {hex(seg_start)}")
