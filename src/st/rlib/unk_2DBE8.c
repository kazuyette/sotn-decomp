// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

extern EInit g_EInitLion;
extern s16 D_us_80181750[];
extern AnimateEntityFrame D_us_801817C0[];
extern AnimateEntityFrame D_us_80181768[];
extern s16 D_us_80181760[];
extern AnimateEntityFrame D_us_801817A8[];
extern AnimateEntityFrame D_us_80181778[];
extern u16 PLAYER_facingLeft;
extern u16 g_pads_1_pressed;
const char D_us_801A0834[] = "charal %x\n";

void func_us_801ADBE8(Entity* self) {
    if (self->flags & 0x100) {
        if (self->step != 6) {
            PlaySfxPositional(0x732);
            self->hitboxState = 0;
            PlaySfxPositional(0x694);
            SetStep(6);
        }
    }

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitLion);
        CreateEntityFromCurrentEntity(0x1D, self + 1);
        // fallthrough
    case 1:
        if (UnkCollisionFunc3(D_us_80181750) & 1) {
            SetStep(2);
        }
        break;
    case 2:
        AnimateEntity(D_us_801817C0, self);
        self->facingLeft = GetSideToPlayer() & 1;
        if (GetDistanceToPlayerX() < 0x60) {
            SetStep(3);
        }
        break;
    case 3: {
        s32 collision;
        u8 flagByte;

        AnimateEntity(D_us_80181768, self);
        collision = UnkCollisionFunc2(D_us_80181760);
        if (collision & 0x60) {
            *(s32*)&self->posX -= self->velocityX;
        }

        flagByte = *(u8*)((u8*)self + 0x84);
        self->velocityX = (self->facingLeft == flagByte) ? 0xC000 : (s32)0xFFFF4000;

        if (flagByte) {
            self->velocityX -= self->velocityX / 4;
        }

        switch (self->step_s) {
        case 0:
            self->facingLeft = GetSideToPlayer() & 1;
            *(u8*)((u8*)self + 0x84) = 1;
            *(u16*)((u8*)self + 0x82) = 0x60;
            self->step_s += 1;
            // fallthrough
        case 1: {
            s32 distX;

            self->facingLeft = GetSideToPlayer() & 1;
            if (PLAYER_facingLeft != self->facingLeft) {
                *(u16*)((u8*)self + 0x82) -= 1;
            }

            distX = GetDistanceToPlayerX();
            if (distX >= 0x61 && PLAYER_facingLeft == self->facingLeft) {
                SetStep(4);
            } else {
                if (*(s16*)((u8*)self + 0x82) == 0) {
                    self->step_s += 1;
                }
                distX = GetDistanceToPlayerX();
                if (distX < 0x28) {
                    SetStep(5);
                }
            }
            break;
        }
        case 2: {
            s32 distX;

            self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
            *(u8*)((u8*)self + 0x84) = 0;
            distX = GetDistanceToPlayerX();
            if (distX >= 0xA1) {
                SetStep(2);
            }
            break;
        }
        }

        if (self->hitFlags & 3) {
            self->facingLeft = GetSideToPlayer() & 1;
            SetStep(5);
        }
        break;
    }
    case 4: {
        s32 collision;

        AnimateEntity(D_us_80181768, self);
        collision = UnkCollisionFunc2(D_us_80181760);
        if (collision & 0x60) {
            *(s32*)&self->posX -= self->velocityX;
        }

        self->facingLeft = GetSideToPlayer() & 1;
        if (PLAYER_facingLeft == self->facingLeft) {
            *(u16*)((u8*)self + 0x82) = 0x20;
            *(u8*)((u8*)self + 0x84) = 0;
        } else {
            u16 counter = *(u16*)((u8*)self + 0x82) - 1;
            *(u16*)((u8*)self + 0x82) = counter;
            if (counter == 0) {
                *(u8*)((u8*)self + 0x84) = 1;
                SetStep(3);
                break;
            }
        }

        {
            u8 flagByte = *(u8*)((u8*)self + 0x84);

            self->velocityX = (self->facingLeft == flagByte) ? 0xC000 : (s32)0xFFFF4000;
            if (flagByte) {
                self->velocityX -= self->velocityX / 4;
            }
        }

        switch (self->step_s) {
        case 0:
            self->facingLeft = GetSideToPlayer() & 1;
            *(u16*)((u8*)self + 0x80) = 0x30;
            self->step_s += 1;
            // fallthrough
        case 1:
            self->velocityX -= self->velocityX / 2;
            if (g_Timer & 1) {
                self->poseTimer += 1;
            }
            {
                u16 counter = *(u16*)((u8*)self + 0x80) - 1;
                *(u16*)((u8*)self + 0x80) = counter;
                if (counter == 0) {
                    self->step_s += 1;
                }
            }
            if (GetDistanceToPlayerX() < 0x40) {
                SetStep(5);
            }
            break;
        case 2:
            if (GetDistanceToPlayerX() < 0x40) {
                SetStep(5);
            }
            break;
        }

        if (self->hitFlags & 3) {
            self->facingLeft = GetSideToPlayer() & 1;
            SetStep(5);
        }
        break;
    }
    case 5:
        if (self->pose == 6) {
            PlaySfxPositional(0x6C6);
        }
        if (!AnimateEntity(D_us_80181778, self)) {
            SetStep(3);
        }
        break;
    case 6:
        switch (self->step_s) {
        case 0: {
            if ((g_Timer & 3) == 0) {
                Entity* entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
                if (entity != NULL) {
                    CreateEntityFromEntity(6, self, entity);
                    *(s16*)((u8*)entity + 0x2) += 0x20 - (Random() & 0x2F);
                    *(s16*)((u8*)entity + 0x6) += 0x20 - (Random() & 0x3F);
                }
            }
            if ((g_Timer & 7) == 0) {
                PlaySfxPositional(0x691);
            }
            if (!AnimateEntity(D_us_801817A8, self)) {
                self->step_s += 1;
            }
            break;
        }
        case 1: {
            s32 i;

            PlaySfxPositional(0x6CA);
            for (i = 0; i < 6; i++) {
                Entity* entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
                if (entity != NULL) {
                    CreateEntityFromEntity(0x1E, self, entity);
                    entity->params = i;
                    entity->facingLeft = self->facingLeft;
                }
            }
            DestroyEntity(self);
            break;
        }
        }
        break;
    case 0xFF:
        FntPrint(D_us_801A0834, self->animCurFrame);
        if (g_pads_1_pressed & 0x80) {
            if (self->params == 0) {
                self->animCurFrame += 1;
                self->params |= 1;
            }
        } else {
            self->params = 0;
        }
        if (g_pads_1_pressed & 0x20) {
            if (self->step_s == 0) {
                self->animCurFrame -= 1;
                self->step_s |= 1;
            }
        } else {
            self->step_s = 0;
        }
        break;
    }
}


