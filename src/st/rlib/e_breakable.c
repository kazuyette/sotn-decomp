// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

enum BreakableDebrisSteps {
    INIT,
    UPDATE,
    DEBRIS_NOP = 256,
};

extern EInit RLIB_EInitBreakable;
extern EInit g_EInitInteractable;
extern EInit g_EInitParticle;
extern u8* D_us_80180874[];
extern u8 D_us_801808DC[];
extern u8 D_us_8018089C[];
extern u16 D_us_801808C8[];
extern u16 D_us_801808E8[];
extern u16 D_us_801808B4[];
extern u16 D_us_801808FC[];
extern u8 D_us_801808A8[];
extern u8 D_us_8018082C[];
extern u8 D_us_80180824[];

void RLIB_EntityBreakable(Entity* self) {
    Entity* entity;
    u16 breakableType;

    breakableType = self->params >> 12;
    if (!self->step) {
        InitializeEntity(RLIB_EInitBreakable);
        self->zPriority = g_unkGraphicsStruct.g_zEntityCenter - 20;
        if (breakableType < 5) {
            entity = self + 1;
            CreateEntityFromEntity(E_BREAKABLE_HELPER, self, entity);
            entity->params = breakableType;
            entity->zPriority = self->zPriority;
        }
        self->blendMode = D_us_801808DC[breakableType];
        self->hitboxHeight = D_us_8018089C[breakableType];
        self->animSet = D_us_801808C8[breakableType];
        self->unk5A = D_us_801808E8[breakableType];
        self->palette = D_us_801808B4[breakableType];
        self->hitboxOffY = D_us_801808FC[breakableType];
    }
    AnimateEntity(D_us_80180874[breakableType], self);
    if (self->hitParams) {
        entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (entity != NULL) {
            CreateEntityFromCurrentEntity(E_EXPLOSION, entity);
            entity->params = D_us_801808A8[breakableType];
        }
        switch (breakableType) {
        case 2:
        case 3:
            PlaySfxPositional(0x61D);
            break;
        case 9:
            entity = AllocEntity(&g_Entities[160], &g_Entities[192]);
            if (entity != NULL) {
                CreateEntityFromCurrentEntity(E_BREAKABLE_DEBRIS, entity);
                entity->params = 256;
            }
            g_api.PlaySfx(0x61D);
            entity = AllocEntity(&g_Entities[160], &g_Entities[192]);
            if (entity != NULL) {
                CreateEntityFromEntity(E_HEART_DROP, self, entity);
                entity->params = self->params & 0x1FF;
            }
            PreventEntityFromRespawning(self);
            DestroyEntity(self);
            return;
        case 7:
            g_api.PlaySfx(0x61D);
            entity = AllocEntity(&g_Entities[160], &g_Entities[192]);
            if (entity != NULL) {
                CreateEntityFromEntity(E_HEART_DROP, self, entity);
                entity->params = self->params & 0x1FF;
            }
            PreventEntityFromRespawning(self);
            DestroyEntity(self);
            return;
        case 8:
            g_api.PlaySfx(0x61D);
            entity = AllocEntity(&g_Entities[160], &g_Entities[192]);
            if (entity != NULL) {
                CreateEntityFromEntity(E_HEART_DROP, self, entity);
                entity->params = 3;
            }
            PreventEntityFromRespawning(self);
            DestroyEntity(self);
            return;
        default:
            g_api.PlaySfx(0x61B);
            break;
        }
        ReplaceBreakableWithItemDrop(self);
    }
}

void EntityBreakableHelper(Entity* self) {
    Entity* entity;
    u8* animFrames;
    if (!self->step) {
        InitializeEntity(g_EInitInteractable);
        self->blendMode = BLEND_TRANSP | BLEND_ADD;
        self->animSet = 0x8002;
    }
    animFrames = D_us_8018082C;
    if (!self->params) {
        animFrames = D_us_80180824;
    }
    AnimateEntity(animFrames, self);
    entity = self - 1;
    if (entity->entityId != E_BREAKABLE) {
        if (self->params) {
            self->animCurFrame = 14;
        } else {
            self->animCurFrame = 13;
        }
    }
}

void RLIB_EntityBreakableDebris(Entity* self) {
    Collider collider;
    Primitive* prim;
    switch (self->step) {
    case INIT:
        if (self->params & 256) {
            InitializeEntity(g_EInitInteractable);
            self->animSet = 0x8007;
            self->unk5A = 0x5B;
            self->palette = 0x213;
            self->animCurFrame = 21;
            self->zPriority = 106;
            self->step = DEBRIS_NOP;
        } else {
            InitializeEntity(g_EInitParticle);
        }
        break;
    case UPDATE:
        break;
    }
}
