// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo0.h"

extern void CreateEntityFromEntity(u16 entityId, Entity* source, Entity* entity);
extern EInit g_EInitEnvironment;
extern Entity g_Entities_224[];
extern void UnkPolyFunc2(Primitive* prim);
extern void UnkPolyFunc0(Primitive* prim);
extern void UnkPrimHelper(Primitive* prim);
extern Primitive* FindFirstUnkPrim(Primitive* prim);
extern Primitive* FindFirstUnkPrim2(Primitive* prim, u8 index);
// FntPrint format string ("timer %x\n"). Defined here (not just extern-declared) because
// this rodata lived only inside func_us_801ABFE0.s's own .rodata block; with that
// INCLUDE_ASM gone, nothing else pulls the symbol in at link time.
const char D_us_801A92CC[] = "timer %x\n";
extern s32 D_us_80180A48[]; // per-r3-state child velocityX table (indexed r3*2)
extern s32 D_us_80180A4C[]; // per-r3-state child velocityY table (indexed r3*2)
extern u16 D_us_80180AC8[]; // per-r3-state tpage(brightness) increment table
extern u16 D_us_80180AEC[]; // floor tile indices restored when the big torch/pillar breaks
extern u16 D_us_80180B00[]; // floor tile values restored when the big torch/pillar breaks
extern u16 D_us_80180B14[]; // floor tile indices restored by the spinning-torch variant
extern u16 D_us_80180B24[]; // floor tile values (spinning-torch variant, initial/unbroken)
extern u16 D_us_80180B34[]; // floor tile values (spinning-torch variant, broken)

#define ENTITY_EXT80(e) (*(s16*)((u8*)(e) + 0x80))     // countdown timer (ticks/frames)
#define ENTITY_EXT7C(e) (*(Primitive**)((u8*)(e) + 0x7C)) // head of this entity's debris prim chain
#define ENTITY_EXT84(e) (*(Primitive**)((u8*)(e) + 0x84)) // tail prim of the spin-torch's ring chain

extern void CreateEntityFromCurrentEntity(s32 type, Entity* newEntity);

