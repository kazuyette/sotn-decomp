// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

extern EInit RLIB_EInitBreakable;
extern EInit g_EInitInteractable;
extern EInit g_EInitParticle;

extern u8 D_us_801808DC[];  // blendMode per breakableType
extern u8 D_us_8018089C[];  // hitboxHeight per breakableType
extern u16 D_us_801808C8[]; // animSet per breakableType
extern u16 D_us_801808E8[]; // unk5A per breakableType
extern u16 D_us_801808B4[]; // palette per breakableType
extern u16 D_us_801808FC[]; // hitboxOffY per breakableType
extern u8 D_us_801808A8[];  // explosion type per breakableType
extern AnimateEntityFrame* D_us_80180874[]; // animations per breakableType

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

extern AnimateEntityFrame D_us_80180824[]; // used when self->params == 0
extern AnimateEntityFrame D_us_8018082C[]; // used when self->params != 0

void EntityBreakableHelper(Entity* self) {
    Entity* entity;

    if (!self->step) {
        InitializeEntity(g_EInitInteractable);
        self->blendMode = 0x30;
        self->animSet = -0x7FFE;
    }
    if (!self->params) {
        AnimateEntity(D_us_80180824, self);
    } else {
        AnimateEntity(D_us_8018082C, self);
    }
    entity = self - 1;
    if (entity->entityId != E_BREAKABLE) {
        if (!self->params) {
            self->animCurFrame = 13;
        } else {
            self->animCurFrame = 14;
        }
    }
}

void RLIB_EntityBreakableDebris(Entity* self) {
    Collider collider;
    Primitive* prim;

    if (!self->step) {
        if (self->params & 256) {
            InitializeEntity(g_EInitInteractable);
            self->animSet = -0x7FF9;
            self->unk5A = 91;
            self->palette = 0x213;
            self->animCurFrame = 21;
            self->zPriority = 106;
            self->step = 256;
        } else {
            InitializeEntity(g_EInitParticle);
        }
    }
}
