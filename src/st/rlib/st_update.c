// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

s32 Random(void) {
    u32 v0 = g_randomNext;
    v0 = (v0 << 8) + v0;
    v0 = v0 + (v0 << 16);
    v0 = v0 + 1;
    g_randomNext = v0;
    return (s32)(v0 >> 24);
}


INCLUDE_ASM("st/rlib/nonmatchings/st_update", Update);

INCLUDE_ASM("st/rlib/nonmatchings/st_update", UpdateStageEntities);