// func_us_801AB9EC: per-debris-shard update, called once per frame per active shard
// from the various torch/pillar entities below. `self` is the shard's own "draw"
// Primitive; `self->next` ("child") is a second Primitive reused as physics scratch
// (chain-of-2 idiom: fields at 0xC/0x10 hold packed 32-bit velocityX/velocityY via
// LOW(), matching func_us_801AC73C; the packed {r2,g2} halfword at 0x1C is a spin
// phase counter). child->u2 (0x24) is the shard's own state machine (0-4), dispatched
// via jtbl_us_801A92B8; child->r3 (0x28) is the shard "type": 0-7 (big torch),
// 8-11 (small pot), 12-15 (spin torch).
void func_us_801AB9EC(Primitive* self) {
    // 5-entry jump table matching the original's jtbl_us_801A92B8 (a plain switch on 5
    // cases doesn't reach GCC's jump-table lowering threshold here, so force it explicitly
    // via GNU C's labels-as-values, same as the hand-authored original .rodata table).
    static void* const jtbl_us_801A92B8[] = {
        &&L801ABA44, &&L801ABB80, &&L801ABCE0, &&L801ABD84, &&L801ABE58
    };
    u8 disp = self->next->u2;
    s32 tailVal;
    Primitive* child; // only used to bridge state 2 -> the shared L801ABB74 tail
    Collider collider; // shared stack slot between state 1 and state 4 (mutually exclusive)

    if (disp >= 5) goto L801ABFB0;
    goto *jtbl_us_801A92B8[disp];

L801ABA44:
    self->tpage = 0xF;
    self->clut = 0x33;
    // mirrored U-coordinates, alternating per child->r3 parity/range
    if (self->next->r3 & 1) {
        self->u0 = self->u2 = 0xB8;
        self->u1 = self->u3 = 0xA8;
    } else {
        self->u0 = self->u2 = 0xA8;
        self->u1 = self->u3 = 0xB8;
    }
    if ((self->next->r3 & 3) < 2) {
        self->v0 = self->v1 = 0xC8;
        self->v2 = self->v3 = 0xD8;
    } else {
        self->v0 = self->v1 = 0xD8;
        self->v2 = self->v3 = 0xC8;
    }
    self->priority = 0x68;
    self->drawMode = 2;
    LOW(self->next->u0) = D_us_80180A48[self->next->r3 * 2];
    LOW(self->next->r1) = D_us_80180A4C[self->next->r3 * 2];
    {
        Primitive* c = self->next;
        *(u16*)&c->b2 = 0x10; // {b2,p2} packed
        *(u16*)&c->r2 = 0x10; // {r2,g2} packed
    }
    self->next->u2 = 1;
    child = self->next;
    if ((u8)(child->r3 - 8) >= 4) goto L801ABFB0;
    tailVal = 8;
    goto L801ABB74;

L801ABB80: { // state 1: falling/settling; on ground contact, spawn 3 impact fragments
    Primitive* child = self->next;
    s32 y0 = (u16)child->y0;
    s16 x1 = child->x1;
    u16 x1u = (u16)child->x1;
    s32 s0 = y0 + 4;
    s32 s4;
    s32 s2;
    s32 i;

    LOW(child->r1) += 0x800;
    if (child->r3 >= 0xC) {
        LOW(child->r1) += 0x1000;
    }
    child->tpage += D_us_80180AC8[child->r3];

    g_api_CheckCollision(x1, (s16)s0, &collider, 0);
    if (!(collider.effects & 1)) goto L801ABFB0;

    s2 = x1u;
    s4 = (u16)collider.unk18 + s0;
    for (i = 0; i < 3; i++, s2 += 4) {
        Primitive* prim = FindFirstUnkPrim2(ENTITY_EXT7C(g_CurrentEntity), 2);
        if (prim != NULL) {
            UnkPolyFunc2(prim);
            prim->next->u2 = 2;
            prim->next->y0 = s4 - 8;
            if (self->next->u2 < 0xC) {
                prim->next->x1 = s2;
            } else {
                prim->next->x1 = x1u;
                prim->next->u2 = 3;
            }
        }
    }
    UnkPolyFunc0(self);
    return;
}

L801ABCE0: { // state 2: spin-torch spawn, random velocity
    s32 r;
    self->tpage = 0xF;
    self->clut = 0x33;
    self->u0 = self->u2 = 0xB8;
    self->u1 = self->u3 = 0xC8;
    self->v0 = self->v1 = 0xC8;
    self->v2 = self->v3 = 0xD8;
    self->priority = 0x6A;
    self->drawMode = 2;

    r = Random();
    LOW(self->next->u0) = -(s32)((r & 7) * 5) << 11;

    r = Random();
    LOW(self->next->r1) = (s32)0xFFFE8000 - ((r & 7) << 13);

    r = Random();
    tailVal = ((r & 3) << 1) | 8;
    child = self->next;
    goto L801ABB74;
}

L801ABD84: { // state 3: small-pot spawn, random velocity (own tail, extra r3 set)
    s32 r;
    s32 v0;

    self->tpage = 0xF;
    self->clut = 0x33;
    self->u0 = self->u2 = 0xB8;
    self->u1 = self->u3 = 0xC8;
    self->v0 = self->v1 = 0xC8;
    self->v2 = self->v3 = 0xD8;
    self->priority = 0x6A;
    self->drawMode = 2;

    r = Random();
    LOW(self->next->u0) = (s32)0xFFFFE000 - ((r & 7) << 14);

    r = Random();
    LOW(self->next->r1) = (s32)0xFFFE8000 - ((r & 7) << 13);

    r = Random();
    v0 = ((r & 3) << 1) | 8;
    {
        Primitive* c = self->next;
        *(u16*)&c->b2 = (u16)v0;
        *(u16*)&c->r2 = (u16)v0;
    }
    {
        Primitive* c = self->next;
        v0 = (LOW(c->u0) > 0) ? 0x10 : 0x11;
        c->r3 = (u8)v0;
    }
    goto L801ABE48;
}

L801ABE58: { // state 4: settled; keep falling/spinning, respawn a fragment if it lands hard
    Primitive* child = self->next;
    s32 y0 = (u16)child->y0;
    s16 x1 = child->x1;
    u16 x1u = (u16)child->x1;
    s16 packed = *(s16*)&child->r2;
    s32 half = packed / 2;
    s32 s4;

    LOW(child->r1) += 0x2000;
    if (child->r3 == 0) {
        child->tpage -= 0x40;
    } else {
        child->tpage += D_us_80180AC8[child->r3];
    }

    s4 = y0 + half;
    g_api_CheckCollision(x1, (s16)s4, &collider, 0);
    if (!(collider.effects & 1)) goto L801ABFB0;

    self->next->y0 = (s16)((u16)self->next->y0 + (u16)collider.unk18);
    LOW(self->next->r1) = -LOW(self->next->r1) / 2;

    if (LOW(self->next->r1) < -0x3FFF) goto L801ABFB0;
    {
        Entity* newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (newEntity != NULL) {
            CreateEntityFromCurrentEntity(6, newEntity);
            newEntity->posX.i.hi = x1u;
            newEntity->posY.i.hi = (s16)s4;
            newEntity->params = 0x10;
        }
    }
    UnkPolyFunc0(self);
    goto L801ABFB0;
}

L801ABB74:
    *(u16*)&child->b2 = (u16)tailVal;
    *(u16*)&child->r2 = (u16)tailVal;

L801ABE48: {
    Primitive* child = self->next;
    child->u2 = 4;
}

L801ABFB0:
    UnkPrimHelper(self);
}

