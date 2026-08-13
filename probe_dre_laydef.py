#!/usr/bin/env python3
import subprocess, struct

DRE_BIN = "disks/pspeu/PSP_GAME/USRDIR/res/ps/hdbin/dre.bin"
OBJ = "/tmp/probe_src_st_dre_gen_e_layout.o"

# get offsets of entity_layout_a1ba7b73_x/_y within the .data section of e_layout.c.o
out = subprocess.run(["mipsel-linux-gnu-objdump", "-t", OBJ], capture_output=True, text=True).stdout
x_off = y_off = None
for line in out.splitlines():
    if "entity_layout_a1ba7b73_x" in line and "*UND*" not in line:
        x_off = int(line.split()[0], 16)
    if "entity_layout_a1ba7b73_y" in line and "*UND*" not in line:
        y_off = int(line.split()[0], 16)
print("x_off", hex(x_off), "y_off", hex(y_off))

E_LAYOUT_START = 0x1620  # confirmed file offset of gen/e_layout.c's .data
VRAM_BASE = 0x80180000
x_vram = VRAM_BASE + E_LAYOUT_START + x_off
y_vram = VRAM_BASE + E_LAYOUT_START + y_off
print("x_vram", hex(x_vram), "y_vram", hex(y_vram))

LAYOUT_ENTITY_SIZE = 10  # 5 x u16

def build_array(base_vram, indices):
    buf = b""
    for idx in indices:
        addr = base_vram + idx * LAYOUT_ENTITY_SIZE
        buf += struct.pack("<I", addr)
    return buf

h_indices = [0, 2] + [0] * 49
v_indices = [0, 2] + [0] * 49

h_bytes = build_array(x_vram, h_indices)
v_bytes = build_array(y_vram, v_indices)

with open(DRE_BIN, "rb") as f:
    rom = f.read()

for name, needle in [("entityLayoutHorizontal", h_bytes), ("entityLayoutVertical", v_bytes)]:
    idx = rom.find(needle)
    count = rom.count(needle)
    print(f"{name}: len={len(needle)} first_offset={hex(idx) if idx>=0 else 'NOT FOUND'} matches={count}")
