# PC Port attempt log

Local working notes for the first attempt at building the cmake-based PC port,
following the wiki runbook: https://github.com/Xeeynamo/sotn-decomp/wiki/PC-Port

This file is a local working log (same spirit as the RLIB overlay's
`overlay_import_log.txt`) — not meant to be committed/PR'd, just a record of
what was needed to get a build going on this machine.

## 2026-08-03 — first attempt, WSL2 Ubuntu

### Prerequisites installed (none were present initially)

```bash
sudo apt update && sudo apt install -y cmake
sudo apt install -y libx11-dev libxext-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libxss-dev libxfixes-dev
sudo apt install -y libxtst-dev
```

Notes:
- `cmake` wasn't installed at all. Went with `apt install cmake` (3.27.8-1build1)
  rather than `snap install cmake --classic` to avoid the classic-confinement
  security prompt for no real benefit here.
- The bundled SDL3 (vendored under `tools/psyz/external/SDL`) needs real X11
  dev headers to configure on Linux — first failure was a generic "could not
  find X11 or Wayland development libraries", second failure (after the first
  batch of X11 libs) was specifically missing XTEST (`libxtst-dev`).
- WSL2 has WSLg built in, so once configured, an X11/Wayland `sotn` binary
  should be able to actually display through it without extra setup — not yet
  confirmed since we haven't reached a successful full build yet.

### Build command

```bash
cd ~/sotn-decomp
cmake -B build/pc -DCMAKE_BUILD_TYPE=Debug -DUSE_PLAYER_MARIA=1
cmake --build build/pc
```

Configure step succeeded after the dependency fixes above (SDL backends
enabled: dummy/offscreen/x11 video, ogl_es2/vulkan render, sdl3_gpu rendering
backend chosen).

Build reached ~70% before failing:

```
/home/user/sotn-decomp/src/maria/pl_main.c:89:10: fatal error: gen/font_eu.h: No such file or directory
   89 | #include "gen/font_eu.h"
compilation terminated.
```

### Current blocker (unresolved)

`gen/font_eu.h` doesn't exist anywhere in the tree yet — it's expected to be a
generated header (likely font/glyph data baked from a game asset), needed by
`src/maria/pl_main.c` (the "Maria" PSP-derived player implementation used by
`-DUSE_PLAYER_MARIA=1`). Candidate theory: this is tied to the PSP EU asset
extraction (`disks/pspeu/...`) that was deliberately deferred during the RLIB
overlay-import work — the PSP EU ISO exists on this machine
(`E:\Cowork artifacts\Projects\decomp\PSPEU\...`) but has never been extracted
into `disks/pspeu/`. Not yet confirmed; still investigating exactly what
script/target is supposed to produce `gen/font_eu.h` and from what source
asset.

**Next diagnostic to run**:
```bash
grep -rn "font_eu" --include=*.py --include=*.go --include=*.cmake --include=Makefile .
grep -rln "font_eu" . 2>/dev/null | grep -v "^./build"
find . -iname "*font_eu*" 2>/dev/null | grep -v "^./build"
cat src/maria/pl_main.c | grep -n "font_eu\|#include" | head -20
```