// func_us_801ABFE0: the big destructible wall torch/pillar with a floor gap underneath.
void func_us_801ABFE0(Entity* self) {
    s32 i;

    FntPrint(D_us_801A92CC, ENTITY_EXT80(self));

    switch (self->step) {
    case 0: {
        InitializeEntity(g_EInitEnvironment);
        self->animCurFrame = 0;
        if (g_CastleFlags[0x20] != 0) {
            for (i = 0; i < 10; i++) {
                g_Tilemap.fg[D_us_80180AEC[i]] = D_us_80180B00[i];
            }
            DestroyEntity(self);
            return;
        }
        self->zPriority = 0xA8;
        self->hitboxState = 2;
        self->hitPoints = 0x10;
        self->hitboxWidth = 0x10;
        self->hitboxHeight = 0x28;
        return;
    }

    case 1: {
        if (self->hitPoints < 0x10) self->animCurFrame = 0xC;
        if (self->hitPoints < 8) self->animCurFrame = 0xD;
        if (!(self->flags & 0x100)) return;
        self->hitboxState = 0;
        self->step++;
        return;
    }

    case 2: {
        s16 primIndex = (s16)g_api_AllocPrimitives(PRIM_GT4, 0x20);

        self->primIndex = primIndex;
        ENTITY_EXT7C(self) = (Primitive*)&g_PrimBuf[primIndex];
        if (primIndex == -1) {
            g_CastleFlags[0x20] |= 1;
            for (i = 0; i < 10; i++) {
                g_Tilemap.fg[D_us_80180AEC[i]] = D_us_80180B00[i];
            }
            DestroyEntity(self);
            return;
        }
        self->flags |= 0x800000;
        {
            Primitive* prim = &g_PrimBuf[primIndex];
            prim->drawMode = 8;
            for (prim = (Primitive*)prim->next; prim != NULL; prim = (Primitive*)prim->next) {
                prim->drawMode = 8;
            }
        }
        {
            Primitive* prim = ENTITY_EXT7C(self);
            for (i = 0; i < 2; i++) {
                s16 x0, x1, y;

                prim->tpage = 0xF;
                prim->clut = 0x21;
                prim->u0 = prim->u2 = 0x80;
                prim->u1 = prim->u3 = 0x97;
                prim->v0 = prim->v1 = 0xF0;
                prim->v2 = prim->v3 = 0xFF;
                x0 = self->posX.i.hi - 0x11;
                x1 = self->posX.i.hi + 7;
                prim->x0 = prim->x2 = x0;
                prim->x3 = x1;
                if (i == 0) {
                    prim->x1 = x1;
                    y = self->posY.i.hi + 8;
                } else {
                    y = self->posY.i.hi - 0x18;
                }
                prim->y0 = prim->y1 = y;
                prim->priority = 0xA8;
                prim->drawMode = 2;
                prim->y2 = prim->y3 = y + 0x10;
                prim = (Primitive*)prim->next;
            }
        }
        ENTITY_EXT80(self) = 0x20;
        self->step++;
        return;
    }

    case 3: {
        ENTITY_EXT80(self)--;
        if (ENTITY_EXT80(self) != 0) return;
        self->step++;
        return;
    }

    case 4: {
        Primitive* prim = ENTITY_EXT7C(self);
        Entity* newEntity;
        s32 s1;

        self->animCurFrame = 0;
        g_CastleFlags[0x20] |= 1;
        for (i = 0; i < 10; i++) {
            g_Tilemap.fg[D_us_80180AEC[i]] = D_us_80180B00[i];
        }

        for (s1 = 0; s1 < 8; s1 += 2) {
            s32 half = s1 / 2;
            s16 offX = (s16)((s1 & 1) * 0x10 - 8);
            s16 offY = (s16)(half * 0x10 - 0x18);
            Primitive* child = (Primitive*)prim->next;

            UnkPolyFunc2(prim);
            child->x1 = self->posX.i.hi + offX;
            child->y0 = self->posY.i.hi + offY;
            child->r3 = (u8)s1;
            prim = (Primitive*)child->next;
        }

        newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (newEntity != NULL) {
            CreateEntityFromEntity(2, self, newEntity);
            newEntity->params = 0xAA13;
            newEntity->posY.i.hi += 0x20;
        }
        for (s1 = 0; s1 < 8; s1++) {
            newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (newEntity == NULL) break;
            CreateEntityFromEntity(6, self, newEntity);
            newEntity->posX.i.hi += 0xF - (Random() & 0x1F);
            newEntity->params = 0xAA10;
            newEntity->posY.i.hi += 0xF - (Random() & 0x1F);
        }
        g_api_PlaySfx(0x644);
        ENTITY_EXT80(self) = 0x180;
        self->step++;
        return;
    }

    case 5: {
        Primitive* prim = ENTITY_EXT7C(self);

        while (prim != NULL) {
            if (prim->p3 & 8) {
                func_us_801AB9EC(prim);
            }
            prim = (Primitive*)prim->next;
        }
        ENTITY_EXT80(self)--;
        if (ENTITY_EXT80(self) != 0) return;
        g_CastleFlags[0x20] |= 2;
        DestroyEntity(self);
        return;
    }
    }
}

