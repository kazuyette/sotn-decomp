// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo0.h"

extern s32 D_us_80181190; // shared "ready" flag, also checked in step 4
extern u16 D_us_80180738[]; // EInit-style table for InitializeEntity
extern u16 D_us_801813A4[]; // tile pattern table (step 0 / step 5, params != 0)
extern u16 D_us_80181374[]; // tile pattern table (step 0 / step 5, params == 0)
extern u16 D_us_801813BC[]; // tile pattern table (step 3, params != 0)
extern u16 D_us_8018138C[]; // tile pattern table (step 3, params == 0)
// FntPrint format string ("charal %x\n"), debug step 0xFF. Defined here (not just
// extern-declared) because this rodata lived only inside func_us_801BAB18.s's own
// .rodata block; with that INCLUDE_ASM gone, nothing else pulls the symbol in at link time.
const char D_us_801A9678[] = "charal %x\n";
extern u16 g_pads_1_pressed;
extern s16 PLAYER_posX_i_hi;
extern s32 GetPlayerCollisionWith(Entity* entity, s32 width, s32 height, s32 unk);
extern void MoveEntity(void);
extern void PlaySfxPositional(s16 sfxId);
extern void CreateEntityFromEntity(u16 entityId, Entity* source, Entity* entity);

void func_us_801BAB18(Entity* self) {
    u16 step = self->step;
    Entity* part = (Entity*)((u8*)self + 0xBC); // embedded sub-entity

    if (step == 3) goto L801BAD9C;
    if (step >= 4) goto L801BAB64;
    if (step == 1) goto L801BAC64;
    if (step >= 2) goto L801BACCC;
    if (step == 0) goto L801BAB90;
    goto L801BAFFC;

L801BAB64:
    if (step == 5) goto L801BAE4C;
    if (step < 5) goto L801BAE30;
    if (step == 6) goto L801BAF04;
    if (step == 0xFF) goto L801BAF58;
    goto L801BAFFC;

L801BAB90: {
    u16* tbl;
    s32 tileIndex;
    s32 i;

    if (D_us_80181190 != 0) goto L801BAF48;

    InitializeEntity(D_us_80180738);

    if (self->params != 0) {
        self->zPriority = 0x69;
        self->posX.i.hi = 0x218 - g_Tilemap.scrollX.i.hi;
        if (self->params != 0) {
            tileIndex = 0xDD;
            tbl = D_us_801813A4;
            goto L801BABFC;
        }
    }
    tileIndex = 0xC0;
    tbl = D_us_80181374;

L801BABFC:
    for (i = 0; i < 4; i++) {
        g_Tilemap.fg[tileIndex] = *tbl++;
        g_Tilemap.fg[tileIndex + 1] = *tbl++;
        g_Tilemap.fg[tileIndex + 2] = *tbl++;
        tileIndex += 0x20;
    }
    goto L801BAFFC;
}

L801BAC64:
    if (PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi < 0x1E8) {
        if (self->params != 0) {
            CreateEntityFromEntity(0x49, self, part);
            part->params = 0;
            part->posX.i.hi = -0x18 - g_Tilemap.scrollX.i.hi;
        }
        PlaySfxPositional(0x608);
    }
    goto L801BAEF0;

L801BACCC: {
    s32 posX;
    s32 dist;

    if (self->step_s == 0) {
        self->velocityX = (self->params != 0) ? -0x8000 : 0x8000;
        self->step_s++;
    }

    GetPlayerCollisionWith(self, 0x18, 0x20, 0x5);
    MoveEntity();

    if ((g_Timer & 0xF) == 0) {
        PlaySfxPositional(0x608);
    }

    dist = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
    if (self->params != 0) {
        if (dist >= 0x1E8) goto L801BAFFC;
        posX = 0x1E8;
    } else {
        if (dist < 0x19) goto L801BAFFC;
        posX = 0x18;
    }
    self->posX.i.hi = posX - g_Tilemap.scrollX.i.hi;
    self->step++;
    goto L801BAFFC;
}

L801BAD9C: {
    u16* tbl;
    s32 tileIndex;
    s32 i;

    if (self->params != 0) {
        tileIndex = 0xDD;
        tbl = D_us_801813BC;
    } else {
        tileIndex = 0xC0;
        tbl = D_us_8018138C;
    }
    for (i = 0; i < 4; i++) {
        g_Tilemap.fg[tileIndex] = *tbl++;
        g_Tilemap.fg[tileIndex + 1] = *tbl++;
        g_Tilemap.fg[tileIndex + 2] = *tbl++;
        tileIndex += 0x20;
    }
    goto L801BAEF0;
}

L801BAE30:
    if (D_us_80181190 == 0) goto L801BAFFC;
    goto L801BAEF0;

L801BAE4C: {
    u16* tbl;
    s32 tileIndex;
    s32 i;

    if (self->params != 0) {
        tileIndex = 0xDD;
        tbl = D_us_801813A4;
    } else {
        tileIndex = 0xC0;
        tbl = D_us_80181374;
    }
    for (i = 0; i < 4; i++) {
        g_Tilemap.fg[tileIndex] = *tbl++;
        g_Tilemap.fg[tileIndex + 1] = *tbl++;
        g_Tilemap.fg[tileIndex + 2] = *tbl++;
        tileIndex += 0x20;
    }
    self->velocityX = (self->params != 0) ? 0x8000 : -0x8000;
    goto L801BAEF0;
}

L801BAF04: {
    s32 dist;

    MoveEntity();
    dist = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
    if (self->params != 0) {
        if (dist >= 0x219) goto L801BAF48;
        goto L801BAFFC;
    } else {
        if (dist >= -0x18) goto L801BAFFC;
    }
L801BAF48:
    DestroyEntity(self);
    goto L801BAFFC;
}

L801BAF58:
    FntPrint(D_us_801A9678, self->animCurFrame);
    if (g_pads_1_pressed & 0x80) {
        if (self->params != 0) goto L801BAFFC;
        self->animCurFrame++;
        self->params |= 1;
    } else {
        self->params = 0;
    }
    if (g_pads_1_pressed & 0x20) {
        if (self->step_s != 0) goto L801BAFFC;
        self->animCurFrame--;
        self->step_s |= 1;
    } else {
        self->step_s = 0;
    }
    goto L801BAFFC;

L801BAEF0:
    self->step++;

L801BAFFC:
    return;
}
