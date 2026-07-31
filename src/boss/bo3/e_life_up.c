// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo3.h"

extern void func_us_801AC7E8(Entity* self);
extern void MoveEntity(void);
extern void UnkPolyFunc2(void** prim);

// Quad corner offsets (GTE angle units) for the 4 vertices of each ring tile,
// fed to RotTransPers4 alongside the ring's own rotation matrix. Real bytes
// from asm/us/boss/bo3/data/0.data.s (D_us_80181C40-D_us_80181C5C).
static SVECTOR D_us_80181C40[4] = {
    {-8, -8, 0, 0},
    {-8, 8, 0, 0},
    {8, -8, 0, 0},
    {8, 8, 0, 0},
};

// Prize/next-state table indexed by self->params (step 6, prize-drop
// dispatch). Real bytes from D_us_80181C60 (22 x u16).
static u16 D_us_80181C60[] = {
    0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
};

// Ring's own local rotation baseline (always {0,0,0} in the retail data).
static SVECTOR D_us_80181C8C = {0, 0, 0, 0};

// EntityLifeUpSpawn: the "Life Max Up" pickup's reward animation.
//
// step 0: spawns a central icon primitive plus a ring of 8 (slot) x 3 (row)
//   x 8 (col) quad *pairs* -- for each ring position, a visible quad prim
//   (screen-space corners filled in every frame by RotTransPers4) immediately
//   followed by a paired "state" prim whose otherwise-unused coordinate
//   fields (x1/y1/u0 packed as one s32, x2/y2, x3/y3/tpage) are reused as a
//   per-cell rotation-angle/scale/countdown scratch struct -- not real
//   texture data. self->ext.lifeUpSpawn.prim1 anchors the first visible quad,
//   prim2 is the central icon.
// step 1/2: step 1 advances how many ring cells are "active" this frame
//   (unk88, capped at 8) before falling into step 2's per-cell GTE update:
//   each active cell's paired state-prim angle accumulator is advanced; once
//   it crosses a threshold the cell's ring quad is hidden (drawMode = 8, and
//   the next 0x30 raw primitive slots are hidden the same way -- a "hide the
//   rest of the chain in one pass" cleanup, confirmed via the real
//   disassembly using a flat 0x34-byte pointer stride here, unlike the
//   paired stride used everywhere else in this function). Otherwise the cell
//   is rotated/scaled/perspective-projected via RotMatrix/TransMatrix/
//   RotTransPers/RotTransPers4 and its angle/scale state advanced for next
//   frame. Falls into the shared step 3/4 tail to resize the central icon.
// step 3/4: step 4 does simple gravity + floor collision on the whole
//   entity, snapping to the collided tile's top edge and shrinking a radius
//   counter (unk84) each bounce; step 3 shrinks the same counter on its own
//   once step 4 stops running, then both re-derive the central icon's prim
//   corners from that radius.
// step 5/6: frees the primitive block, then dispatches into the shared
//   prize-drop state machine (func_us_801AB53C for Richter, func_us_801AC7E8
//   / func_us_801ABF50 otherwise -- same family as 2AFCC.c), looking up the
//   next prize id in D_us_80181C60.
//
// Confidence: high on control flow and every Entity/Primitive field (both
// structs are fully named in game.h/primitive.h, and every offset here was
// checked against them); the paired-primitive "state" reuse above is inferred
// from behavior, not cross-checked against another already-matched function.
void EntityLifeUpSpawn(Entity* self)
{
    Collider collider;
    SVECTOR ringAngle;
    VECTOR transVec;
    MATRIX mtx;
    SVECTOR centerAngle;
    long screenXY;
    long depthP;
    long clipFlag;
    long depthReturn;
    s32 shrunkCount;
    s32 activeCount;
    Primitive* centerPrim;
    Primitive* ringHead;
    Primitive* quadPrim;
    Primitive* cellPrim;
    Primitive* hidePtr;
    s32 col, row, slot;
    s32 i;
    s32 primIdx;
    s32 packedAngle, angleHi;
    s16 radius;

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_80180564);
        self->animSet = 2;
        self->animCurFrame = 0;

        primIdx = g_api_func_800EDB58(4, 0x181);
        if (primIdx == -1) {
            self->step = 6;
            return;
        }

        centerPrim = &g_PrimBuf[primIdx];
        self->primIndex = primIdx;
        self->ext.lifeUpSpawn.prim2 = centerPrim;
        self->flags |= FLAG_HAS_PRIMS;

        centerPrim->tpage = 0x1A;
        centerPrim->clut = 0x19F;
        centerPrim->u3 = 0x3F;
        centerPrim->u1 = 0x3F;
        centerPrim->v1 = 0xC0;
        centerPrim->v0 = 0xC0;
        centerPrim->u2 = 0;
        centerPrim->u0 = 0;
        centerPrim->v3 = 0xFF;
        centerPrim->v2 = 0xFF;
        // Retail really does use posX.i.hi for both X and Y corners here
        // (matches real disassembly) -- RotTransPers4 fills in real,
        // Y-separated coordinates once the ring starts animating below.
        centerPrim->x3 = self->posX.i.hi;
        centerPrim->x2 = self->posX.i.hi;
        centerPrim->x1 = self->posX.i.hi;
        centerPrim->x0 = self->posX.i.hi;
        centerPrim->priority = 0xC0;
        centerPrim->drawMode = 0x33;
        centerPrim->y3 = self->posX.i.hi;
        centerPrim->y2 = self->posX.i.hi;
        centerPrim->y1 = self->posX.i.hi;
        centerPrim->y0 = self->posX.i.hi;

        ringHead = centerPrim->next;
        self->ext.lifeUpSpawn.prim1 = ringHead;

        quadPrim = ringHead;
        // 8 (ring) x 3 (row) x 8 (slot) walk over prim pairs, reproduced
        // goto-for-goto against the real control flow (not restructured into
        // nested for-loops) since the row counter resets per ring but col
        // keeps accumulating within a ring -- easy to get subtly wrong by
        // "cleaning up" into nested loops.
        i = 0;   // ring counter (real var_s0), 8 iterations
        row = 0; // real var_s6
        do {
            col = 0;
        loop_row:
            slot = 0;
        loop_col:
            UnkPolyFunc2((void**)quadPrim);
            cellPrim = quadPrim->next;
            quadPrim->tpage = 0x1A;
            quadPrim->clut = 0x194;
            quadPrim->u3 = 0x10;
            quadPrim->u1 = 0x10;
            quadPrim->v1 = 0x50;
            quadPrim->v0 = 0x50;
            quadPrim->v3 = 0x60;
            quadPrim->v2 = 0x60;
            quadPrim->u2 = 0;
            quadPrim->u0 = 0;
            quadPrim->r3 = 0x20;
            quadPrim->r2 = 0x20;
            quadPrim->r1 = 0x20;
            quadPrim->r0 = 0x20;
            quadPrim->g3 = 0;
            quadPrim->g2 = 0;
            quadPrim->g1 = 0;
            quadPrim->g0 = 0;
            quadPrim->b3 = 0;
            quadPrim->b2 = 0;
            quadPrim->b1 = 0;
            quadPrim->b0 = 0;
            cellPrim->x2 = 0x1000;
            cellPrim->y2 = 0x1000;
            cellPrim->x3 = (s16)(slot << 9);
            cellPrim->x2 = 0;
            cellPrim->y3 = (s16)col;
            *(s32*)&cellPrim->u0 = 0xFFFB0000;
            cellPrim->x1 = 0x80;
            cellPrim->y1 = 0;
            quadPrim->priority = 0xC0;
            quadPrim->drawMode = 0x73;
            quadPrim = cellPrim;
            slot += 1;
            quadPrim->drawMode &= 0xFFFD;
            quadPrim = quadPrim->next;
            if (slot < 8) {
                goto loop_col;
            }
            row += 1;
            col += 0x540;
            if (row < 3) {
                goto loop_row;
            }
            i += 1;
            row = 0;
        } while (i < 8);

        g_api_PlaySfx(0x7D2);
        return;

    case 1:
        if (self->ext.lifeUpSpawn.unk86 == 0) {
            self->ext.lifeUpSpawn.unk86 = 2;
            self->ext.lifeUpSpawn.unk88++;
        }
        self->ext.lifeUpSpawn.unk86--;
        if (self->ext.lifeUpSpawn.unk88 >= 8) {
            self->ext.lifeUpSpawn.unk88 = 7;
            self->step++;
        }
        // fallthrough
    case 2:
        SetGeomScreen(0x200);
        shrunkCount = 0;
        SetGeomOffset(self->posX.i.hi, self->posY.i.hi);
        quadPrim = self->ext.lifeUpSpawn.prim1;
        activeCount = 0;
        if (self->ext.lifeUpSpawn.unk88 + 1 > 0) {
            do {
                cellPrim = quadPrim->next;
                // offset 0xC spans u0/v0/clut; retail reuses it as one
                // packed s32 rotation-angle accumulator here, not texture
                // coords.
                packedAngle = cellPrim->y1 + (cellPrim->x1 << 0x10) + *(s32*)&cellPrim->u0;
                cellPrim->y1 = (s16)packedAngle;
                angleHi = packedAngle >> 0x10;
                cellPrim->x1 = (s16)angleHi;
                if (*(s32*)&cellPrim->u0 < -0x4000) {
                    *(s32*)&cellPrim->u0 = *(s32*)&cellPrim->u0 + 0x3800;
                }

                if (cellPrim->x1 < 8) {
                    shrunkCount += 1;
                    self->ext.lifeUpSpawn.unk84 += 4;
                    hidePtr = quadPrim;
                    for (i = 0x2F; i >= 0; i--) {
                        hidePtr->drawMode = 8;
                        hidePtr = (Primitive*)((u8*)hidePtr + 0x34);
                    }
                } else {
                    ringAngle.vx = cellPrim->x3;
                    ringAngle.vy = cellPrim->y3;
                    ringAngle.vz = cellPrim->tpage;
                    RotMatrix(&ringAngle, &mtx);
                    transVec.vx = 0;
                    transVec.vy = 0;
                    transVec.vz = 0x200;
                    TransMatrix(&mtx, &transVec);
                    SetRotMatrix(&mtx);
                    SetTransMatrix(&mtx);
                    centerAngle.vx = (s16)angleHi;
                    centerAngle.vy = 0;
                    centerAngle.vz = 0;
                    depthReturn = RotTransPers(&centerAngle, &screenXY, &depthP, &clipFlag);
                    RotMatrix(&D_us_80181C8C, &mtx);
                    transVec.vx = (s16)screenXY - self->posX.i.hi;
                    transVec.vz = (depthReturn << 0x10) >> 0xE;
                    transVec.vy = (s16)(screenXY >> 0x10) - self->posY.i.hi;
                    TransMatrix(&mtx, &transVec);
                    transVec.vx = cellPrim->x2;
                    transVec.vz = 0x1000;
                    transVec.vy = cellPrim->y2;
                    ScaleMatrix(&mtx, &transVec);
                    SetRotMatrix(&mtx);
                    SetTransMatrix(&mtx);
                    RotTransPers4(&D_us_80181C40[0], &D_us_80181C40[1], &D_us_80181C40[2], &D_us_80181C40[3],
                        (long*)&quadPrim->x0, (long*)&quadPrim->x1, (long*)&quadPrim->x2, (long*)&quadPrim->x3,
                        &depthP, &clipFlag);
                    cellPrim->y2 = (u16)(cellPrim->y2 - 0x10);
                    cellPrim->x2 = cellPrim->y2;
                    cellPrim->tpage = (u16)(cellPrim->tpage + 8);
                    cellPrim->x3 = (u16)(cellPrim->x3 + 0x10);
                    cellPrim->y3 = (u16)(cellPrim->y3 + 0x20);
                    quadPrim = cellPrim->next;
                }
                activeCount += 1;
            } while (activeCount < self->ext.lifeUpSpawn.unk88 + 1);
        }

        if (shrunkCount == 8) {
            self->step++;
        }

        radius = (s16)self->ext.lifeUpSpawn.unk84;
        goto apply_icon_radius;

    case 4:
        MoveEntity();
        self->velocityY += 0x2000;
        g_api_CheckCollision(self->posX.i.hi, (s16)(self->posY.i.hi + 4), &collider, 0);
        if (collider.effects & EFFECT_SOLID) {
            self->velocityY = 0;
            self->ext.lifeUpSpawn.unk84 -= 1;
            self->posY.i.hi += (s16)collider.unk18;
            if (self->ext.lifeUpSpawn.unk84 == 0) {
                self->step = 5;
                return;
            }
        }
        // fallthrough
    case 3:
        if ((s16)self->ext.lifeUpSpawn.unk84 > 0) {
            self->ext.lifeUpSpawn.unk84 -= 0x20;
        } else {
            self->ext.lifeUpSpawn.unk84 = 0x10;
            self->step = 5;
        }
        radius = (s16)self->ext.lifeUpSpawn.unk84;
    apply_icon_radius:
        if (radius >= 0xE1) {
            radius = 0xE0;
        }
        centerPrim = self->ext.lifeUpSpawn.prim2;
        centerPrim->x2 = self->posX.i.hi - radius;
        centerPrim->x0 = centerPrim->x2;
        centerPrim->x3 = self->posX.i.hi + radius;
        centerPrim->x1 = centerPrim->x3;
        centerPrim->y2 = self->posY.i.hi - radius;
        centerPrim->y0 = centerPrim->y2;
        centerPrim->y3 = self->posY.i.hi + radius;
        centerPrim->y1 = centerPrim->y3;
        return;

    case 5:
        g_api_FreePrimitives(self->primIndex);
        self->step++;
        // fallthrough
    case 6:
        if (self->params >= 0x11) {
            if (g_PlayableCharacter != 0) {
                self->params = 0x17;
                self->entityId = 3;
                self->pfnUpdate = func_us_801AB53C;
                self->poseTimer = 0;
                self->pose = 0;
                goto finish_prize_drop;
            }
            self->entityId = 0xB;
            self->pfnUpdate = func_us_801AC7E8;
            self->poseTimer = 0;
            self->pose = 0;
            self->unk6D[0] = 0x10;
            self->step = 0;
            self->params = D_us_80181C60[self->params];
            return;
        }
        {
            u16 nextParams = D_us_80181C60[self->params];
            u16 low12 = nextParams & 0xFFF;
            self->params = nextParams;
            if (low12 < 0x80) {
                self->entityId = 3;
                self->pfnUpdate = func_us_801AB53C;
                self->poseTimer = 0;
                self->pose = 0;
            } else {
                low12 -= 0x80;
                self->entityId = 0xA;
                self->pfnUpdate = func_us_801ABF50;
            }
            self->params = low12;
        }
    finish_prize_drop:
        self->unk6D[0] = 0x10;
        self->step = 0;
        self->params |= 0x8000;
        return;

    default:
        return;
    }
}
