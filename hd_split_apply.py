#!/usr/bin/env python3
"""
Apply the HD per-topic split derived by hd_split_analyze.py.

Reads hd_split_plan.<ovl>.txt (hd_addr, owner, label, us_name, ref_ovl per
line, tab-separated, sorted by address) and the existing
config/splat.hd.st<ovl>.yaml, then replaces the single monolithic
`[<off>, c, hd]` text subsegment with one subsegment per contiguous
same-owner run:
  - matched runs (owner = topic name) -> dotted `.c` type (matched, splat
    won't re-extract, reuses the ALREADY EXISTING src/st/<ovl>/<topic>.c
    file that us/pspeu already compile from -- same src_path is shared
    across versions for non-psp platforms).
  - unmatched runs (owner = "hd") -> plain `c` type, name "hd" (still needs
    real decompilation, same as today).

Does NOT touch the .data/.rodata header lines or the sbss/closing lines.
Writes a .yaml.bak backup before overwriting.
"""
import re
import sys

VRAM_BASE = 0x80180000


def load_plan(path):
    rows = []
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            parts = line.split("\t")
            hd_addr = int(parts[0], 16)
            owner = parts[1]
            rows.append((hd_addr, owner))
    rows.sort()
    return rows


def coalesce(rows):
    """Collapse consecutive same-owner rows into (start_addr, owner) runs."""
    runs = []
    for addr, owner in rows:
        if runs and runs[-1][1] == owner:
            continue  # still inside the same run, only need the start addr
        runs.append([addr, owner])
    return runs


def build_subsegment_lines(runs):
    lines = []
    for addr, owner in runs:
        off = addr - VRAM_BASE
        if owner == "hd":
            lines.append(f"      - [0x{off:X}, c, hd]")
        else:
            lines.append(f"      - [0x{off:X}, .c, {owner}]")
    return lines


def apply(ovl_short, plan_path, yaml_path):
    rows = load_plan(plan_path)
    runs = coalesce(rows)
    new_lines = build_subsegment_lines(runs)

    with open(yaml_path) as f:
        content = f.read()

    # find the single monolithic text line, e.g. "      - [0x303F4, c, hd]"
    pat = re.compile(r'^      - \[0x[0-9A-Fa-f]+, c, hd\]$', re.MULTILINE)
    matches = pat.findall(content)
    if len(matches) != 1:
        print(f"[{ovl_short}] ERROR: expected exactly 1 monolithic 'c, hd' line, found {len(matches)}. Aborting, no changes written.")
        return False

    replacement = "\n".join(new_lines)
    new_content = pat.sub(replacement, content)

    backup_path = yaml_path + ".bak"
    with open(backup_path, "w") as f:
        f.write(content)
    with open(yaml_path, "w") as f:
        f.write(new_content)

    n_matched = sum(1 for _, owner in runs if owner != "hd")
    n_unmatched = sum(1 for _, owner in runs if owner == "hd")
    print(f"[{ovl_short}] Wrote {yaml_path} ({len(runs)} subsegments: {n_matched} matched, {n_unmatched} unmatched runs). Backup at {backup_path}")
    return True


if __name__ == "__main__":
    ok1 = apply("nz0", "hd_split_plan.nz0.txt", "config/splat.hd.stnz0.yaml")
    ok2 = apply("dre", "hd_split_plan.dre.txt", "config/splat.hd.stdre.yaml")
    sys.exit(0 if (ok1 and ok2) else 1)
