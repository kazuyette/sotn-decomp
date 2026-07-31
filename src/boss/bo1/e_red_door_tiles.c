// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo1.h"

// Transcribed byte-for-byte from disks/us/BOSS/BO1/BO1.BIN (D_us_80180B64,
// offset 0xB64, 16 x u16). Confirmed real values via direct ROM extraction
// (VRAM - 0x80180000), same pattern as bo3's e_red_door_tiles.c.
u16 OVL_EXPORT(RedDoorTiles)[][8] = {
    {0x001F, 0x001D, 0x001D, 0x0020, 0x02A1, 0x0000, 0x0000, 0x02BF},
    {0x001C, 0x001D, 0x001D, 0x001E, 0x072A, 0x0000, 0x0000, 0x075B},
};
