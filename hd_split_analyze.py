#!/usr/bin/env python3
"""
Dry-run analysis for splitting src/st/nz0/hd.c and src/st/dre/hd.c into
per-topic files matching the existing us/pspeu topic-file structure.

For every function in config/symbols.hd.st<ovl>.txt, find the real HD vram
address (RHS of the assignment) and figure out which topic .c file the
byte-matching US function lives in, by scanning ONLY the real reference
overlays that hydrate_psx_duplicate_symbols() in tools/make-config.py
actually cross-references stage overlays against: stdre, stnp3, stnz0,
stst0, stwrp (see dup_paths in that function). Earlier version of this
script searched ALL build/us/*.map files and got false collisions from
unrelated overlays that happen to share the same literal vram address.

Prints a report only. Does not write anything.
"""
import re
import os
import sys
from collections import defaultdict

REFERENCE_OVERLAYS = ["stdre", "stnp3", "stnz0", "stst0", "stwrp"]


def parse_hd_symbols(path):
    """Return dict: label -> (hd_addr:int, comment:str)"""
    out = {}
    pat = re.compile(r'^(\S+)\s*=\s*(0x[0-9A-Fa-f]+);\s*(?://\s*(.*))?$')
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            m = pat.match(line)
            if not m:
                continue
            label, addr, comment = m.group(1), int(m.group(2), 16), (m.group(3) or "")
            out[label] = (addr, comment.strip())
    return out


def build_us_address_index():
    """Scan only the 5 real reference-overlay maps. Returns:
    addr:int -> list of (symbol_name, topic_basename, overlay) candidates.
    Keeping all candidates (not just first) makes collisions visible."""
    index = defaultdict(list)
    map_files = [f"build/us/{o}.map" for o in REFERENCE_OVERLAYS if os.path.isfile(f"build/us/{o}.map")]
    sec_re = re.compile(r'^\s\.(text|rodata|data|bss)\s+0x([0-9a-fA-F]+)\s+0x[0-9a-fA-F]+\s+(\S+)$')
    sym_re = re.compile(r'^\s{16}0x([0-9a-fA-F]+)\s+(\S+)$')
    for mf in map_files:
        cur_file = None
        ovl = os.path.basename(mf)[:-4]  # strip .map
        with open(mf, errors="ignore") as f:
            for line in f:
                line = line.rstrip('\n')
                m = sec_re.match(line)
                if m:
                    obj = m.group(3)
                    base = os.path.basename(obj)
                    cur_file = base[:-4] if base.endswith(".c.o") else None
                    continue
                m = sym_re.match(line)
                if m and cur_file:
                    addr = int(m.group(1), 16)
                    name = m.group(2)
                    cand = (name, cur_file, ovl)
                    if cand not in index[addr]:
                        index[addr].append(cand)
    return index


def analyze(ovl_short, symbols_path, us_index):
    print(f"\n{'='*70}\n{ovl_short}: {symbols_path}\n{'='*70}")
    hd_syms = parse_hd_symbols(symbols_path)
    print(f"Total entries: {len(hd_syms)}")

    groups = defaultdict(list)     # topic -> [(hd_addr, label, us_name, ovl)]
    unmatched = []
    ambiguous = []

    for label, (hd_addr, comment) in hd_syms.items():
        m = re.match(r'func_([0-9A-Fa-f]{8})$', label)
        target_addr = int(m.group(1), 16) if m else None

        candidates = us_index.get(target_addr, []) if target_addr is not None else []
        if not candidates:
            unmatched.append((hd_addr, label, comment))
            continue

        topics = set(c[1] for c in candidates)
        if len(topics) == 1:
            us_name, topic, ovl = candidates[0]
            groups[topic].append((hd_addr, label, us_name, ovl))
        else:
            ambiguous.append((hd_addr, label, candidates))

    print(f"Matched to a single topic: {sum(len(v) for v in groups.values())}")
    print(f"Ambiguous (different topics across ref overlays): {len(ambiguous)}")
    print(f"Unmatched (no candidate at all): {len(unmatched)}")

    print("\n--- Topic groups (sorted by HD address) ---")
    all_ranges = []
    for topic, items in sorted(groups.items(), key=lambda kv: min(x[0] for x in kv[1])):
        items.sort()
        lo, hi = items[0][0], items[-1][0]
        all_ranges.append((lo, hi, topic, len(items)))
        print(f"{topic:30s} n={len(items):3d}  range=0x{lo:08X}-0x{hi:08X}")

    print("\n--- Contiguity check (topics should not interleave in HD address space) ---")
    all_ranges.sort()
    prev_hi = None
    prev_topic = None
    problems = 0
    for lo, hi, topic, n in all_ranges:
        if prev_hi is not None and lo < prev_hi:
            print(f"  OVERLAP: {topic} (0x{lo:08X}) starts before {prev_topic} ends (0x{prev_hi:08X})")
            problems += 1
        prev_hi, prev_topic = hi, topic
    if problems == 0:
        print("  OK: ranges are in non-decreasing order, no overlaps detected.")
    else:
        print(f"  {problems} problem(s) found.")

    if ambiguous:
        print("\n--- Ambiguous entries (candidates disagree on topic) ---")
        for hd_addr, label, candidates in sorted(ambiguous):
            cand_str = ", ".join(f"{name}({topic}/{ovl})" for name, topic, ovl in candidates)
            print(f"  {label:20s} hd_addr=0x{hd_addr:08X}  candidates: {cand_str}")

    print("\n--- Unmatched entries (would stay in hd.c / need real decompilation) ---")
    for hd_addr, label, comment in sorted(unmatched):
        print(f"  {label:20s} hd_addr=0x{hd_addr:08X}  comment={comment}")

    return groups, unmatched, ambiguous, all_ranges


def write_plan(ovl_short, groups, unmatched, ambiguous, out_path):
    """Write a full per-function sorted plan: hd_addr, owner, label, us_name.
    owner is the topic name for matched functions, or 'hd' for unmatched/ambiguous
    (both of these need to stay as real INCLUDE_ASM stubs, not matched segments)."""
    rows = []
    for topic, items in groups.items():
        for hd_addr, label, us_name, ovl in items:
            rows.append((hd_addr, topic, label, us_name, ovl))
    for hd_addr, label, comment in unmatched:
        rows.append((hd_addr, "hd", label, "", ""))
    for hd_addr, label, candidates in ambiguous:
        rows.append((hd_addr, "hd", label, "AMBIGUOUS", ""))
    rows.sort()
    with open(out_path, "w") as f:
        for hd_addr, topic, label, us_name, ovl in rows:
            f.write(f"0x{hd_addr:08X}\t{topic}\t{label}\t{us_name}\t{ovl}\n")
    print(f"\nWrote {len(rows)} rows to {out_path}")


if __name__ == "__main__":
    if not os.path.isdir("build/us") or not os.path.isdir("config"):
        print("Run this from the sotn-decomp repo root.", file=sys.stderr)
        sys.exit(1)

    us_index = build_us_address_index()
    groups, unmatched, ambiguous, _ = analyze("nz0", "config/symbols.hd.stnz0.txt", us_index)
    write_plan("nz0", groups, unmatched, ambiguous, "hd_split_plan.nz0.txt")

    groups, unmatched, ambiguous, _ = analyze("dre", "config/symbols.hd.stdre.txt", us_index)
    write_plan("dre", groups, unmatched, ambiguous, "hd_split_plan.dre.txt")
