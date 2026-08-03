#!/usr/bin/env python3
"""Lightweight SH2 disassembler for sotn-decomp Saturn .PRG files.

Uses capstone (CS_ARCH_SH) since the project's own tools/saturn-splitter
(Rust) and sh-elf-objdump are not buildable in every environment. This is a
reading/analysis aid, not a replacement for the real splitter: it does not
resolve symbols, detect function boundaries precisely, or produce buildable
asm. Output listing is grouped by the subsegments already declared in the
matching config/saturn/<name>.prg.yaml (data ranges are skipped by default).

Usage:
    python3 tools/sh2_disasm.py alucard
    python3 tools/sh2_disasm.py alucard --start 0x60 --end 0x200
    python3 tools/sh2_disasm.py game --all   # include data subsegments too

Requires: pip install capstone pyyaml
"""
import argparse
import pathlib
import sys

try:
    import capstone
except ImportError:
    sys.exit("capstone is required: pip install capstone --break-system-packages")

try:
    import yaml
except ImportError:
    sys.exit("pyyaml is required: pip install pyyaml --break-system-packages")

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
CONFIG_DIR = REPO_ROOT / "config" / "saturn"


def load_config(name):
    cfg_path = CONFIG_DIR / f"{name}.prg.yaml"
    if not cfg_path.exists():
        sys.exit(f"no config at {cfg_path}")
    with open(cfg_path) as f:
        return yaml.safe_load(f)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("name", help="segment name, e.g. alucard, game, richter, maria, warp, t_bat, stage_02")
    ap.add_argument("--start", type=lambda x: int(x, 0), default=None, help="file offset to start at (default: whole file)")
    ap.add_argument("--end", type=lambda x: int(x, 0), default=None, help="file offset to stop at")
    ap.add_argument("--all", action="store_true", help="also disassemble subsegments marked type: data")
    args = ap.parse_args()

    cfg = load_config(args.name)
    target = REPO_ROOT / cfg["options"]["target_path"]
    if not target.exists():
        sys.exit(f"missing {target} -- extract the Saturn disc into disks/saturn/ first")

    data = target.read_bytes()
    md = capstone.Cs(capstone.CS_ARCH_SH, capstone.CS_MODE_SH2 | capstone.CS_MODE_BIG_ENDIAN)

    for seg in cfg["segments"]:
        vram = seg["vram"]
        for sub in seg.get("subsegments", []):
            sstart, send = sub["start"], sub["end"]
            if args.start is not None and send < args.start:
                continue
            if args.end is not None and sstart > args.end:
                continue
            if sub.get("type") == "data" and not args.all:
                print(f"; -- skipped data subsegment 0x{sstart:X}-0x{send:X} (use --all to include) --")
                continue
            lo = max(sstart, args.start) if args.start is not None else sstart
            hi = min(send, args.end) if args.end is not None else send
            print(f"; -- subsegment {sub.get('file', '?')} type={sub.get('type')} 0x{lo:X}-0x{hi:X} --")
            for insn in md.disasm(data[lo:hi + 1], vram + lo):
                print(f"0x{insn.address:08X}:\t{insn.mnemonic}\t{insn.op_str}")


if __name__ == "__main__":
    main()