extern EInit D_us_8018064C;
extern u8 D_us_80181804[];

typedef struct {
    /* 0x0 */ s8 offX;
    /* 0x1 */ s8 offY;
    /* 0x2 */ u8 width;
    /* 0x3 */ u8 height;
} UnkStruct_801817E4;

extern UnkStruct_801817E4 D_us_801817E4[];

void func_us_80196768_from_rbo0(Entity* self) {
    Entity* parent = self - 1;
    UnkStruct_801817E4* s;

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_8018064C);
        // fallthrough
    case 1:
        self->facingLeft = parent->facingLeft;
        self->posX = parent->posX;
        self->posY = parent->posY;

        s = &D_us_801817E4[D_us_80181804[parent->animCurFrame]];
        self->hitboxOffX = s->offX;
        self->hitboxOffY = s->offY;
        self->hitboxWidth = s->width;
        self->hitboxHeight = s->height;

        if (parent->entityId != 0x1C) {
            DestroyEntity(self);
        }
        break;
    }
}


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


extern EInit g_EInitTinman;
extern s16 D_us_80181820[];
extern s16 D_us_80181830[];
extern AnimateEntityFrame D_us_80181838[];
extern AnimateEntityFrame D_us_801818B8[];
extern AnimateEntityFrame D_us_80181844[];
extern AnimateEntityFrame D_us_80181880[];
const char D_us_801A0840[] = "charal %x\n";

