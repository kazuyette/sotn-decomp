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


extern EInit D_us_80180688;
extern AnimateEntityFrame D_us_801818A8[];

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 pad3;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} UnkStruct_801818D0; // size 0xC

extern UnkStruct_801818D0 D_us_801818D0[];

void func_us_801AF11C(Entity* self) {
    Entity* entity;
    u16* pUnk80 = (u16*)((u8*)self + 0x80);
    u8* pUnk86 = (u8*)self + 0x86;

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_80180688);
        if (self->params == 0) {
            s16 speed;
            s16 angle;

            self->step = 1;
            speed = (Random() & 0x1F) + 0x10;
            angle = Random() * 6 + 0x900;
            self->velocityX = speed * rcos(angle);
            self->velocityY = speed * rsin(angle);
        } else {
            self->animCurFrame = self->params;
            if (self->params < 0x3B) {
                self->drawFlags = 4;
                self->step = 2;
            } else {
                UnkStruct_801818D0* s = &D_us_801818D0[self->params - 0x3B];

                self->velocityX = self->facingLeft ? -s->unk4 : s->unk4;
                self->velocityY = s->unk8;
                *pUnk80 = s->unk0;
                *pUnk86 = s->unk2;
                self->step = 3;
            }
        }
        break;
    case 1:
        AnimateEntity(D_us_801818A8, self);
        MoveEntity();
        self->velocityY += 0x1800;
        break;
    case 2:
        MoveEntity();
        self->velocityY += 0x1800;
        self->rotate += 0x80;
        break;
    case 3:
        switch (self->step_s) {
        case 0:
            if (--(*pUnk80) == 0) {
                *pUnk80 = 0x20;
                self->step_s += 1;
            }
            break;
        case 1:
            MoveEntity();
            if (*pUnk86) {
                self->velocityY += 0x2000;
            }
            if (--(*pUnk80) == 0) {
                PlaySfxPositional(0x655);
                entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
                if (entity != NULL) {
                    CreateEntityFromEntity(E_EXPLOSION, self, entity);
                    entity->params = 1;
                    *(u16*)((u8*)entity + 0x6) += 0x10;
                }
                DestroyEntity(self);
            }
            break;
        }
        break;
    }
}


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

extern EInit D_us_801806A0;

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
} UnkStruct_80181954; // size 0xC

extern UnkStruct_80181954 D_us_80181954[];

void func_us_801AF9E8(Entity* self) {
    Entity* parent;
    UnkStruct_80181954* s5;
    s16* pUnk2 = (s16*)((u8*)self + 0x2);
    s16* pUnk6 = (s16*)((u8*)self + 0x6);
    s32 dx, dy;
    s32 rotSin, rotCos;

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_801806A0);
        parent = self - (self->params + 1);
        self->drawFlags |= 4;
        self->animCurFrame = D_us_80181954[self->params].unk0;
        self->zPriority = parent->zPriority + D_us_80181954[self->params].unk2;
        self->unk84 = parent->velocityX;
        self->unk88 = parent->velocityY;
        // fallthrough
    case 1:
        parent = self - (self->params + 1);
        *pUnk2 = *(s16*)((u8*)parent + 0x2);
        *pUnk6 = *(s16*)((u8*)parent + 0x6);
        self->facingLeft = parent->facingLeft;
        self->palette = parent->palette;
        s5 = &D_us_80181954[self->params];

        rotSin = rsin(parent->rotate);
        rotCos = rcos(parent->rotate);
        dx = (s5->unk4 * rotCos - s5->unk6 * rotSin) >> 12;
        dy = (s5->unk4 * rotSin + s5->unk6 * rotCos) >> 12;

        *pUnk2 += self->facingLeft ? -dx : dx;
        *pUnk6 += dy;

        {
            s32 velDiffX = parent->velocityX - self->unk84;
            s32 velDiffY = self->unk88 - parent->velocityY;
            if (self->facingLeft) {
                velDiffX = -velDiffX;
            }
            self->rotate = self->rotate + (velDiffX >> 8) + (velDiffY >> 8);
            func_us_801AF3C8((s16*)&self->rotate, 0, 16);
        }

        if (s5->unkA < self->rotate) {
            self->rotate = s5->unkA;
        }
        if (self->rotate < s5->unk8) {
            self->rotate = s5->unk8;
        }

        self->unk84 = parent->velocityX;
        self->unk88 = parent->velocityY;
        if (parent->entityId != 0x24) {
            DestroyEntity(self);
        }
        break;
    }
}


extern EInit g_EInitSchmoo;
extern AnimateEntityFrame D_us_8018198C[];
extern AnimateEntityFrame D_us_80181998[];

void func_us_801AFC88(Entity* self) {
    u16 step;
    s32 rot;

    if (self->flags & 0x100) {
        Entity* entity;

        PlaySfxPositional(0x655);
        entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (entity != NULL) {
            CreateEntityFromEntity(E_EXPLOSION, self, entity);
            entity->params = 1;
        }
        DestroyEntity(self);
        return;
    }

    step = self->step;
    switch (step) {
    case 0:
        InitializeEntity(g_EInitSchmoo);
        self->drawFlags = 4;
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
        // fallthrough
    case 1: {
        s32 sideFlags;
        s32 distX;
        s32 flag;

        AnimateEntity(D_us_8018198C, self);
        MoveEntity();
        sideFlags = GetSideToPlayer();

        if (self->facingLeft) {
            self->velocityX += 0xC00;
            if (self->velocityX > 0x30000) {
                self->velocityX = 0x30000;
            }
        } else {
            self->velocityX -= 0xC00;
            if (self->velocityX < (s32)0xFFFD0000) {
                self->velocityX = (s32)0xFFFD0000;
            }
        }

        if (sideFlags & 2) {
            self->velocityY -= 0xC00;
            if (self->velocityY < (s32)0xFFFE8000) {
                self->velocityY = (s32)0xFFFE8000;
            }
        } else {
            self->velocityY += 0xC00;
            if (self->velocityY > 0x18000) {
                self->velocityY = 0x18000;
            }
        }

        flag = (sideFlags & 1) ^ 1;
        distX = GetDistanceToPlayerX();
        if (flag != self->facingLeft) {
            if (distX < 0x59) {
                SetStep(2);
            }
        }
        break;
    }
    case 2:
        MoveEntity();
        switch (self->step_s) {
        case 0: {
            s32 absVelX;

            self->velocityX -= self->velocityX >> 4;
            self->velocityY -= self->velocityY >> 4;
            absVelX = self->velocityX;
            if (absVelX < 0) {
                absVelX = -absVelX;
            }
            if (absVelX < 0x4000) {
                self->step_s += 1;
            }
            break;
        }
        case 1: {
            s32 animResult;

            self->velocityX -= self->velocityX >> 8;
            self->velocityY -= self->velocityY >> 8;
            animResult = AnimateEntity(D_us_80181998, self);
            if (!animResult) {
                SetStep(1);
            }
            if (self->pose == step) {
                self->facingLeft ^= 1;
            }
            break;
        }
        }
        break;
    }

    rot = self->velocityX >> 8;
    if (rot < 0) {
        rot = -rot;
    }
    self->rotate = -rot;
}

