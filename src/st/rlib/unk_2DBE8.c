// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801ADBE8);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_80196768_from_rbo0);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AE414);

s32 func_us_801AE4B4(s16* value, s16 target, s16 step) {
    s16 current = *value;
    s16 diff = current - target;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff < step) {
        *value = target;
        return 1;
    }
    if (target < current) {
        *value = current - step;
    }
    if (*value < target) {
        *value = step + *value;
    }
    return 0;
}


INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AE534);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AED4C);

extern EInit D_us_80180688;
extern u8 D_us_80181888[];

void func_us_801AEFE0(Entity* self) {
    switch (self->step) {
    case 0:
        InitializeEntity(D_us_80180688);
        self->blendMode = BLEND_TRANSP;
        PlaySfxPositional(0x673);
        // fallthrough
    case 1: {
        Entity* fishheadEntity = self->ext.fishhead.fishheadEntity;
        self->posX = fishheadEntity->posX;
        self->posY = fishheadEntity->posY;
        if (!AnimateEntity(D_us_80181888, self)) {
            DestroyEntity(self);
        }
        break;
    }
    }
}

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", EntityFishheadFireball);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AF11C);

s32 func_us_801AF3C8(s16* value, s16 target, s16 step) {
    s16 current = *value;
    s16 diff = current - target;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff < step) {
        *value = target;
        return 1;
    }
    if (target < current) {
        *value = current - step;
    }
    if (*value < target) {
        *value = step + *value;
    }
    return 0;
}


INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AF448);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AF9E8);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801AFC88);