void func_us_801AE534(Entity* self) {
    if (self->step != 0) {
        s16 val = *(s16*)((u8*)g_api.enemyDefs + 0x2944);
        if (val < 0) {
            val += 3;
        }
        val >>= 2;
        if (self->hitPoints < val && self->step < 5) {
            SetStep(5);
        }
    }

    if (self->hitFlags & 3) {
        PlaySfxPositional(0x73D);
    }
    if (self->flags & 0x100) {
        if (self->step != 7) {
            SetStep(7);
        }
    }

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitTinman);
        self->hitboxOffY = 9;
        CreateEntityFromCurrentEntity(0x20, self + 1);
        (self + 1)->zPriority = self->zPriority + 1;
        // fallthrough
    case 1:
        if (UnkCollisionFunc3(D_us_80181820) & 1) {
            SetStep(2);
        }
        break;
    case 2:
        if (GetDistanceToPlayerX() < 0x60) {
            SetStep(3);
        }
        break;
    case 3: {
        s32 exceeded;
        s32 clamped = 0;
        s32 spawnFlag;

        AnimateEntity(D_us_80181838, self);

        *(u8*)((u8*)self + 0x84) = 0;
        if (self->facingLeft) {
            self->velocityX += 0x800;
            exceeded = (0x18000 < self->velocityX);
        } else {
            self->velocityX -= 0x800;
            exceeded = (self->velocityX < (s32)0xFFFE8000);
        }

        if (exceeded) {
            self->velocityX = self->facingLeft ? 0x18000 : (s32)0xFFFE8000;
            clamped = 1;
        }

        if (clamped) {
            spawnFlag = (Random() & 0x1F) < 1;
            if (self->poseTimer != 0) {
                self->poseTimer -= 1;
            }
        } else {
            spawnFlag = (g_Timer & 0xF) < 1;
        }

        if (spawnFlag) {
            Entity* entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
            if (entity != NULL) {
                CreateEntityFromEntity(0x22, self, entity);
                *(Entity**)((u8*)entity + 0x9C) = self;
                entity->facingLeft = self->facingLeft;
                entity->zPriority = self->zPriority + 1;
            }
        }

        if (UnkCollisionFunc2(D_us_80181830) & 0x60) {
            *(s32*)&self->posX -= self->velocityX;
            self->velocityX = -self->velocityX;
        }

        if (GetDistanceToPlayerX() < 0x50) {
            *(u16*)((u8*)self + 0x82) = (*(u16*)((u8*)self + 0x82) - 0x100) & 0xFFF;
        } else {
            func_us_801AE4B4((s16*)((u8*)self + 0x82), 0, 0x80);
        }

        if ((GetSideToPlayer() & 1) != self->facingLeft) {
            break;
        }
        if (GetDistanceToPlayerX() < 0x31) {
            break;
        }
        SetStep(4);
        break;
    }
    case 4: {
        s32 collision;

        self->velocityX -= self->velocityX / 32;

        collision = UnkCollisionFunc2(D_us_80181830);
        if (collision & 0x60) {
            *(s32*)&self->posX -= self->velocityX;
            self->velocityX = -self->velocityX;
        }

        switch (self->step_s) {
        case 0:
            *(u8*)((u8*)self + 0x84) = 0;
            if (func_us_801AE4B4((s16*)((u8*)self + 0x82), 0, 0x80)) {
                self->step_s += 1;
            }
            break;
        case 1:
            *(u8*)((u8*)self + 0x84) = 1;
            if (!AnimateEntity(D_us_801818B8, self)) {
                *(u8*)((u8*)self + 0x84) = 0;
                self->animCurFrame = 1;
                self->facingLeft ^= 1;
                SetStep(3);
            }
            break;
        }
        break;
    }
    case 5:
        switch (self->step_s) {
        case 0:
            self->velocityX = (GetSideToPlayer() & 1) ? 0x10000 : (s32)0xFFFF0000;
            self->velocityY = (s32)0xFFFE0000;
            self->drawFlags = 4;
            self->step_s += 1;
            // fallthrough
        case 1:
            func_us_801AE4B4((s16*)&self->rotate, 0x80, 0x10);
            if (UnkCollisionFunc3(D_us_80181820) & 1) {
                self->step_s += 1;
            }
            break;
        case 2:
            if (func_us_801AE4B4((s16*)&self->rotate, 0, 0x10)) {
                *(u8*)((u8*)self + 0x85) = 1;
                self->drawFlags = 0;
                self->rotate = 0;
                self->step_s += 1;
            }
            break;
        case 3:
            if (!AnimateEntity(D_us_80181844, self)) {
                self->hitboxWidth = 0xF;
                self->hitboxHeight = 0x11;
                self->hitboxOffX = 2;
                self->hitboxOffY = 0xF;
                *(u16*)((u8*)self + 0x80) = Random() & 0xF;
                SetStep(6);
            }
            break;
        }
        break;
    case 6: {
        AnimateEntity(D_us_80181880, self);
        if ((*(u16*)((u8*)self + 0x80) & 7) == 0) {
            PlaySfxPositional(0x658);
        }
        *(u16*)((u8*)self + 0x80) += 1;
        if ((*(u16*)((u8*)self + 0x80) & 3) == 0) {
            Entity* entity = AllocEntity(&g_Entities[160], &g_Entities[192]);
            if (entity != NULL) {
                CreateEntityFromEntity(0x21, self, entity);
                entity->facingLeft = self->facingLeft;
                *(s16*)((u8*)entity + 0x6) = *(s16*)((u8*)entity + 0x6) - 7 - ((*(u16*)((u8*)self + 0x80) & 7) >> 1);
                if (self->facingLeft) {
                    *(s16*)((u8*)entity + 0x2) -= 2;
                } else {
                    *(s16*)((u8*)entity + 0x2) += 2;
                }
            }
        }
        break;
    }
    case 7:
        UnkCollisionFunc3(D_us_80181820);
        func_us_801AE4B4((s16*)&self->rotate, 0, 0x20);
        switch (self->step_s) {
        case 0:
            if (*(u8*)((u8*)self + 0x85)) {
                self->hitboxState = 0;
                self->step_s = 2;
            } else {
                self->step_s = 1;
            }
            break;
        case 1:
            if (!AnimateEntity(D_us_80181844, self)) {
                self->step_s += 1;
            }
            break;
        case 2: {
            s32 i;

            for (i = 0; i < 0xB; i++) {
                Entity* entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
                if (entity != NULL) {
                    CreateEntityFromEntity(0x23, self, entity);
                    entity->params = i + 0x3B;
                    entity->facingLeft = self->facingLeft;
                }
            }
            PlaySfxPositional(0x6CA);
            DestroyEntity(self);
            break;
        }
        }
        break;
    case 0xFF:
        FntPrint(D_us_801A0840, self->animCurFrame);
        if (g_pads_1_pressed & 0x80) {
            if (self->params == 0) {
                self->animCurFrame += 1;
                self->params |= 1;
            }
        } else {
            self->params = 0;
        }
        if (g_pads_1_pressed & 0x20) {
            if (self->step_s == 0) {
                self->animCurFrame -= 1;
                self->step_s |= 1;
            }
        } else {
            self->step_s = 0;
        }
        break;
    }
}


