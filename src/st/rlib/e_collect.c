// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

static void PrizeDropFall(void) {
    if (g_CurrentEntity->velocityY >= 0) {
        g_CurrentEntity->ext.equipItemDrop.fallSpeed +=
            g_CurrentEntity->ext.equipItemDrop.gravity;
        g_CurrentEntity->velocityX =
            g_CurrentEntity->ext.equipItemDrop.fallSpeed;
        if (g_CurrentEntity->velocityX == FIX(1) ||
            g_CurrentEntity->velocityX == FIX(-1)) {
            g_CurrentEntity->ext.equipItemDrop.gravity =
                -g_CurrentEntity->ext.equipItemDrop.gravity;
        }
    }

    if (g_CurrentEntity->velocityY < FIX(0.25)) {
        g_CurrentEntity->velocityY += FIX(0.125);
    }
}


static void PrizeDropFall2(u16 arg0) {
    Collider collider;

    if (g_CurrentEntity->velocityX < 0) {
        g_api.CheckCollision(g_CurrentEntity->posX.i.hi,
                             g_CurrentEntity->posY.i.hi - 7, &collider, 0);
        if (collider.effects & EFFECT_NOTHROUGH) {
            g_CurrentEntity->velocityY = 0;
        }
    }

    g_api.CheckCollision(g_CurrentEntity->posX.i.hi,
                         g_CurrentEntity->posY.i.hi + 7, &collider, 0);

    if (arg0) {
        if (!(collider.effects & EFFECT_NOTHROUGH)) {
            MoveEntity();
            FallEntity();
            return;
        }

        g_CurrentEntity->velocityX = 0;
        g_CurrentEntity->velocityY = 0;

        if (collider.effects & EFFECT_QUICKSAND) {
            g_CurrentEntity->posY.val += FIX(0.125);
            return;
        }

        g_CurrentEntity->posY.i.hi += collider.unk18;
        return;
    }

    if (!(collider.effects & EFFECT_NOTHROUGH)) {
        MoveEntity();
        PrizeDropFall();
    }
}


extern s8 D_us_80181464[];

void CollectHeart(u16 heartIdx) {
    g_api.PlaySfx(SFX_HEART_PICKUP);
    g_Status.hearts += D_us_80181464[heartIdx];

    if (g_Status.hearts > g_Status.heartsMax) {
        g_Status.hearts = g_Status.heartsMax;
    }

    DestroyEntity(g_CurrentEntity);
}


extern u32 D_us_801813D4[];
extern const char* g_goldCollectTexts[];

void CollectGold(u16 goldSize) {
    g_api.PlaySfx(SFX_GOLD_PICKUP);
    goldSize -= 2;
    g_Status.gold += D_us_801813D4[goldSize];
    if (g_Status.gold > MAX_GOLD) {
        g_Status.gold = MAX_GOLD;
    }
    if (g_unkGraphicsStruct.BottomCornerTextTimer) {
        g_api.FreePrimitives(g_unkGraphicsStruct.BottomCornerTextPrims);
        g_unkGraphicsStruct.BottomCornerTextTimer = 0;
    }

    BottomCornerText(g_goldCollectTexts[goldSize], true);
    DestroyEntity(g_CurrentEntity);
}


extern u16 D_us_80181368[];
extern u16 D_us_80181398[];

void CollectSubweapon(u16 subWeaponIdx) {
    Entity* player = &PLAYER;
    u16 subWeapon;

    g_api.PlaySfx(SFX_ITEM_PICKUP);
    subWeapon = g_Status.subWeapon;
    g_Status.subWeapon = D_us_80181368[subWeaponIdx - 14];

    if (subWeapon == g_Status.subWeapon) {
        subWeapon = 1;
        g_CurrentEntity->unk6D[0] = 0x10;
    } else {
        subWeapon = D_us_80181398[subWeapon];
        g_CurrentEntity->unk6D[0] = 0x60;
    }

    if (subWeapon) {
        g_CurrentEntity->params = subWeapon;
        g_CurrentEntity->posY.i.hi = player->posY.i.hi + 12;
        SetStep(7);
        g_CurrentEntity->velocityY = FIX(-2.5);
        g_CurrentEntity->animCurFrame = 0;
        g_CurrentEntity->ext.equipItemDrop.sparkleTimer = 5;
        if (player->facingLeft ^ 1) {
            g_CurrentEntity->velocityX = FIX(-2);
        } else {
            g_CurrentEntity->velocityX = FIX(2);
        }
    } else {
        DestroyEntity(g_CurrentEntity);
    }
}


