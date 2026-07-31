// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo2.h"

// animFrames for func_us_801AF844 (transcribed byte-for-byte from
// D_us_80181680, AnimateEntityFrame{duration,pose} pairs terminated by a
// 0xFF duration -- see src/st/animate_entity.h's AnimateEntity for the
// termination convention).
static u8 D_us_80181680[] = {0x02, 0x01, 0x02, 0x02, 0x02, 0x03,
                              0x02, 0x04, 0x02, 0x05, 0x04, 0x06, 0xFF, 0x00};

// Dispatch entry 6 (E_UNK_06 in bo2.h) -- a small particle/debris entity.
// step 0: initializes (g_EInitParticle), sets palette/animSet/animCurFrame,
// derives blendMode/palette/zPriority from params. step != 0: ages via
// poseTimer, applies gravity-like velocityY decay, advances animCurFrame
// every other tick, self-destructs after 0x25 ticks.
void func_us_801AF744(Entity* self) {
    if (self->step) {
        self->poseTimer++;
        self->velocityY -= 0x4000;
        if (!(self->poseTimer & 1)) {
            self->animCurFrame++;
        }
        if (self->poseTimer < 0x25) {
            return;
        }
        DestroyEntity(self);
        return;
    }

    InitializeEntity(g_EInitParticle);
    self->palette = 0x8170;
    self->animSet = 5;
    self->animCurFrame = 1;
    self->blendMode = 0x30;
    if (self->params & 0xF0) {
        self->palette = 0x8195;
        self->blendMode = 0x10;
    }
    if (self->params & 0xFF00) {
        self->zPriority = (self->params & 0xFF00) >> 8;
    }
    self->zPriority += 8;
}

// A second small particle entity (address between func_us_801AF744 and
// func_us_801AF910, both boss-specific, not in src/st/e_misc.h). step 0:
// initializes, sets opacity/scale/animSet, derives zPriority/palette from
// params. step != 0: moves via the global MoveEntity()/AnimateEntity(), and
// self-destructs once the animation finishes (AnimateEntity returns 0).
void func_us_801AF844(Entity* self) {
    if (self->step) {
        MoveEntity();
        if (!AnimateEntity(D_us_80181680, self)) {
            DestroyEntity(self);
        }
        return;
    }

    InitializeEntity(g_EInitParticle);
    self->opacity = 0xF0;
    self->scaleX = 0x1A0;
    self->scaleY = 0x1A0;
    self->animSet = 8;
    self->animCurFrame = 1;
    self->zPriority += 0x10;
    if (self->params != 0) {
        self->palette = self->params;
    } else {
        self->palette = 0x8160;
    }
    self->step++;
}

// A boss-specific extended variant of CreateEntityFromEntity (create_entity.h)
// -- same base behavior (DestroyEntity(dest); dest->entityId = entityId;
// dest->pfnUpdate = PfnEntityUpdates[entityId - 1]; posX.i.hi/posY.i.hi
// copied from source) PLUS extra field copies this boss needs
// (unk5A/zPriority/animSet, flags hardcoded to 0xCD002000, and a
// palette-vs-hitEffect conditional based on source's palette high bit).
void func_us_801AF910(u16 entityId, Entity* source, Entity* dest) {
    DestroyEntity(dest);
    dest->entityId = entityId;
    dest->pfnUpdate = (&D_us_80180410)[entityId];
    dest->posX.i.hi = source->posX.i.hi;
    dest->posY.i.hi = source->posY.i.hi;
    dest->unk5A = source->unk5A;
    dest->zPriority = source->zPriority;
    dest->flags = 0xCD002000;
    dest->animSet = source->animSet;
    if (source->palette & 0x8000) {
        dest->palette = source->hitEffect;
    } else {
        dest->palette = source->palette;
    }
}

// Spawns 6 small explosion-like entities (E_EXPLOSION) around
// g_CurrentEntity with randomized angle/scale/timing -- likely this boss's
// death/hit burst effect.
void func_us_801AF9D8(void) {
    s32 i;
    s16 scale;
    u16 angle;
    Entity* entity;

    Random();
    angle = Random() & 3;
    scale = (Random() & 0xF) << 8;
    scale -= 0x800;

    for (i = 0; i < 6; i++) {
        entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (entity != NULL) {
            func_us_801A9D5C(E_EXPLOSION, g_CurrentEntity, entity);
            entity->hitboxWidth = 6 - i;
            entity->params = scale;
            entity->hitboxHeight = angle;
        }
    }
}

// Remaining 18 functions confirmed identical to src/st/e_misc.h (same
// technique as bo1: names/signatures already matched against
// config/symbols.us.bobo2.txt, no orphans found among these -- only the 4
// hand-decompiled functions above were boss-specific).
#include "../../st/e_misc.h"