extern EInit D_us_80180670;

void func_us_801AED4C(Entity* self) {
    Entity* parent = self - 1;

    self->posX = parent->posX;
    self->facingLeft = parent->facingLeft;
    self->posY = parent->posY;

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_80180670);
        self->drawFlags = 4;
        // fallthrough
    case 1: {
        u8 flag = *(u8*)((u8*)parent + 0x84);

        if (flag == 1) {
            self->hitboxState = 0;
            self->animCurFrame = 0;
        } else {
            s32 angle;
            s32 c, sn;

            self->rotate = *(u16*)((u8*)parent + 0x82);
            self->animCurFrame = 5;
            angle = self->rotate;
            self->hitboxState = 1;
            angle += 0x200;
            angle = (s16)angle;

            c = rcos(angle);
            self->hitboxOffX = (u32)((c << 3) - c) >> 10;
            sn = rsin(angle);
            self->hitboxOffY = (u32)((sn << 3) - sn) >> 10;

            if (self->rotate < 0xF00) {
                PlaySfxPositional(0x6C7);
            }
        }

        if (*((u8*)parent + 0x85) != 0 || (parent->flags & 0x100)) {
            self->hitboxState = 0;
            self->step += 1;
        }
        break;
    }
    case 2: {
        Entity* entity;
        s32 i;

        entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (entity != NULL) {
            CreateEntityFromEntity(0x23, self, entity);
            entity->facingLeft = self->facingLeft;
            entity->rotate = self->rotate;
            entity->params = 5;
            entity->zPriority = self->zPriority;
        }

        for (i = 0; i < 8; i++) {
            entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
            if (entity != NULL) {
                CreateEntityFromEntity(0x23, self, entity);
                entity->params = 0;
                entity->zPriority = self->zPriority;
            }
        }

        PlaySfxPositional(0x645);

        self->hitboxWidth = 0xF;
        self->hitboxHeight = 0xB;
        self->hitboxOffX = 0x17;
        self->hitboxOffY = -5;
        self->parent = parent;
        self->animCurFrame = 0;
        self->hitboxState = parent->hitboxState;
        self->attackElement = parent->attackElement;
        self->attack = parent->attack;
        self->step += 1;
        break;
    }
    case 3:
        if (parent->entityId != 0x1F || (parent->flags & 0x100)) {
            DestroyEntity(self);
        }
        break;
    }
}


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


