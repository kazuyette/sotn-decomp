#!/usr/bin/env python3
"""
Find where a US .data/.rodata byte range physically lives inside the HD
binary, by extracting the exact bytes from the US ELF at a known vram range
and searching for that same byte sequence inside the HD target binary.

This assumes (as with the .text duplicate matches already confirmed this
session) that this specific data content is byte-identical between US and
HD -- a reasonable bet for game-balance tables, confirmed by direct search
rather than assumed.

Usage:
    python3 hd_find_data_offset.py <us_elf> <vram_start_hex> <size_hex> <hd_bin_path>

Example:
    python3 hd_find_data_offset.py build/us/stnz0.elf 0x80180a94 0x2be \
        disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/nz0.bin
"""
import subprocess
import sys
import re


def extract_us_bytes(elf_path, vram_start, size):
    vram_end = vram_start + size
    out = subprocess.run(
        [
            "objdump", "-s",
            f"--start-address=0x{vram_start:x}",
            f"--stop-address=0x{vram_end:x}",
            elf_path,
        ],
        capture_output=True, text=True, check=True,
    ).stdout

    data = bytearray()
    # objdump -s hex dump lines look like:
    #  80180a94 xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  ascii...
    hexline = re.compile(r'^\s*[0-9a-fA-F]{8}\s+((?:[0-9a-fA-F]{2,8}\s*){1,4})')
    for line in out.splitlines():
        m = hexline.match(line)
        if not m:
            continue
        chunk = m.group(1).split()
        for h in chunk:
            data += bytes.fromhex(h)
    return bytes(data[:size])


def find_in_hd(hd_bin_path, needle):
    with open(hd_bin_path, "rb") as f:
        haystack = f.read()
    offsets = []
    start = 0
    while True:
        idx = haystack.find(needle, start)
        if idx == -1:
            break
        offsets.append(idx)
        start = idx + 1
    return offsets, len(haystack)


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print(__doc__)
        sys.exit(1)
    elf_path, vram_hex, size_hex, hd_bin_path = sys.argv[1:5]
    vram = int(vram_hex, 16)
    size = int(size_hex, 16)

    needle = extract_us_bytes(elf_path, vram, size)
    print(f"Extracted {len(needle)} bytes from {elf_path} @ 0x{vram:X}")

    offsets, hd_len = find_in_hd(hd_bin_path, needle)
    print(f"HD binary {hd_bin_path}: {hd_len} bytes total")
    if not offsets:
        print("NOT FOUND -- byte content differs between US and HD for this range.")
    elif len(offsets) == 1:
        off = offsets[0]
        print(f"FOUND UNIQUE MATCH at file offset 0x{off:X}")
        print(f"(if this segment's 'code' vram base is 0x80180000 and start=0, the")
        print(f" subsegment offset to use in the yaml is 0x{off:X})")
    else:
        print(f"FOUND {len(offsets)} MATCHES (ambiguous): {[hex(o) for o in offsets]}")
