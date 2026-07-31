// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo2.h"

// All 5 static tables + 2 anim arrays below transcribed byte-for-byte from
// disks/us/BOSS/BO2/BO2.BIN (D_us_80180668-D_us_801806C0), confirmed via
// EntityBreakable's real disassembly (each table's address confirmed via its
// own lui/lw reference). EInitBreakable is defined separately in e_init.c
// (OVL_EXPORT-wrapped, no raw-name conflict).
static u8 anim_1[] = {4, 1, 4, 2, 0, 0, 0, 0};
static u8 anim_2[] = {4, 0, 4, 0, 0, 0, 0, 0};
static u8* g_eBreakableAnimations[8] = {anim_1, anim_2};
static u8 g_eBreakableHitboxes[8] = {0x30, 0x30};
static u8 g_eBreakableExplosionTypes[8] = {0, 0};
static u16 g_eBreakableanimSets[8] = {3, 3};
static u8 blend_modes[8] = {8, 8};

#include "../../st/e_breakable.h"