extern EInit g_EInitScarecrow;
extern s16 D_us_80181984[];
const char D_us_801A084C[] = "charal %x\n";
extern u16 g_pads_1_pressed;

void func_us_801AF448(Entity* self) {
    if ((self->flags & 0x100) && self->step != 3) {
        SetStep(3);
    }

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitScarecrow);
        self->drawFlags = 4;
        self->animCurFrame = 2;
        self->hitboxWidth = 0xC;
        self->hitboxHeight = 8;
        self->hitboxOffY = -6;
        if ((Random() & 0xF) == 0) {
            self->params |= 1;
        }
        if (self->params != 0) {
            self->animCurFrame = 7;
            self->hitboxWidth = 9;
            self->hitboxHeight = 0xC;
            self->hitboxOffY = -6;
            self->palette -= 1;
            return;
        }
        {
            s32 i;

            for (i = 0; i < 4; i++) {
                Entity* child = &self[i + 1];

                CreateEntityFromEntity(0x25, self, child);
                child->params = i;
                child->nextPart = &self[i];
                child->parent = self;
            }
            self->nextPart = &self[4];
        }
        // fallthrough
    case 1: {
        s32 collisionBuf[16];
        s32 collided;

        switch (self->step_s) {
        case 0:
            self->velocityY = (s32)0xFFFD8000;
            self->velocityX = self->facingLeft ? 0x18000 : (s32)0xFFFE8000;
            self->step_s += 1;
            // fallthrough
        case 1:
            MoveEntity();
            self->velocityY += 0x2000;
            if (self->velocityY > 0) {
                func_us_801AF3C8((s16*)&self->rotate, 0x100, 8);
            } else {
                func_us_801AF3C8((s16*)&self->rotate, -0x100, 0x10);
            }

            g_api.CheckCollision(*(s16*)((u8*)self + 0x2), *(s16*)((u8*)self + 0x6) + 0x30, collisionBuf, 0);
            collided = collisionBuf[0];
            if (collided & 1) {
                s32 offset = collisionBuf[6];

                PlaySfxPositional(0x649);
                self->step_s = 0;

                g_api.CheckCollision(*(s16*)((u8*)self + 0x2), *(s16*)((u8*)self + 0x6) + 0x28, collisionBuf, 0);
                collided = collisionBuf[0];
                if (collided & 1) {
                    self->facingLeft ^= 1;
                    return;
                }

                *(s16*)((u8*)self + 0x6) += offset;

                {
                    s32 sideFlags = GetSideToPlayer();
                    s32 flag = (sideFlags & 1) ^ 1;
                    s32 distX = GetDistanceToPlayerX();

                    if (flag != self->facingLeft && distX >= 0x41) {
                        self->facingLeft ^= 1;
                        if ((Random() & 3) == 0) {
                            SetStep(2);
                        }
                    }
                }
            }
            break;
        }
        break;
    }
    case 2: {
        s32 side;

        if (self->step_s == 0) {
            self->velocityX = 0;
            self->velocityY = 0;
            (*(u16*)((u8*)self + 0x80)) = (Random() & 0x1F) + 0x20;
            self->step_s += 1;
        }

        side = UnkCollisionFunc2(D_us_80181984);
        if (self->facingLeft) {
            self->velocityX += 0x1000;
        } else {
            self->velocityX -= 0x1000;
        }
        if (side == 0xFF) {
            (*(u16*)((u8*)self + 0x80)) = 1;
        }

        if (--(*(u16*)((u8*)self + 0x80)) == 0) {
            self->facingLeft = GetSideToPlayer() & 1;
            SetStep(1);
        }
        break;
    }
    case 3: {
        Entity* entity;
        s32 angle;
        s32 amplitude;
        s32 dx, dy;
        s32 i;

        entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
        if (entity != NULL) {
            CreateEntityFromEntity(E_EXPLOSION, self, entity);
            entity->params = 3;
        }

        angle = self->rotate + 0x400;
        if (self->facingLeft) {
            angle = 0x800 - angle;
        }
        angle = (s16)angle;

        amplitude = 0x30;
        for (i = 0; i < 8; i++) {
            dx = (amplitude * rcos(angle)) >> 12;
            dy = (amplitude * rsin(angle)) >> 12;
            entity = AllocEntity(&g_Entities[224], &g_Entities[256]);
            amplitude -= 8;
            if (entity != NULL) {
                CreateEntityFromEntity(E_EXPLOSION_PUFF_OPAQUE, self, entity);
                entity->params = 3;
                entity->rotate = self->rotate;
                *(s16*)((u8*)entity + 0x6) = *(s16*)((u8*)entity + 0x6) + dy;
                *(s16*)((u8*)entity + 0x2) = *(s16*)((u8*)self + 0x2) + dx;
            }
        }

        PlaySfxPositional(0x62C);
        DestroyEntity(self);
        break;
    }
    case 0xFF:
        FntPrint(D_us_801A084C, self->animCurFrame);
        if (g_pads_1_pressed & 0x80) {
            if (self->params == 0) {
                self->animCurFrame += 1;
                self->params |= 1;
            }
        } else {
            self->params = 0;
        }
        if (g_pads_1_pressed & 0x20) {
            if (self->step_s == 0) {
                self->animCurFrame -= 1;
                self->step_s |= 1;
            }
        } else {
            self->step_s = 0;
        }
        break;
    }
}


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
        (*(s32*)((u8*)self + 0x84)) = parent->velocityX;
        (*(s32*)((u8*)self + 0x88)) = parent->velocityY;
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
            s32 velDiffX = parent->velocityX - (*(s32*)((u8*)self + 0x84));
            s32 velDiffY = (*(s32*)((u8*)self + 0x88)) - parent->velocityY;
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

        (*(s32*)((u8*)self + 0x84)) = parent->velocityX;
        (*(s32*)((u8*)self + 0x88)) = parent->velocityY;
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

