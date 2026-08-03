#!/usr/bin/env bash
# Sync the Windows-side repo (edited by Claude/Cowork) into the WSL clone
# before running `make build`. Run this FROM INSIDE WSL.
#
# Usage:
#   bash tools/sync-to-wsl.sh
# (or add an alias in ~/.bashrc, see bottom of this file)

set -euo pipefail

# Source: the Windows folder, as seen from WSL (adjust drive letter if needed)
SRC="/mnt/e/Cowork artifacts/Projects/decomp/sotn-decomp-repo/"
# Destination: your WSL clone
DST="$HOME/sotn-decomp/"

if [ ! -d "$SRC" ]; then
    echo "ERROR: source not found at: $SRC"
    echo "Check that the E: drive is mounted at /mnt/e in WSL (try: ls /mnt/e)."
    exit 1
fi

if [ ! -d "$DST" ]; then
    echo "ERROR: destination not found at: $DST"
    exit 1
fi

rsync -av --delete \
    --exclude ".git" \
    --exclude ".venv" \
    --exclude "build" \
    --exclude "**/build" \
    --exclude "*.bin" \
    --exclude "*.cue" \
    --exclude "*.o" \
    --exclude "__pycache__" \
    --exclude "bin/" \
    --exclude "disks/" \
    --exclude "**/target" \
    "$SRC" "$DST"

echo ""
echo "Sync done: $SRC -> $DST"
echo "You can now run: cd ~/sotn-decomp && make build -j"

# --- Optional: add this alias to ~/.bashrc for convenience ---
# alias sotn-sync='bash "/mnt/e/Cowork artifacts/Projects/decomp/sotn-decomp-repo/tools/sync-to-wsl.sh"'
