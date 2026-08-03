#include "common.h"

extern void DestroyEntity(Entity* entity);
extern void InitializeEntity(u16 arg0[]);

// Set nonzero elsewhere (func_us_801A4E40) to force func_us_801A66B0 to reset the
// entity it's called on the next time it runs.
extern s32 D_us_801B5538;

extern u8* D_us_801B557C;
extern s16 D_us_801B557A;
extern s16 D_us_801B5578;

// Initial quad-chain layout tables consumed by func_us_801A66B0's step-0 case,
// selected by its `side` argument (0 or 1).
extern s16 D_us_80180CF0[8]; // x,y pairs x4, side 0
extern s16 D_us_80180D00[8]; // x,y pairs x4, side 1
extern u8 D_us_80180D10[16]; // u0/v0/u1/v1 bytes x4, side 0
extern u8 D_us_80180D20[16]; // u0/v0/u1/v1 bytes x4, side 1
extern u16 D_us_80180D30[]; // clut lookup, indexed by side * 4 + i
extern u16 D_us_80180478[]; // InitializeEntity() param table

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_8019FD30);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_8019FE84);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A0184);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A037C);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A0490);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1010);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A10DC);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1260);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1404);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A159C);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1884);

void func_us_801A19CC(u8 step) {
    g_CurrentEntity->poseTimer = 0;
    g_CurrentEntity->pose = 0;
    // ext+0x1 (0x7D): unidentified per-entity-type flag byte, cleared on step change
    *((u8*)&g_CurrentEntity->ext + 1) = 0;
    g_CurrentEntity->step = step;
    g_CurrentEntity->step_s = 0;
}

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A19FC);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1BA0);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A1C14);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A3B88);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A3E78);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A3FD4);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A425C);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4430);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4494);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A459C);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A45F0);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A47B4);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4810);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A48C8);

void func_us_801A4A4C(u8* arg0) {
    D_us_801B557C = arg0 + 0x100000;
    D_us_801B557A = 0;
    D_us_801B5578 = 1;
}

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4A78);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4D3C);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A4E40);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A5F28);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A5F88);

INCLUDE_ASM("boss/bo5/nonmatchings/1FD30", func_us_801A62B4);

// Nudges all 4 corners of a wing/tail-trail quad by `delta`, along its two
// "long" axes (x2/x0 pair and x3/x1 pair; y1/y0 pair and y3/y2 pair).
void func_us_801A666C(Primitive* prim, s32 delta) {
    prim->x2 -= delta;
    prim->x0 = prim->x2;
    prim->x3 += delta;
    prim->x1 = prim->x3;
    prim->y1 -= delta;
    prim->y0 = prim->y1;
    prim->y3 += delta;
    prim->y2 = prim->y3;
}

// Manages a 4-quad primitive chain (a wing/tail trail effect) through a
// 10-step state machine driven by self->step:
//   step 0: allocate + initialize the 4-quad chain (side selects the
//           left/right layout tables), then wait.
//   steps 1-4: grow the trail outward one quad at a time (calling
//           func_us_801A666C with a positive delta on progressively deeper
//           quads in the chain), advancing self->step every 8 calls.
//   step 5: hold for up to 0xC0 calls before continuing.
//   steps 6-8: retract the trail back in (negative delta), advancing every
//           8 calls.
//   step 9: final retraction; once its own 8-call timer expires, destroys
//           the entity.
void func_us_801A66B0(Entity* self, s32 side) {
    s32 primIndex;
    Primitive* p;

    if (D_us_801B5538) {
        DestroyEntity(self);
    }

    switch (self->step) {
        case 0: {
            s32 i;
            u16* posTable;
            u8* uvTable;
            u16 dir;

            primIndex = g_api_AllocPrimitives(PRIM_GT4, 4);
            if (primIndex == -1) {
                return;
            }

            InitializeEntity(D_us_80180478);

            p = &g_PrimBuf[primIndex];
            self->animSet = -0x8000;
            self->primIndex = primIndex;
            self->flags |= FLAG_HAS_PRIMS;

            dir = (u16)side;
            if (dir != 0) {
                posTable = (u16*)D_us_80180D00;
                uvTable = D_us_80180D20;
            } else {
                posTable = (u16*)D_us_80180CF0;
                uvTable = D_us_80180D10;
            }

            for (i = 0; i < 4; i++) {
                p->tpage = 0x14;
                p->clut = D_us_80180D30[dir * 4 + i];

                p->x3 = p->x2 = p->x1 = p->x0 = *posTable++;
                p->y3 = p->y2 = p->y1 = p->y0 = *posTable++;

                p->u2 = p->u0 = *uvTable++;
                p->v1 = p->v0 = *uvTable++;
                p->u3 = p->u1 = *uvTable++;
                p->v3 = p->v2 = *uvTable++;

                p->drawMode = 0;
                p->priority = self->zPriority;
                p = p->next;
            }

            self->ext.utimer.t = 0;
            return;
        }

        case 1:
            p = &g_PrimBuf[self->primIndex];
            if (self->ext.utimer.t & 1) {
                func_us_801A666C(p, 1);
            }
            goto tail8;

        case 2:
            p = g_PrimBuf[self->primIndex].next;
            if (self->ext.utimer.t & 1) {
                func_us_801A666C(p, 1);
            }
            goto tail8;

        case 3:
            p = g_PrimBuf[self->primIndex].next->next;
            func_us_801A666C(p, 1);
            goto tail8;

        case 4:
            p = g_PrimBuf[self->primIndex].next->next->next;
            func_us_801A666C(p, 2);
            if (self->ext.utimer.t & 1) {
                p->y1 -= 1;
                p->y0 = p->y1;
                p->y3 += 1;
                p->y2 = p->y3;
            }
            goto tail8;

        case 5:
            self->ext.utimer.t++;
            if (self->ext.utimer.t < 0xC0) {
                return;
            }
            goto advanceStep;

        case 6:
            p = g_PrimBuf[self->primIndex].next->next->next;
            func_us_801A666C(p, -2);
            if (self->ext.utimer.t & 1) {
                p->y1 += 1;
                p->y0 = p->y1;
                p->y3 -= 1;
                p->y2 = p->y3;
            }
            goto tail8;

        case 7:
            p = g_PrimBuf[self->primIndex].next->next;
            func_us_801A666C(p, -1);
            goto tail8;

        case 8:
            p = g_PrimBuf[self->primIndex].next;
            if (self->ext.utimer.t & 1) {
                func_us_801A666C(p, -1);
            }
            goto tail8;

        case 9:
            p = &g_PrimBuf[self->primIndex];
            if (self->ext.utimer.t & 1) {
                func_us_801A666C(p, -1);
            }
            self->ext.utimer.t++;
            if (self->ext.utimer.t < 8) {
                return;
            }
            DestroyEntity(self);
            return;
    }
    return;

tail8:
    self->ext.utimer.t++;
    if (self->ext.utimer.t < 8) {
        return;
    }

advanceStep:
    self->ext.utimer.t = 0;
    self->step++;
}

void func_us_801A6B74(Entity* self) {
    func_us_801A66B0(self, 0);
}

void func_us_801A6B94(Entity* self) {
    func_us_801A66B0(self, 1);
}
