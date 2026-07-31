// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo3.h"

void EntityUnkId13(Entity* self);
void EntityExplosionVariants(Entity* self);
void EntityGreyPuff(Entity* self);

// e_misc.h's own function bodies reference 3 EInit/animation globals by
// generic (non-OVL_EXPORT) name, expecting each overlay that includes this
// header to supply the real bo3-local data under these names. Real addresses
// found by reading the actual InitializeEntity/AnimateEntity call sites in
// asm/us/boss/bo3/nonmatchings/e_misc/{EntityUnkId13,EntityBigRedFireball}.s
// (confirmed via a real WSL link failure the first time these were left
// undefined — "undefined reference to g_EInitUnkId13" etc.). All 3 addresses
// are still raw/unsplit in the config, but already have real symbol names
// (glabel) in the retail data disassembly, so the raw extern + #define alias
// pattern (same as BO3_EntityUpdates etc. in bo3.h) applies directly.
extern u16 D_us_80180570[]; // g_EInitUnkId13
extern u8 D_us_80181954[];  // g_bigRedFireballAnim (AnimateEntity frame data)
#define g_EInitUnkId13 D_us_80180570
#define g_bigRedFireballAnim D_us_80181954

// g_EInitParticle = D_us_8018054C, shared with 1CEEC.c/2AFCC.c and this
// file's own func_us_801AE7E4/8E4 (see their own extern comments there).
extern u16 D_us_8018054C[];
#define g_EInitParticle D_us_8018054C

#include "../../st/e_misc.h"

// Boss-specific (Scylla) animation frame data used only by func_us_801AE8E4.
extern u8 D_us_80181AB4[];

// BO3_EntityUpdates (see bo3.h), defined in e_init.c. Declared raw here since
// this is a separate translation unit.
extern PfnEntityUpdate D_us_80180424[];

// CreateEntityFromEntity is defined (non-static, non-OVL_EXPORT) in
// create_entity.c via the shared create_entity.h. e_misc.h itself already
// calls it (e.g. inside EntityUnkId13) via an implicit declaration (no
// prototype is provided anywhere for it in this overlay, matching every
// other overlay that includes this shared header) — we rely on that same
// implicit declaration below rather than adding our own prototype, since
// GCC treats a later explicit prototype as a real "conflicting types" error
// against the implicit one.

// The following 4 functions are genuinely boss-specific (Scylla) content —
// NOT part of the shared e_misc.h header. Hand-decompiled. Each resembles a
// shared helper (EntityIntenseExplosion / MakeEntityFromId) but is
// structurally distinct (confirmed via real disassembly: different
// addresses, and for func_us_801AE9B0, a different dispatch-table index
// convention than MakeEntityFromId). See DECOMP_LOG.md.

// E_INTENSE_EXPLOSION dispatch target. Distinct from the shared
// EntityIntenseExplosion helper (0x801AF7E4, called below via #include).
void func_us_801AE7E4(Entity* self) {
    if (self->step == 0) {
        InitializeEntity(D_us_8018054C);
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
        return;
    }

    self->poseTimer++;
    self->posY.val -= 0x4000;

    if ((self->poseTimer & 1) == 0) {
        self->animCurFrame++;
    }

    if (self->poseTimer >= 0x25) {
        DestroyEntity(self);
    }
}

void func_us_801AE8E4(Entity* self) {
    if (self->step == 0) {
        InitializeEntity(D_us_8018054C);
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
        return;
    }

    MoveEntity();

    if (AnimateEntity(D_us_80181AB4, self) == 0) {
        DestroyEntity(self);
    }
}

// Resembles the shared MakeEntityFromId (e_misc.h) but is NOT identical: the
// pfnUpdate dispatch-table lookup here indexes by entityId directly, while
// MakeEntityFromId (and CreateEntityFromEntity) both use `entityId - 1`.
// Confirmed via real disassembly (no decrement present) — kept as its own
// function rather than aliased.
void func_us_801AE9B0(u16 entityId, Entity* src, Entity* dst) {
    DestroyEntity(dst);
    dst->entityId = entityId;
    dst->pfnUpdate = D_us_80180424[entityId];
    dst->posX.i.hi = src->posX.i.hi;
    dst->posY.i.hi = src->posY.i.hi;
    dst->unk5A = src->unk5A;
    dst->zPriority = src->zPriority;
    dst->flags = FLAG_UNK_2000 | FLAG_NOT_AN_ENEMY | FLAG_KEEP_ALIVE_OFFCAMERA |
                 FLAG_POS_CAMERA_LOCKED | FLAG_DESTROY_IF_BARELY_OUT_OF_CAMERA |
                 FLAG_DESTROY_IF_OUT_OF_CAMERA;
    dst->animSet = src->animSet;

    if (src->palette & PAL_UNK_FLAG) {
        dst->palette = src->hitEffect;
    } else {
        dst->palette = src->palette;
    }
}

// Spawns 6 E_EXPLOSION entities (ext.destructAnim fields, matching the
// layout the shared EntityExplosionVariants/EntityGreyPuff family already
// use) with a shared random angle/kind. Likely called by func_us_801AE7E4's
// dispatch chain as an intense-explosion burst effect.
void func_us_801AEA78(void) {
    u8 spawnKind;
    s16 baseAngle;
    s32 i;
    Entity* newEntity;

    spawnKind = Random() & 3;
    baseAngle = ((Random() & 0xF) << 8) - 0x800;

    for (i = 0; i < 6; i++) {
        newEntity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (newEntity != NULL) {
            CreateEntityFromEntity(E_EXPLOSION, g_CurrentEntity, newEntity);
            newEntity->ext.destructAnim.unk85 = 6 - i;
            newEntity->ext.destructAnim.unk80 = baseAngle;
            newEntity->ext.destructAnim.unk84 = spawnKind;
        }
    }
}