void CollectHeartVessel(void) {
    if (g_PlayableCharacter != PLAYER_ALUCARD) {
        g_api.PlaySfx(SFX_HEART_PICKUP);
        g_Status.hearts += HEART_VESSEL_RICHTER;

        if (g_Status.hearts > g_Status.heartsMax) {
            g_Status.hearts = g_Status.heartsMax;
        }
    } else {
        g_api.PlaySfx(SFX_HEART_PICKUP);
        g_api.func_800FE044(HEART_VESSEL_INCREASE, 0x4000);
    }
    DestroyEntity(g_CurrentEntity);
}


void CollectLifeVessel(void) {
    g_api.PlaySfx(SFX_HEART_PICKUP);
    g_api.func_800FE044(LIFE_VESSEL_INCREASE, 0x8000);
    DestroyEntity(g_CurrentEntity);
}


void CollectDummy(u16 id) {
    DestroyEntity(g_CurrentEntity);
}


INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A072C);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A0734);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A073C);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A0744);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A074C);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A0754);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A075C);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A0764);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A076C);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A0774);

INCLUDE_ASM("st/rlib/nonmatchings/e_collect", EntityPrizeDrop);

extern EInit g_EInitParticle;
extern s32 D_us_80181468[];
extern AnimateEntityFrame* D_us_80181514[];

void EntityExplosion(Entity* entity) {
    if (!entity->step) {
        InitializeEntity(g_EInitParticle);
        entity->animSet = ANIMSET_DRA(2);
        entity->pose = 0;
        entity->poseTimer = 0;
        entity->blendMode = BLEND_TRANSP | BLEND_ADD;
        if (entity->params & 0xF0) {
            entity->palette = PAL_FLAG(PAL_UNK_195);
            entity->blendMode = BLEND_TRANSP;
        }

        if (entity->params & 0xFF00) {
            entity->zPriority = (entity->params & 0xFF00) >> 8;
        }
        entity->params &= 15;
        entity->velocityY = D_us_80181468[entity->params];
    } else {
        entity->posY.val += entity->velocityY;

        if (!AnimateEntity(D_us_80181514[entity->params], entity)) {
            DestroyEntity(entity);
        }
    }
}


static void BlinkItem(Entity* self, u16 timer) {
    Primitive* prim;
    prim = &g_PrimBuf[self->primIndex];

    prim->x0 = prim->x2 = self->posX.i.hi - 7;
    prim->x1 = prim->x3 = prim->x0 + 14;

    prim->y0 = prim->y1 = self->posY.i.hi - 7;
    prim->y2 = prim->y3 = prim->y0 + 14;

    if (timer & 2) {
        PCOL(prim) = 0xFF;
    } else {
        PCOL(prim) = 0x80;
    }
}


INCLUDE_ASM("st/rlib/nonmatchings/e_collect", EntityEquipItemDrop);

INCLUDE_ASM("st/rlib/nonmatchings/e_collect", BlitChar);

INCLUDE_RODATA("st/rlib/nonmatchings/e_collect", D_us_801A07B4);

INCLUDE_ASM("st/rlib/nonmatchings/e_collect", EntityRelicOrb);

extern u16 D_us_80180BC8[];

void EntityPrizeDrop(Entity* self);
void EntityEquipItemDrop(Entity* self);

void EntityHeartDrop(Entity* self) {
    u16 index;
    u8 value;
    PfnEntityUpdate update;

    if (!self->step) {
        index = self->ext.heartDrop.unkB4 = self->params + 0x1D8;
        value = g_CastleFlags[(index >> 3) + CASTLE_COLLECTIBLES_100] >>
                (index & 7);
        if (value & 1) {
            DestroyEntity(self);
            return;
        }

        index -= 0x1D8;
        index = D_us_80180BC8[index];
        if (index < 128) {
            self->unkB8 = (Entity*)EntityPrizeDrop;
        } else {
            self->unkB8 = (Entity*)EntityEquipItemDrop;
            index -= 128;
        }
        self->params = index + 0x8000;
    } else {
        index = self->ext.heartDrop.unkB4;
        if (self->step < 5) {
            if (self->hitFlags) {
                g_CastleFlags[(index >> 3) + CASTLE_COLLECTIBLES_100] |=
                    1 << (index & 7);
                self->step = 5;
            }
        }
    }
    update = (PfnEntityUpdate)self->unkB8;
    update(self);
}


INCLUDE_ASM("st/rlib/nonmatchings/e_collect", EntityMessageBox);