// func_us_801AC54C: the smaller destructible pot/urn variant (4 shards, no floor gap).
void func_us_801AC54C(Entity* self) {
    switch (self->step) {
    case 0: {
        InitializeEntity(g_EInitEnvironment);
        self->hitboxState = 0;
        self->animCurFrame = 0;
        self->step++;
        return;
    }

    case 1: {
        if (g_CastleFlags[0x20] & 2) {
            DestroyEntity(self);
            return;
        }
        if (g_CastleFlags[0x20] != 0) {
            g_CastleFlags[0x20] |= 2;
        }
        {
            s16 primIndex = (s16)g_api_AllocPrimitives(PRIM_GT4, 8);
            Primitive* prim;
            s32 s1;

            self->primIndex = primIndex;
            ENTITY_EXT7C(self) = (Primitive*)&g_PrimBuf[primIndex];
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }
            self->flags |= 0x800000;
            for (prim = &g_PrimBuf[primIndex]; prim != NULL; prim = (Primitive*)prim->next) {
                prim->drawMode = 8;
            }

            prim = ENTITY_EXT7C(self);
            for (s1 = 0; s1 < 4; s1++) {
                Primitive* child = (Primitive*)prim->next;

                UnkPolyFunc2(prim);
                child->x1 = self->posX.i.hi;
                child->y0 = self->posY.i.hi;
                child->r3 = (u8)(s1 + 8);
                prim = (Primitive*)child->next;
            }
        }
        self->step++;
        return;
    }

    case 2: {
        Primitive* prim = ENTITY_EXT7C(self);
        bool anyActive = false;

        while (prim != NULL) {
            if (prim->p3 & 8) {
                anyActive = true;
                func_us_801AB9EC(prim);
            }
            prim = (Primitive*)prim->next;
        }
        if (anyActive) return;
        DestroyEntity(self);
        return;
    }
    }
}

