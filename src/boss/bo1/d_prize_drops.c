// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo1.h"

// Transcribed byte-for-byte from asm/us/boss/bo1/data/0.data.s
// (D_us_80181020, 16 entries, 0x1020-0x1040 — splat already isolated this
// exact boundary since D_us_80181040, a different table, immediately
// follows). Unlike bo4 (ZERO_LEN, no real prize-drop room content),
// Granfaloon's room genuinely has 16 real entries here.
u16 OVL_EXPORT(PrizeDrops)[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0001, 0x0000,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0101,
};
