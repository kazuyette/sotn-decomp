// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_801ADBE8);

INCLUDE_ASM("st/rlib/nonmatchings/unk_2DBE8", func_us_80196768_from_rbo0);

extern EInit D_us_80180658;
extern s32 D_us_801817CC[];

void func_us_801AE414(Entity* self) {
    // offset 0x9C is ext.fishhead.fishheadEntity in ET_Fishhead, but this
    // function reinterprets it as a raw s32 accumulator (different ext
    // union member for whatever entity actually uses this function).
    s32* accum = (s32*)((u8*)self + 0x9C);

    switch (self->step_s) {
    case 0:
        InitializeEntity(D_us_80180658);
        self->animCurFrame = self->params + 0x1C;
        // fallthrough
    case 1:
        MoveEntity();
        self->velocityY += *accum;
        *accum += D_us_801817CC[self->params];
        break;
    }
}


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

extern EInit D_us_8018067C;
extern AnimateEntityFrame D_us_8018189C[];

void EntityFishheadFireball(Entity* self) {
    switch (self->step) {
    case 0:
        InitializeEntity(D_us_8018067C);
        if (self->facingLeft) {
            self->velocityX = 0x60000;
        } else {
            self->velocityX = -0x60000;
        }
        // fallthrough
    case 1:
        MoveEntity();
        AnimateEntity(D_us_8018189C, self);
        if (self->flags & 0x100) {
            DestroyEntity(self);
        }
        break;
    }
}


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