// func_us_801AC73C: per-frame physics update for a debris shard's own scratch Primitive
// (self is the CHILD prim from the chain-of-2, i.e. what the other functions call `child`).
// Fixed-point position: x0.x1 (integer.fraction) and y0.y1; velocity packed into the
// x2/y2 and x3/y3 halfword pairs via LOW(). g3 gates one-time (re)initialization; r3 is a
// lifetime counter that hides the shard (drawMode=8) once it expires.
void func_us_801AC73C(Primitive* prim) {
    s32 accumX, accumY;

    if (prim->g3 == 0) {
        prim->r0 = 0x80;
        prim->g0 = 0x80;
        prim->u0 = 1;
        prim->v0 = 1;
        prim->b0 = 0xC0;
        prim->drawMode = 2;
        prim->x0 = g_CurrentEntity->posX.i.hi;
        prim->x1 = 0;
        prim->y1 = 0;
        prim->y0 = g_CurrentEntity->posY.i.hi + 8;
        LOW(prim->x2) = 0x7000 - ((Random() & 7) << 13);
        LOW(prim->x3) = 0x7000 - ((Random() & 7) << 13);
        prim->g3 = 1;
        prim->r3 = 0x20;
    }

    accumX = (prim->x0 << 16) + (u16)prim->x1;
    accumX += LOW(prim->x2);
    prim->x0 = (s16)(accumX >> 16);
    prim->x1 = (u16)accumX;

    accumY = (prim->y0 << 16) + (u16)prim->y1;
    accumY += LOW(prim->x3);
    prim->y0 = (s16)(accumY >> 16);
    prim->y1 = (u16)accumY;

    prim->r3--;
    LOW(prim->x3) += 0x2000;
    if (prim->r3 == 0) {
        prim->g3 = 0;
        prim->drawMode = 8;
        prim->p3 = 0;
    }
}

