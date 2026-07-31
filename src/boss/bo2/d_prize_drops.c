// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo2.h"

// Transcribed byte-for-byte from disks/us/BOSS/BO2/BO2.BIN (D_us_80180CA4,
// 16 entries, 0xCA4-0xCC4 -- confirmed via EntityHeartDrop's real
// disassembly, boundary confirmed against the following bytes turning into
// unrelated large values at 0xCC4).
u16 OVL_EXPORT(PrizeDrops)[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0000, 0x0001, 0x0000,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0101,
};