**Next steps once found**: identify the generation script/asset dependency,
run it (extracting PSP EU assets first if that's the actual source), rebuild.

### Resolved: `gen/font_eu.h` came from the PSPEU asset pipeline

Confirmed via `config/assets.pspeu.yaml` (`font_eu` cmpgfx entries under the
`maria.bin` segment, `src_path: src/maria`) — exactly the deferred PSP EU
extraction. Fix:

```bash
ln -sf "/mnt/e/Cowork artifacts/Projects/decomp/PSPEU/Castlevania - The Dracula X Chronicles (Europe) (En,Fr,De,Es,It).iso" disks/sotn.pspeu.iso
make extract-disk VERSION=pspeu   # 7z x into disks/pspeu/ (already had 7z installed)
make extract_assets VERSION=pspeu  # raw extraction into assets/maria/ etc — silent/exit 0, no visible output
make build_assets VERSION=pspeu    # THIS is the step that generates src/maria/gen/*.h (font_eu.h, hud_*.h, etc.)
```

Two-step asset pipeline confirmed: `extract_assets` (raw dump from the PSP
disk into `assets/<overlay>/`) then `build_assets` (converts raw assets into
the `gen/*.h` C headers under `src_path`) — both must run, `extract_assets`
alone is not enough and gives no error/warning to indicate the second step is
still needed.

### Full WSL build succeeded after the fix

```bash
cmake --build build/pc
```
Reached 100%, `build/pc/sotn` produced (13MB).

Stages already wired into `src/pc/stages/` (found via
`find src/pc/stages -name "*.c"`): `st0`, `nz0`, `wrp`, `cen`, `sel` (plus
`overlay.c`/`loader.c`, infra not stages).

### Runtime attempt on WSL — window opens but renders nothing

```bash
./build/pc/sotn --disk disks/sotn.us.cue --stage nz0
```
(Note: the wiki's example path has no `us/` subfolder — `disks/sotn.us.cue`,
not `disks/us/sotn.us.cue` — using the wrong path just disables music, not
fatal.)

Game logs progress far (loads title screen assets, ARC player spritesheet,
NZ0 stage files, VAB sound banks, weapon files) with no crash, but the SDL
window shows only a blank gradient with no rendered frame. Root cause: WSLg
on this machine only exposes a **software** Vulkan device
(`vulkaninfo --summary` → `deviceType = PHYSICAL_DEVICE_TYPE_CPU`, driver
`llvmpipe`) — no GPU passthrough ICD registered
(`/usr/share/vulkan/icd.d/` has `lvp_icd.json` and others but no working
`dzn`/d3d12 or functional `virtio_icd.json` — tried forcing
`VK_ICD_FILENAMES=.../virtio_icd.json`, got
"Failed to detect any valid GPUs in the current config"). `libd3d12.so` exists
under `/usr/lib/wsl/lib/` but isn't wired into a Vulkan ICD on this WSL
install. Not fully root-caused why llvmpipe itself doesn't present a frame —
concluded this is a WSLg/graphics-stack limitation, not a decomp code bug,
and moved to trying a native Windows build instead of debugging WSL's Vulkan
stack further.

## Native Windows build (same day) — much further, real crash found

Toolchain already available on this machine: **Visual Studio 2022 Build
Tools** (native MSVC, no separate install needed) — confirmed via Start Menu
("x64 Native Tools Command Prompt for VS 2022" etc. present). Used
**"Developer PowerShell for VS 2022"** rather than plain cmd.exe, since
cmd.exe cannot `cd` into UNC paths (`\\wsl.localhost\...`) — PowerShell
handles them fine as `Microsoft.PowerShell.Core\FileSystem::` drives.

### First attempt: building directly against the WSL UNC path — fails

```powershell
cd \\wsl.localhost\Ubuntu-24.04\home\user\sotn-decomp
cmake -B build\pc-win -DCMAKE_BUILD_TYPE=Debug -DUSE_PLAYER_MARIA=1
cmake --build build\pc-win --config Debug
```
Configure succeeds (real GPU backends detected this time: `d3d12`/`vulkan`
render drivers, unlike WSL's llvmpipe-only). Build fails with cascading
MSB8064/8065 warnings ("UNC paths are not supported" — MSBuild's custom
build steps shell out via `cmd.exe` internally, which silently falls back to
a default directory instead of the UNC path) culminating in a real fatal
error: `cl : error D8004: '/FI' nécessite un argument` (a compiler flag
built from a broken/empty path). **Lesson: MSBuild custom build steps
(vendored SDL3's codegen) cannot run from a UNC path as the working
directory — must build from a real local Windows drive.**

### Fix: copy the repo to a local drive, rebuild — succeeds

```powershell
robocopy \\wsl.localhost\Ubuntu-24.04\home\user\sotn-decomp C:\dev\sotn-decomp-win /E /XD ".git" "build" "disks\pspeu" /XF "disks\sotn.pspeu.iso" /R:2 /W:2
cd C:\dev\sotn-decomp-win
cmake -B build\pc-win -DCMAKE_BUILD_TYPE=Debug -DUSE_PLAYER_MARIA=1
cmake --build build\pc-win --config Debug
```
~4GB copied (9 files failed to copy, likely symlinks/locked submodule
internals — not fatal). Build succeeds this time (only ordinary decomp-in-
progress warnings: C4013 implicit-declaration, C4244 narrowing conversions,
C4091 empty typedef — all expected/harmless). Produces
`build\pc-win\Debug\sotn.exe`.

### Running it — real GPU works, but a real crash found

**Gotcha**: must run with cwd = repo root, not the `Debug\` folder — the
game resolves asset paths like `disks/us/BIN/F_TITLE0.BIN` relative to cwd,
not relative to the `--disk` argument's directory. Running from inside
`Debug\` gives repeated `unable to open 'disks/us/BIN/F_TITLE0.BIN'` and an
all-black window.

```powershell
cd C:\dev\sotn-decomp-win
.\build\pc-win\Debug\sotn.exe --disk disks\sotn.us.cue --stage nz0
```

With the correct cwd: SDL_GPU initializes with real `vulkan` driver, audio
initializes successfully (`Psyz_AudioInit] audio initialized` — unlike WSL's
"No available audio device"), loads title screen, ARC player, NZ0 stage
files, VAB banks, and gets to `TODO: will load weapon 'w1_008'` / `f1_008`
— then **crashes**: exit code `-1073741819` = `0xC0000005` =
`STATUS_ACCESS_VIOLATION`, a classic null/invalid-pointer dereference,
right in the neighborhood of weapon-loading code
(`src/pc/sim_pc.c` around line 565-571, and/or `src/pc/weapon_pc.c`).
This is a genuine, reproducible decomp/port bug — matches the wiki's
documented "Typical Bugs" categories (pointer type width, uninitialized
pointers) exactly, not an environment/driver issue this time.

**Next step**: attach a real debugger to get an exact crash line/stack trace
— current toolchain is Build Tools only (no full Visual Studio IDE debugger
UI installed). Options: install the full VS Community IDE, or WinDbg, or
narrow down via targeted logging/bisection in `sim_pc.c`/`weapon_pc.c`
around the weapon-loading path. Not yet attempted.

### WinDbg installed, exact crash root-caused

```powershell
winget install --id Microsoft.WinDbg -e --accept-package-agreements --accept-source-agreements
```

Launched via File → Launch executable (advanced...): executable
`build\pc-win\Debug\sotn.exe`, args `--disk disks\sotn.us.cue --stage nz0`,
**start directory must be set to `C:\dev\sotn-decomp-win`** (same cwd gotcha
as above — WinDbg doesn't infer it from the exe path). `g` to run, crash hits
automatically, then `!analyze -v` gives a full report immediately without
needing manual breakpoints.

**Exact crash location** (`!analyze -v` → `FAULTING_SOURCE_LINE`):
```
src\dra\4B758.c @ 193, function RenderEntities
   192:         if (r->animSet > 0) {
>  193:             animFrame = D_800A3B70[r->animSet][pose];
```
`D_800A3B70` (`src/dra/d_37d8.c:360`, `extern s16** D_800A3B70[18]` in
`dra.h:583`) is a legitimate 18-entry array of real named symbols (not raw
address literals) — the table itself isn't the bug.

**Confirmed via `dv` (locals) at the crash frame**:
- `r->animSet` = `9727` (0n9727) — wildly out of range for an 18-entry table
- `pose` = `0x59b6` (22966)
- `r->animSet` is a direct copy of `entity->animSet` (line 174:
  `r->animSet = entity->animSet`), so the corruption is in the `Entity`
  struct's data itself, not in `EntitiesRenderer` (`r`'s type, defined
  locally in `4B758.c:138` — already has a `#ifdef VERSION_PC` branch using
  real stack locals instead of the original PS1 scratchpad-pointer trick, so
  that part's already correctly ported).

**Struct layout drift confirmed** (`dt sotn!Entity` in WinDbg vs the
documented PS1 offsets in `game.h`):
```
+0x028 pfnUpdate : Ptr64 void            (was a 4-byte PS1 pointer)
+0x050 anim      : Ptr64 AnimationFrame  (documented as 0x4C on PS1)
+0x05c animSet   : Int2B                 (documented as 0x54 on PS1 — now +8)
```
Confirms the "Struct alignment" bug class from the wiki's Typical Bugs
section: `Entity` contains pointer fields (`pfnUpdate`, `anim`, `parent`,
`nextPart`, `unkB8`) that are 8 bytes here vs 4 bytes on PS1, so the real
compiled struct is larger than the documented `size = 0xBC`. This alone is
harmless as long as ALL code accesses fields by name (the compiler always
computes the correct offset) — the actual bug must be some OTHER code path
that assumes the old PS1 byte layout/size directly (raw offset math, a
hardcoded size constant, or a raw memory copy sized for the old struct).

**Searched for an obvious hardcoded-size culprit, found none**:
```powershell
Select-String -Path "src\**\*.c" -Pattern "0xBC\b"
```
All hits are unrelated (`POSE()` macro data, color/palette constants,
config tables) — no hardcoded `Entity` struct size literal found this way.
Root cause of the actual `entity->animSet` corruption is NOT yet identified
— would need real interactive debugging (conditional breakpoint on entity
corruption, or stepping backward through whatever populates/updates
entities before this frame) to pin down. Stopped here for the session; this
is a legitimate, well-diagnosed starting point for whoever picks this up
next (possibly worth asking on Discord too, since it may be a known
PC-port rough edge already).

**Status**: first PC port attempt complete. Full native Windows build
works, runs further than WSL (real GPU, real audio), crashes on a genuine,
well-localized decomp bug rather than an environment issue. Good foundation
for a future session — next step is either (a) interactive step-through
debugging around entity update code to find what still assumes the old
32-bit `Entity` layout, or (b) asking upstream if this is already known.