// func_us_801AC894: the "spinning" torch variant -- has an extra electrified spin-up
// phase (step 1) driven off a ~30-prim ring before it settles into the same
// destroy/respawn-debris pattern as the other two torch variants.
void func_us_801AC894(Entity* self) {
    s32 i;

    switch (self->step) {
    case 0: {
        InitializeEntity(g_EInitEnvironment);
        self->animCurFrame = 0;
        self->drawFlags |= 4;
        self->rotate = 0xC00;

        if (g_CastleFlags[0x21] != 0) {
            for (i = 0; i < 8; i++) {
                g_Tilemap.fg[D_us_80180B14[i]] = D_us_80180B34[i];
            }
            DestroyEntity(self);
            return;
        }

        for (i = 0; i < 8; i++) {
            g_Tilemap.fg[D_us_80180B14[i]] = D_us_80180B24[i];
        }
        self->hitboxState = 2;
        self->hitPoints = 0x10;
        self->hitboxWidth = 0x10;
        self->hitboxHeight = 0x28;

        {
            s16 primIndex = (s16)g_api_func_800EDB58(0x1E, 0);
            Primitive* prim;

            self->primIndex = primIndex;
            ENTITY_EXT7C(self) = (Primitive*)&g_PrimBuf[primIndex];
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }
            self->flags |= 0x800000;
            prim = &g_PrimBuf[primIndex];
            prim->drawMode = 8;
            for (; prim != NULL; prim = (Primitive*)prim->next) {
                prim->priority = 0x68;
                ENTITY_EXT84(self) = prim;
                if ((Primitive*)prim->next != NULL) {
                    ((Primitive*)prim->next)->drawMode = 8;
                }
            }
        }
        return;
    }

    case 1: {
        Primitive* prim;
        Primitive* tail;

        if (self->hitFlags != 0) {
            s32 s1 = 0;

            do {
                prim = FindFirstUnkPrim(ENTITY_EXT7C(self));
                if (prim != NULL) {
                    prim->p3 = 1;
                    s1++;
                }
            } while (s1 < 0x10);
        }

        for (prim = ENTITY_EXT7C(self); prim != NULL; prim = (Primitive*)prim->next) {
            if (prim->p3 != 0) {
                func_us_801AC73C(prim);
            }
        }

        tail = ENTITY_EXT84(self);
        tail->y0 = 0;
        tail->x0 = 0;
        tail->u0 = 0;
        tail->drawMode = 2;
        if (!(self->flags & 0x100)) return;
        self->animCurFrame = 0;
        self->step++;
        return;
    }

    case 2: {
        s16 primIndex;
        Primitive* prim;
        Entity* newEntity;
        s32 s1;

        g_api_FreePrimitives(self->primIndex);
        self->flags &= ~0x800000;
        g_CastleFlags[0x21] = 1;
        for (i = 0; i < 8; i++) {
            g_Tilemap.fg[D_us_80180B14[i]] = D_us_80180B34[i];
        }

        primIndex = (s16)g_api_AllocPrimitives(PRIM_GT4, 0x18);
        self->primIndex = primIndex;
        ENTITY_EXT7C(self) = (Primitive*)&g_PrimBuf[primIndex];
        if (primIndex == -1) {
            DestroyEntity(self);
            return;
        }
        self->flags |= 0x800000;
        for (prim = &g_PrimBuf[primIndex]; prim != NULL; prim = (Primitive*)prim->next) {
            prim->drawMode = 8;
        }

        prim = ENTITY_EXT7C(self);
        for (s1 = 0; s1 < 4; s1++) {
            s32 half = s1 / 2;
            s16 offX = (s16)((s1 & 1) * 0x10 - 8);
            s16 offY = (s16)(half * 0x10);
            Primitive* child = (Primitive*)prim->next;

            UnkPolyFunc2(prim);
            child->x1 = self->posX.i.hi + offX;
            child->y0 = self->posY.i.hi + offY;
            child->r3 = (u8)(s1 + 0xC);
            prim = (Primitive*)child->next;
        }

        newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (newEntity != NULL) {
            CreateEntityFromEntity(2, self, newEntity);
            newEntity->params = 0xAA13;
            newEntity->posY.i.hi += 0x20;
        }
        for (s1 = 0; s1 < 8; s1++) {
            newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (newEntity == NULL) break;
            CreateEntityFromEntity(6, self, newEntity);
            newEntity->posX.i.hi += 0xF - (Random() & 0x1F);
            newEntity->params = 0xAA10;
            newEntity->posY.i.hi += 0xF - (Random() & 0x1F);
        }
        g_api_PlaySfx(0x644);
        self->step++;
        return;
    }

    case 3: {
        Primitive* prim = ENTITY_EXT7C(self);
        bool anyActive = false;

        while (prim != NULL) {
            if (prim->p3 & 8) {
                anyActive = true;
                func_us_801AB9EC(prim);
            }
            prim = (Primitive*)prim->next;
        }
        if (anyActive) return;
        DestroyEntity(self);
        return;
    }
    }
}
