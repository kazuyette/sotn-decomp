#include "common.h"
#include <stage.h>

extern u16 D_us_801803E8[];
extern u8 D_us_801805CC[];
extern u8 D_us_801805AC[];
extern u16 D_us_801805BC[];
extern u8 D_us_801805B4[];
extern u16 D_us_801805D4[];
extern u8* D_us_8018058C[];
extern Entity g_Entities_224[];
extern void func_us_80198CC0(s32 arg0, Entity* self); /* not yet decompiled, boss/bo7/create_entity.c */
extern void func_us_80198D34(s32 arg0, Entity* self, Entity* other); /* not yet decompiled, boss/bo7/create_entity.c */
extern void ReplaceBreakableWithItemDrop(Entity* self);
extern s32 AnimateEntity(u8* animSet, Entity* self);
extern void MoveEntity(void);

void func_us_80193258(Entity* self) {
    s16 variant;
    Entity* second;
    Entity* other;
    u16* tbl;
    s32 i;

    variant = (u16)self->params >> 12;
    second = (Entity*)((u8*)self + 0xBC);

    if (self->step == 0) {
        InitializeEntity(D_us_801803E8);
        self->zPriority = 0x70;
        self->blendMode = D_us_801805CC[variant];
        self->hitboxHeight = D_us_801805AC[variant];
        self->animSet = D_us_801805BC[variant];
        DestroyEntity(second);
        func_us_80198D34(0x11, self, second);
        if (variant == 0) {
            *(s16*)((u8*)self + 0xC2) -= 0x20;
        } else {
            *(s16*)((u8*)self + 0xC2) -= 0x10;
        }
        *(u16*)((u8*)self + 0xEC) = 1;
    }

    AnimateEntity(D_us_8018058C[variant], self);

    if (self->hitParams != 0) {
        g_api_PlaySfx(0x634);
        g_api_PlaySfx(0x644);

        other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (other != NULL) {
            func_us_80198CC0(2, other);
            other->params = D_us_801805B4[variant] | 0x10;
        }

        tbl = D_us_801805D4;
        for (i = 0; i < 4; i++) {
            other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (other != NULL) {
                func_us_80198D34(0x1A, self, other);
                other->posX.i.hi += *tbl++;
                other->posY.i.hi += *tbl++;
                if (variant != 0) {
                    other->posY.i.hi -= 0x14;
                }
                other->params = (u16)i;
            }
        }

        if (variant != 0) {
            for (i = 0; i < 3; i++) {
                other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
                if (other != NULL) {
                    func_us_80198D34(0x1A, self, other);
                    other->posX.i.hi += *tbl++;
                    other->params = (u16)(i + 4);
                    other->posY.i.hi += *tbl++;
                }
            }
        }

        DestroyEntity(second);
        ReplaceBreakableWithItemDrop(self);
    }
}

extern u8 GetSideToPlayer(void); /* st_common.h, not pulled in by this overlay */
extern void UnkPolyFunc2(Primitive* prim);
extern void UnkPrimHelper(Primitive* prim);

void func_us_801934E8(Entity* self) {
    Entity* other;
    Primitive* prim;
    Collider coll;
    s32 side;
    s32 idx;

    if (self->step == 1) {
        goto L8019370C;
    }
    if (self->step != 0) {
        return;
    }

    InitializeEntity(D_us_801803E8);
    self->zPriority = 0x70;
    self->hitboxState = 0;
    self->drawFlags = 4;
    self->animCurFrame = self->params + 0x1C;
    GetSideToPlayer(); /* return value discarded; called for its side effect */

    side = Random() & 1;
    *(s16*)((u8*)self + 0x80) = ((Random() & 0x1E) + 8);
    if (self->facingLeft != 0) {
        *(s16*)((u8*)self + 0x80) = -*(s16*)((u8*)self + 0x80);
    }
    if (self->params >= 4) {
        *(s16*)((u8*)self + 0x80) = -*(s16*)((u8*)self + 0x80);
    }

    self->velocityX = side ? 0x10000 : 0xFFFF0000;
    self->velocityY = 0xFFFD0000; /* -0x28000 */
    self->velocityX = self->velocityX + 0x8000 - (Random() << 8);
    self->velocityY = ((self->params / 2) * 3 << 13) + 0xFFFD0000;
    if (self->params == 6) {
        self->velocityX = 0;
        self->velocityY = 0;
        self->step = 2;
    }

    if (self->params != 0) {
        return;
    }

    self->primIndex = 0;
    idx = g_api_AllocPrimitives(PRIM_GT4, 2);
    if (idx == -1) {
        return;
    }
    self->primIndex = idx;
    self->flags |= 0x800000;
    prim = &g_PrimBuf[idx];
    UnkPolyFunc2(prim);
    prim->tpage = 0x1A;
    prim->clut = 0x159;
    prim->u0 = 0x40;
    prim->u2 = 0x40;
    prim->u1 = 0x60;
    prim->u3 = 0x60;
    prim->v0 = 0;
    prim->v1 = 0;
    prim->v2 = 0x20;
    prim->v3 = 0x20;
    prim->next->x1 = self->posX.i.hi + 4;
    prim->next->y0 = self->posY.i.hi - 8;
    prim->next->x2 = 0x20;
    prim->next->y2 = 0x20;
    prim->next->b3 = 0x10;
    prim->priority = 0x72;
    prim->drawMode = 0x37;
    return;

L8019370C:
    MoveEntity();
    self->rotate += *(s16*)((u8*)self + 0x80);
    self->velocityY += 0x4000;
    g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 6, &coll, 0);
    if (coll.effects & 1) {
        self->posY.i.hi += coll.unk18;
        self->velocityY = (-self->velocityY) / 2;
        self->velocityX = self->velocityX - (self->velocityX / 3);
        if (self->velocityY <= -0xA000) {
            goto L80193804;
        }
        other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (other != NULL) {
            func_us_80198D34(6, self, other);
            other->params = 0x10;
        }
        DestroyEntity(self);
        return;
    }

L80193804:
    if (self->primIndex == 0) {
        return;
    }
    prim = &g_PrimBuf[self->primIndex];
    UnkPrimHelper(prim);
    prim->next->y2 += 4;
    prim->next->x2 = prim->next->y2;
    if (prim->next->x2 < 0x41) {
        return;
    }
    prim->next->b3 -= 4;
    if (prim->next->b3 != 0) {
        return;
    }
    g_api_FreePrimitives(self->primIndex);
    self->flags &= ~0x800000;
    self->primIndex = 0;
}

/* p3 is used as a step counter (see primitive.h's comment on Primitive::p3 -
 * "CHI uses this like a step value"; CHI = this boss's internal name). */
void func_us_801938E0(Primitive* self) {
    u8 cnt;

    switch (self->p3) {
        case 1:
            self->drawMode = 2;
            self->u0 = self->v0 = (Random() & 1) + 1;
            self->r0 = 0x60;
            self->g0 = 0x80;
            self->b0 = 0x30;
            self->priority = 0xA0;
            self->p2 = (u8)((Random() & 0x1F) + 0x10);
            self->p3 = 2;
            /* fallthrough */
        case 2:
            self->y0 += 2;
            cnt = self->p2 - 1;
            self->p2 = cnt;
            if (cnt == 0 || (g_Tilemap.scrollY.i.hi + self->y0) >= 0xA1) {
                self->drawMode = 8;
                self->p3 = 0;
            }
            break;
        default:
            break;
    }
}

extern u16 D_us_80180454[];

void func_us_801939CC(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180454);
            self->hitPoints = 0x7FFF;
            self->hitboxWidth = 6;
            self->animCurFrame = 3;
            self->hitboxState = 3;
            self->hitboxHeight = 8;
            if (g_CastleFlags[0x50] != 0) {
                self->animCurFrame = 4;
            }
            /* fallthrough */
        case 1:
            if (self->hitParams == 0) {
                return;
            }
            g_api_PlaySfx(0x640);
            g_CastleFlags[0x50] = 1;
            g_api_RevealSecretPassageAtPlayerPositionOnMap(0x50);
            self->animCurFrame = 4;
            self->step++;
            break;
        default:
            break;
    }
}

extern u16 D_us_801805F0[];
extern u16 D_us_80180608[];
extern Primitive* FindFirstUnkPrim(Primitive* prim);
extern u16 g_pads_1_pressed;

void func_us_80193AB0(Entity* self) {
    Entity* other;
    Primitive* prim;
    Primitive* chain;
    u16* tbl;
    s32 col, row;
    s32 t0;
    s32 idx;
    s32 t1;
    s16 s2, s3;

    if (self->step >= 0x11) {
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180454);
            self->animCurFrame = 1;
            t0 = 0x6D;
            tbl = D_us_801805F0;
            if (g_CastleFlags[0x50] != 0) {
                tbl += 0xC;
            }
            for (col = 0; col < 3; col++) {
                for (row = 0; row < 4; row++) {
                    g_Tilemap.fg[t0 + row * 16] = *tbl++;
                }
                t0++;
            }
            if (g_CastleFlags[0x50] == 0) {
                return;
            }
            self->animCurFrame = 0;
            self->step = 0x10;
            return;

        case 1:
            if (g_CastleFlags[0x50] == 0) {
                return;
            }
            goto L80193C2C;

        case 2:
            idx = g_api_AllocPrimitives(PRIM_TILE, 0x10);
            if ((s16)idx == -1) {
                DestroyEntity(self);
                return;
            }
            prim = &g_PrimBuf[(s16)idx];
            self->flags |= 0x800000;
            self->primIndex = (s16)idx;
            *(Primitive**)((u8*)self + 0x7C) = prim;
            for (chain = prim; chain != NULL; chain = chain->next) {
                chain->drawMode = 8;
            }
        L80193C2C:
            self->step++;
            return;

        case 3:
            *(s32*)((u8*)self + 0x80) += 1;
            if (*(s32*)((u8*)self + 0x80) & 1) {
                self->posY.i.hi += 1;
            } else {
                self->posY.i.hi -= 1;
            }
            if ((*(s32*)((u8*)self + 0x80) & 7) == 0) {
                g_api_PlaySfx(0x644);
            }
            MoveEntity();
            if (self->velocityX < 0x4000) {
                self->velocityX += 0x200;
            }

            prim = FindFirstUnkPrim(*(Primitive**)((u8*)self + 0x7C));
            if (prim != NULL) {
                s16 baseX;

                prim->p3 = 1;
                baseX = self->posX.i.hi + (Random() & 0x3F);
                s2 = baseX - 0x18;
                if (s2 >= 0x101) {
                    s2 = baseX - 0x28;
                }
                prim->x0 = s2;
                s3 = self->posY.i.hi - 0x20;
                prim->y0 = s3;
            }

            for (chain = *(Primitive**)((u8*)self + 0x7C); chain != NULL; chain = chain->next) {
                if (chain->p3 != 0) {
                    func_us_801938E0(chain);
                }
            }

            s2 = self->posX.i.hi - 0x18;
            s3 = self->posY.i.hi + 0x20;
            other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (other != NULL) {
                func_us_80198CC0(0x15, other);
                other->posX.i.hi = s2 + (Random() & 0x1F);
                other->posY.i.hi = s3;
                other->params = Random() & 3;
                other->zPriority = 0xA0;
            }

            t1 = (self->posX.i.hi - 0xE8) >> 4;
            t0 = 0x6D;
            if (t1 >= 4) {
                t1 = 3;
            }
            tbl = D_us_80180608;
            for (col = 0; col < t1; col++) {
                for (row = 0; row < 4; row++) {
                    g_Tilemap.fg[t0 + row * 16] = *tbl++;
                }
                t0++;
            }

            if (self->posX.i.hi < 0x129) {
                return;
            }
            DestroyEntity(self);
            return;

        case 0x10:
            if (g_pads_1_pressed & 0x80) {
                if (self->params != 0) {
                    return;
                }
                self->animCurFrame++;
                self->params |= 1;
            } else {
                self->params = 0;
            }
            if (g_pads_1_pressed & 0x20) {
                if (self->step_s != 0) {
                    return;
                }
                self->animCurFrame--;
                self->step_s |= 1;
            } else {
                self->step_s = 0;
            }
            return;

        default:
            return;
    }
}

void func_us_80193EF0(Entity* self) {
    Entity* other;
    s32 i;
    Collider coll;

    if (self->step == 1) {
        if (self->params & 0x100) {
            self->params &= 0xFF;
            self->step++;
        }
        return;
    }
    if (self->step < 2) {
        if (self->step != 0) {
            return;
        }
        InitializeEntity(D_us_80180454);
        self->drawFlags = 4;
        self->zPriority = 0x69;
        if (self->rotate & 1) {
            self->animCurFrame = (u8)self->params;
            self->facingLeft = 1;
            self->rotate &= 0xFFF0;
        }
        self->velocityX = (Random() & 0xF) << 12;
        if (self->animCurFrame == 0xD) {
            self->velocityX += 0x4000;
        }
        self->velocityY = ((Random() & 7) << 11) - 0x4000;
        if (self->animCurFrame < 0xB) {
            self->velocityY -= 0x10000;
        }
        *(u16*)((u8*)self + 0x9C) = ((Random() & 3) + 1) << 5;
        return;
    }
    if (self->step != 2) {
        return;
    }

    self->rotate += *(u16*)((u8*)self + 0x9C);
    MoveEntity();
    self->velocityY += 0x2000;
    g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 6, &coll, 0);
    if (!(coll.effects & 1)) {
        return;
    }
    self->posY.i.hi += coll.unk18;
    if (self->animCurFrame < 0xC) {
        for (i = 0; i < 2; i++) {
            other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (other != NULL) {
                func_us_80198D34(0x19, self, other);
                other->params = ((Random() & 3) + 9) | 0x100;
            }
        }
        DestroyEntity(self);
        return;
    }
    if (self->velocityY > 0x7FFF) {
        self->velocityY = (-self->velocityY * 2) / 3;
        return;
    }
    other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
    if (other != NULL) {
        func_us_80198D34(6, self, other);
        other->params = 0xC010;
    }
    DestroyEntity(self);
}

extern u16 D_us_80180620[];
extern u16 D_us_80180650[];
extern Entity g_Entities_160[];

void func_us_801941A0(Entity* self) {
    Entity* other;
    Primitive* prim;
    Primitive* chain;
    u16* tbl;
    u8* cur;
    u8* trk;
    u8* p2;
    s32 i, row;
    s32 idx;
    s32 colIndex;
    s16 v0;
    s16 t0, t1, t2, t3, a0, a2, a3;
    u16* ext84 = (u16*)((u8*)self + 0x84);
    u16* ext94 = (u16*)((u8*)self + 0x94);

    if (self->step >= 9) {
        return;
    }

    switch (self->step) {
        case 0: {
            InitializeEntity(D_us_80180454);
            self->hitPoints = 0x20;
            self->hitboxWidth = 0x18;
            self->hitboxHeight = 0x20;
            self->hitboxState = 2;
            self->flags |= 0x400000;
            self->animCurFrame = 0;

            tbl = D_us_80180620;
            colIndex = 0x160;
            if (g_CastleFlags[0x51] != 0) {
                tbl += 0xC;
            }
            for (i = 0; i < 3; i++) {
                for (row = 0; row < 4; row++) {
                    g_Tilemap.fg[colIndex + row * 16] = *tbl++;
                }
                colIndex++;
            }

            if (g_CastleFlags[0x51] != 0) {
                goto L80194724;
            }

            idx = g_api_AllocPrimitives(PRIM_GT4, 2);
            if ((s16)idx == -1) {
                goto L80194724;
            }
            prim = &g_PrimBuf[(s16)idx];
            tbl = D_us_80180650;
            cur = (u8*)self + 0xBC;
            trk = (u8*)self + 0xDA;
            self->flags |= 0x800000;
            self->primIndex = (s16)idx;
            *(Primitive**)((u8*)self + 0x7C) = prim;

            t1 = self->posX.i.hi - 0x17;
            a3 = self->posX.i.hi + 9;
            t0 = self->posY.i.hi - 0x1F;
            a0 = self->posY.i.hi + 0x21;
            a2 = self->posX.i.hi + 0x19;

            prim->clut = 7;
            prim->u2 = 0x94;
            prim->u0 = 0x94;
            prim->x2 = t1;
            prim->x0 = t1;
            prim->tpage = 0xF;
            prim->u3 = 0xB4;
            prim->u1 = 0xB4;
            prim->v1 = 0x84;
            prim->v0 = 0x84;
            prim->v3 = 0xC4;
            prim->v2 = 0xC4;
            prim->x3 = a3;
            prim->x1 = a3;
            prim->y1 = t0;
            prim->y0 = t0;
            prim->y3 = a0;
            prim->y2 = a0;
            prim->priority = 0x6A;
            prim->drawMode = 2;

            chain = prim->next;
            chain->clut = 8;
            chain->u2 = 0xBC;
            chain->u0 = 0xBC;
            chain->tpage = 0xF;
            chain->u3 = 0xCC;
            chain->u1 = 0xCC;
            chain->v1 = 0x84;
            chain->v0 = 0x84;
            chain->v3 = 0xC4;
            chain->v2 = 0xC4;
            chain->x2 = a3;
            chain->x0 = a3;
            chain->x3 = a2;
            chain->x1 = a2;
            chain->y1 = t0;
            chain->y0 = t0;
            chain->y3 = a0;
            chain->y2 = a0;
            chain->priority = 0x6A;
            chain->drawMode = 2;

            for (i = 0; i < 15; i++) {
                DestroyEntity((Entity*)cur);
                func_us_80198D34(0x19, self, (Entity*)cur);
                cur += 0xBC;
                *(u16*)(trk + 0x12) = *tbl++;
                *(u16*)(trk - 0x1C) = *(u16*)(trk - 0x1C) + *tbl++;
                *(u16*)(trk - 0x18) = *(u16*)(trk - 0x18) + *tbl++;
                *(u16*)(trk + 0x0) = *tbl++;
                trk += 0xBC;
            }
            return;
        }

        case 1:
            if (!(self->flags & 0x100)) {
                return;
            }
            g_api_PlaySfx(0x644);
            tbl = D_us_80180620;
            self->hitPoints = 0x20;
            self->hitboxWidth = (u8)(self->hitboxWidth - 8);
            *ext84 = (u16)(*ext84 + 1);
            self->flags &= ~0x101;
            self->hitboxOffX -= 8;
            colIndex = 0x163 - *ext84;
            tbl += (0x18 - *ext84 * 4);
            for (row = 0; row < 4; row++) {
                g_Tilemap.fg[colIndex + row * 16] = *tbl++;
            }

            {
                s32 off = (*ext84 - 1) * 940;
                p2 = (u8*)self + off + 0xE8;
                for (i = 0; i < 5; i++) {
                    *(u16*)p2 = *(u16*)p2 + 1;
                    p2 += 0xBC;
                }
            }

            {
                s16 sx = self->posX.i.hi + 0x20 - *ext84 * 12;
                s16 sy = self->posY.i.hi;

                other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
                if (other != NULL) {
                    func_us_80198D34(2, self, other);
                    other->posY.i.hi = sy + 0x10;
                    other->posX.i.hi = sx;
                    other->params = 0xC013;
                }
                for (i = 0; i < 3; i++) {
                    other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
                    if (other != NULL) {
                        func_us_80198D34(6, self, other);
                        other->posX.i.hi = sx;
                        other->posY.i.hi = sy - (((Random() & 3) << 3) - 0x20);
                        other->params = 0xC010;
                    }
                }
            }
            v0 = self->step + *ext84;
            goto L80194750;

        case 2:
            prim = *(Primitive**)((u8*)self + 0x7C);
            prim->next->drawMode = 8;
            *ext94 = 0x14;
            self->step = 8;
            return;

        case 3:
            prim = *(Primitive**)((u8*)self + 0x7C);
            prim->u3 -= 0x10;
            prim->u1 = prim->u3;
            prim->x3 -= 0x10;
            prim->x1 = prim->x3;
            other = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
            if (other != NULL) {
                func_us_80198CC0(0xC, other);
                other->posX.i.hi = 0x20 - g_Tilemap.scrollX.i.hi;
                other->params = 3;
                other->posY.i.hi = 0x188 - g_Tilemap.scrollY.i.hi;
            }
            *ext94 = 0x14;
            v0 = 8;
            goto L80194750;

        case 4:
            prim = *(Primitive**)((u8*)self + 0x7C);
            prim->next->drawMode = 8;
            self->hitboxState = 0;
            g_CastleFlags[0x51] = 1;
            g_api_RevealSecretPassageAtPlayerPositionOnMap(0x51);
            goto L80194724;

        case 8:
            *ext94 -= 1;
            if (*ext94 != 0) {
                return;
            }
            v0 = 1;
            goto L80194750;

        default:
            return;
    }

L80194724:
    DestroyEntity(self);
    return;

L80194750:
    self->step = v0;
}

extern void PlaySfxPositional(s16 sfxId);
extern s32 D_us_80180848;

void func_us_8019477C(Entity* self) {
    Entity* other;
    Primitive* prim;
    Primitive* chain;
    Primitive* found;
    Collider coll;
    s32 idx;
    s16 a1;
    s16 v0;
    s32 i;

    if (self->step >= 7) {
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180454);
            if (self->params == 0) {
                self->zPriority = 0x6A;
                a1 = 0xC0;
                self->animCurFrame = 5;
                v0 = 6;
            } else {
                self->zPriority = 0x6A;
                a1 = 0xDF;
                self->animCurFrame = 6;
                v0 = 0x1FA;
            }
            self->posX.i.hi = v0 - g_Tilemap.scrollX.i.hi;
            for (i = 0; i < 4; i++) {
                g_Tilemap.fg[a1] = 0x32E;
                a1 += 0x20;
            }

            idx = g_api_AllocPrimitives(PRIM_GT4, 0x10);
            if ((s16)idx == -1) {
                self->step = 0;
                return;
            }
            prim = &g_PrimBuf[(s16)idx];
            self->flags |= 0x800000;
            self->primIndex = (s16)idx;
            *(Primitive**)((u8*)self + 0x7C) = prim;
            prim->tpage = 0xF;
            prim->clut = 0x1B;
            if (self->params == 0) {
                prim->u2 = 0xE3;
                prim->u0 = 0xE3;
                v0 = 0xD3;
            } else {
                prim->u2 = 0xD4;
                prim->u0 = 0xD4;
                v0 = 0xE4;
            }
            prim->u3 = v0;
            prim->u1 = v0;
            prim->v1 = 0x9C;
            prim->v0 = 0x9C;
            prim->v3 = 0xDC;
            prim->v2 = 0xDC;
            prim->y1 = 0x1C;
            prim->y0 = 0x1C;
            prim->y3 = 0x5C;
            prim->y2 = 0x5C;
            prim->priority = 0x6B;
            prim->drawMode = 2;
            {
                s16 xBase = (self->params != 0 ? 0x1F0 : 0) - g_Tilemap.scrollX.i.hi;
                prim->x2 = xBase;
                prim->x0 = xBase;
                prim->x1 = xBase + 0x10;
                prim->x3 = xBase + 0x10;
            }
            chain = prim->next;
            for (; chain != NULL; chain = chain->next) {
                chain->type = 1;
                chain->v0 = 1;
                chain->u0 = 1;
                chain->r0 = 0x20;
                chain->g0 = 0x40;
                chain->b0 = 0x20;
                chain->priority = 0x6C;
                chain->drawMode = 8;
            }
            return;

        case 1:
            if (D_us_80180848 & 1) {
                goto L80194CB8;
            }
            return;

        case 2:
            self->posY.val += self->velocityY;
            if (self->posY.i.hi < 0x61) {
                self->velocityY += 0x8000;
                return;
            }
            self->velocityY += 0x8000;
            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 0x22, &coll, 0);
            if (!(coll.effects)) {
                return;
            }
            self->posY.i.hi += coll.unk18;
            self->velocityY = (-self->velocityY) / 4;
            PlaySfxPositional(0x655);
            other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (other != NULL) {
                func_us_80198D34(6, self, other);
                other->params = 0x10;
                other->posX.i.hi = other->posX.i.hi - 8 + ((Random() & 7) << 1);
                other->posY.i.hi += 0x20;
            }
            if (self->velocityY < -0x1FFF) {
                return;
            }
            goto L80194CB8;

        case 3:
            a1 = (self->params == 0) ? 0xC0 : 0xDF;
            for (i = 0; i < 4; i++) {
                g_Tilemap.fg[a1] = 0x3A1;
                a1 += 0x20;
            }
            goto L80194CB8;

        case 4:
            if (D_us_80180848 & 0x10) {
                goto L80194CB8;
            }
            return;

        case 5:
            self->posY.val -= 0x4000;
            if (g_Timer & 1) {
                prim = *(Primitive**)((u8*)self + 0x7C);
                found = FindFirstUnkPrim(prim->next);
                if (found != NULL) {
                    found->p3 = 1;
                    found->y0 = 0x5C;
                    found->drawMode = 2;
                    found->x0 = self->posX.i.hi + (Random() & 0xF) - 8;
                }
            }
            {
                s16 a1_2 = 0x120;
                s16 v1 = 0x7A - self->posY.i.hi;
                if (self->params != 0) {
                    v1 >>= 4;
                    a1_2 = 0x13F;
                }
                if (v1 > 0) {
                    for (i = 0; i < v1; i++) {
                        g_Tilemap.fg[a1_2] = 0x32E;
                        a1_2 -= 0x20;
                    }
                }
            }
            /* fallthrough */
        case 6:
        L80194C3C:
            *(u16*)((u8*)self + 0x80) = 0x20;
            if (self->posY.i.hi < 0x3A) {
                self->step++;
            }
            prim = *(Primitive**)((u8*)self + 0x7C);
            if (prim != NULL) {
                for (chain = prim; chain != NULL; chain = chain->next) {
                    if (chain->p3 != 0) {
                        chain->y0 += 2;
                        if (chain->y0 >= 0xA1) {
                            chain->drawMode = 8;
                            chain->p3 = 0;
                        }
                    }
                }
            }
            *(u16*)((u8*)self + 0x80) -= 1;
            if (*(u16*)((u8*)self + 0x80) != 0) {
                return;
            }
            goto L80194CB8;

        default:
            return;
    }

L80194CB8:
    self->step++;
}

/* pushes g_CurrentEntity up so it doesn't sink through a floor sensor probed at
 * sensor[1] (offset 2, same s16-table convention as bo5's UnkCollisionFunc2/3);
 * clears velocity on contact. returns 1 if a push happened, 0 otherwise. */
s32 func_us_80194CE0(s16* sensor) {
    s32 gap;

    gap = 0xD0 - (g_CurrentEntity->posY.i.hi + sensor[1] + g_Tilemap.scrollY.i.hi);
    if (gap <= 0) {
        return 0;
    }
    g_CurrentEntity->posY.i.hi += gap;
    g_CurrentEntity->velocityX = 0;
    g_CurrentEntity->velocityY = 0;
    return 1;
}

extern u16 D_us_80180460[];
extern u8 D_us_801806FC[];
extern u8 D_us_80180708[];
extern u8 D_us_8018071C[];
extern u8 D_us_80180748[];
extern u8 D_us_80180754[];
extern u8 D_us_80180760[];
extern u8 D_us_80180778[];
extern u8 D_us_80180788[];
extern u8 D_us_8018072C[];
extern u8 D_us_8018076C[];
extern s16 D_us_801806D8[];
extern u16 D_us_801806F0[];
extern u16 D_us_801806F2[];
extern u8 D_us_8018073C[];
extern u8 D_us_8018081C[];
extern u8 D_us_801807E4[];
extern char D_us_80192E34[];
extern Entity g_Entities_192[];
extern Entity g_Entities_128[];
extern void SetStep(u8 step);
extern void SetSubStep(u8 step_s);
extern s16 GetDistanceToPlayerX(void);

void func_us_80194D3C(Entity* self) {
    Entity* other;
    s16 v0, v1, s2, s3;
    s32 i;

    if (self->flags & 0x100) {
        if (self->step != 8) {
            SetStep(8);
        }
    }
    v1 = self->step;
    if (v1 == 6) {
        goto L80195548;
    }
    if (v1 < 7) {
        if (v1 == 3) {
            goto L80194E78;
        }
        if (v1 < 4) {
            if (v1 == 0) {
                goto L80194E2C;
            }
            if (v1 == 2) {
                goto L80194E48;
            }
            goto L80195914;
        }
        if (v1 == 4) {
            goto L80195434;
        }
        if (v1 == 5) {
            goto L80195110;
        }
        goto L80195914;
    } else {
        if (v1 == 0xA) {
            goto L801952C0;
        }
        if (v1 < 0xB) {
            if (v1 == 8) {
                goto L80195680;
            }
            if (v1 == 9) {
                goto L80195380;
            }
            goto L80195914;
        }
        if (v1 == 0xB) {
            goto L8019507C;
        }
        if (v1 == 0xFF) {
            goto L80195870;
        }
        goto L80195914;
    }

L80194E2C:
    InitializeEntity(D_us_80180460);
    self->hitboxState = 0;
    SetStep(2);
    /* fallthrough */

L80194E48:
    AnimateEntity(D_us_801806FC, self);
    if (D_us_80180848 & 2) {
        self->hitboxState = 3;
        goto L80195538;
    }
    goto L80195914;

L80194E78:
    if (self->step_s == 0) {
        *(u16*)((u8*)self + 0x80) = 0x40;
        self->step_s += 1;
    }
    if (!AnimateEntity(D_us_80180708, self)) {
        PlaySfxPositional(0x783);
    }
    if (GetDistanceToPlayerX() < 0x68) {
        *(u8*)((u8*)self + 0x84) = 1;
    }
    if (!(GetDistanceToPlayerX() < 0x81)) {
        *(u8*)((u8*)self + 0x84) = 0;
    }
    if (GetDistanceToPlayerX() < 0x31) {
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
    }
    MoveEntity();
    self->velocityX = (self->facingLeft == *(u8*)((u8*)self + 0x84)) ? 0xFFFF8000 : 0x8000;
    *(u16*)((u8*)self + 0x80) -= 1;
    if (*(u16*)((u8*)self + 0x80) != 0) {
        return;
    }
    SetStep(0xA);
    s2 = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
    if (self->facingLeft) {
        s2 = 0x200 - s2;
    }
    s3 = (GetSideToPlayer() & 1) ^ 1;
    {
        s16 absS2 = (s2 >= 0) ? s2 : -s2;
        if (absS2 >= 0x51) {
            if (self->facingLeft != s3) {
                SetStep(9);
            }
        }
        if (s2 >= 0x141) {
            if (self->facingLeft == s3) {
                SetStep(6);
            }
        }
        if (s2 >= 0x181) {
            if (self->facingLeft == s3) {
                SetStep(4);
            }
        }
    }
    if (!(*(s32*)((u8*)&g_Player + 0x35C) & 1)) {
        if (!(g_Entities[0].posY.i.hi < 0xA0)) {
            return;
        }
        if (!(*(s32*)((u8*)&g_Player + 0x350) & 1)) {
            return;
        }
    }
    SetStep(5);
    return;

L8019507C:
    if (self->step_s == 0) {
        goto L8019509C;
    }
    if (self->step_s == 1) {
        goto L801950DC;
    }
    return;

L8019509C:
    if (AnimateEntity(D_us_8018071C, self)) {
        return;
    }
    self->velocityX = (self->facingLeft != 0) ? 0xFFFA0000 : 0x60000;
    self->step_s += 1;
    *(u16*)((u8*)self + 0x80) = 0x40;
    goto L80195840;

L801950DC:
    MoveEntity();
    v0 = self->velocityX;
    self->velocityX = v0 - (v0 >> 6);
    *(u16*)((u8*)self + 0x80) -= 1;
    if (*(u16*)((u8*)self + 0x80) != 0) {
        return;
    }
    goto L80195538;

L80195110:
    if (self->step_s == 1) {
        goto L80195164;
    }
    if (self->step_s < 2) {
        if (self->step_s == 0) {
            goto L80195154;
        }
        goto L80195914;
    }
    if (self->step_s == 2) {
        goto L801951A4;
    }
    if (self->step_s == 3) {
        goto L801952A0;
    }
    return;

L80195154:
    /* fallthrough to shared anim+SetSubStep(1) block */
    if (!AnimateEntity(D_us_80180754, self)) {
        SetSubStep(1);
    }
    return;

L80195164:
    if (AnimateEntity(D_us_80180748, self)) {
        return;
    }
    PlaySfxPositional(0x782);
    PlaySfxPositional(0x7CF);
    *(u16*)((u8*)self + 0x80) = 0x80;
    SetSubStep(2);
    return;

L801951A4:
    if (!(*(u16*)((u8*)self + 0x80) & 3)) {
        s3 = 0;
        for (i = 0; i < 3; i++) {
            other = AllocEntity(g_Entities_128, (Entity*)((u8*)g_Entities_128 + 0x4680));
            if (other != NULL) {
                func_us_80198D34(0x1E, self, other);
                if (self->facingLeft == 0) {
                    other->posX.i.hi = other->posX.i.hi + D_us_801806F0[i];
                } else {
                    other->posX.i.hi = other->posX.i.hi - D_us_801806F0[i];
                }
                other->posY.i.hi = other->posY.i.hi + D_us_801806F2[i];
                other->params = i;
                other->zPriority = self->zPriority + 1;
                other->facingLeft = self->facingLeft;
            }
        }
    }
    *(u16*)((u8*)self + 0x80) -= 1;
    if (*(u16*)((u8*)self + 0x80) != 0) {
        return;
    }
    v0 = self->step_s + 1;
    goto L80195844;

L801952A0:
    if (!AnimateEntity(D_us_80180760, self)) {
        return;
    }
    goto L80195538;

L80195538:
    SetStep(3);
    return;

L801952C0:
    if (!AnimateEntity(D_us_8018073C, self)) {
        SetStep(3);
    }
    if (*(s32*)((u8*)self + 0x50) != 3) {
        return;
    }
    PlaySfxPositional(0x7C6);
    PlaySfxPositional(0x782);
    other = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
    if (other == NULL) {
        return;
    }
    func_us_80198D34(0x1D, self, other);
    other->facingLeft = self->facingLeft;
    if (self->facingLeft == 0) {
        other->posX.i.hi -= 0x18;
    } else {
        other->posX.i.hi += 0x18;
    }
    other->params = *(u8*)((u8*)self + 0x85);
    self->zPriority += 1;
    other->zPriority = self->zPriority;
    return;

L80195380:
    if (self->step_s == 1) {
        goto L801953B4;
    }
    if (self->step_s < 2) {
        goto L80195450;
    }
    if (self->step_s == 2) {
        goto L801953E0;
    }
    if (self->step_s == 3) {
        goto L80195520;
    }
    return;

L80195450:
    if (self->step_s == 0) {
        goto L80195478;
    }
    return;

L80195478:
    if (!AnimateEntity(D_us_80180778, self)) {
        SetSubStep(1);
    }
    return;

L801953B4:
    self->velocityY = 0xFFFA0000;
    self->velocityX = (self->facingLeft == 0) ? 0x28000 : 0xFFFD8000;
    self->step_s += 1;
    goto L801953E0;

L801953E0:
    MoveEntity();
    self->velocityY += 0x3800;
    if (func_us_80194CE0(D_us_801806D8) & 1) {
        PlaySfxPositional(0x63D);
        g_api_func_80102CD8(1);
        v0 = self->step_s + 1;
        goto L80195844;
    }
    return;

L80195520:
    if (!AnimateEntity(D_us_80180788, self)) {
        return;
    }
    goto L80195538;

L80195434:
    if (self->step_s == 1) {
        goto L801954A0;
    }
    if (self->step_s < 2) {
        goto L80195450;
    }
    if (self->step_s == 2) {
        goto L801954CC;
    }
    if (self->step_s == 3) {
        goto L80195520;
    }
    return;

L801954A0:
    self->velocityY = 0xFFFA0000;
    self->velocityX = (self->facingLeft == 0) ? 0xFFFD8000 : 0x28000;
    self->step_s += 1;
    goto L801954CC;

L801954CC:
    MoveEntity();
    self->velocityY += 0x3800;
    if (func_us_80194CE0(D_us_801806D8) & 1) {
        PlaySfxPositional(0x63D);
        g_api_func_80102CD8(1);
        v0 = self->step_s + 1;
        goto L80195844;
    }
    return;

L80195548:
    if (self->step_s == 0) {
        goto L80195568;
    }
    if (self->step_s == 1) {
        goto L8019557C;
    }
    return;

L80195568:
    *(u8*)((u8*)self + 0x86) = 8;
    self->step_s += 1;
    /* fallthrough */

L8019557C:
    if (!AnimateEntity(D_us_8018072C, self)) {
        *(u8*)((u8*)self + 0x86) -= 1;
        if (*(u8*)((u8*)self + 0x86) == 0) {
            SetStep(3);
        }
    }
    if (*(s32*)((u8*)self + 0x50) != 5) {
        return;
    }
    PlaySfxPositional(0x7C6);
    PlaySfxPositional(0x782);
    other = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
    if (other != NULL) {
        func_us_80198D34(0x1D, self, other);
        other->facingLeft = self->facingLeft;
        if (self->facingLeft == 0) {
            other->posX.i.hi -= 0x18;
        } else {
            other->posX.i.hi += 0x18;
        }
        other->params = *(u8*)((u8*)self + 0x85);
        other->zPriority = self->zPriority + 1;
    }
    *(u8*)((u8*)self + 0x85) += 1;
    if (*(u8*)((u8*)self + 0x85) < 3) {
        return;
    }
    *(u8*)((u8*)self + 0x85) = 0;
    return;

L80195680:
    if (self->step_s >= 5) {
        return;
    }
    switch (self->step_s) {
        case 0:
            D_us_80180848 |= 4;
            self->hitboxState = 0;
            PlaySfxPositional(0x780);
            self->step_s += 1;
            /* fallthrough */
        case 1:
            if ((g_Timer & 7) == 0) {
                PlaySfxPositional(0x655);
                other = AllocEntity(g_Entities_192, (Entity*)((u8*)g_Entities_192 + 0x2F00));
                if (other != NULL) {
                    func_us_80198D34(0x20, self, other);
                    other->params = 1;
                    self->zPriority += 1;
                    other->zPriority = self->zPriority;
                    other->posX.i.hi = other->posX.i.hi - 0x20 + (Random() & 0x3F);
                    other->posY.i.hi = other->posY.i.hi - 0xC + (Random() & 0x1F);
                }
            }
            if (AnimateEntity(D_us_8018076C, self)) {
                return;
            }
            PlaySfxPositional(0x7C5);
            *(u16*)((u8*)self + 0x80) = 0x50;
            v0 = self->step_s + 1;
            goto L80195844;

        case 2:
            other = AllocEntity(g_Entities_192, (Entity*)((u8*)g_Entities_192 + 0x2F00));
            if (other != NULL) {
                func_us_80198D34(0x20, self, other);
                other->params = 2;
                self->zPriority += 1;
                other->zPriority = self->zPriority;
                other->posX.i.hi = other->posX.i.hi - 0x20 + (Random() & 0x3F);
                if (self->facingLeft == 0) {
                    other->posX.i.hi -= 0xC;
                } else {
                    other->posX.i.hi += 0xC;
                }
                other->posY.i.hi += 0x18;
            }
            *(u16*)((u8*)self + 0x80) -= 1;
            if (*(u16*)((u8*)self + 0x80) != 0) {
                return;
            }
            self->animCurFrame = 0;
            v0 = self->step_s + 1;
            goto L80195844;

        case 3:
            D_us_80180848 |= 8;
            v0 = self->step_s + 1;
            goto L80195844;

        default:
            return;
    }

L80195870:
    FntPrint(D_us_80192E34, self->animCurFrame);
    if (g_pads_1_pressed & 0x80) {
        if (self->params != 0) {
            return;
        }
        self->animCurFrame += 1;
        self->params |= 1;
    } else {
        self->params = 0;
    }
    if (g_pads_1_pressed & 0x20) {
        if (self->step_s != 0) {
            return;
        }
        self->animCurFrame -= 1;
        self->step_s |= 1;
    } else {
        self->step_s = 0;
    }
    return;

L80195840:
    v0 += 1;
L80195844:
    self->step_s = v0;
    return;

L80195914:
    s2 = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
    if (self->velocityX < 0) {
        if (s2 < 0x68) {
            self->posX.i.hi = 0x68 - g_Tilemap.scrollX.i.hi;
        }
    } else {
        if (!(s2 < 0x1A1)) {
            self->posX.i.hi = 0x1A0 - g_Tilemap.scrollX.i.hi;
        }
    }
    {
        u8* rec = D_us_801807E4 + D_us_8018081C[self->animCurFrame] * 4;
        self->hitboxOffX = (s8)rec[0];
        self->hitboxOffY = (s8)rec[1];
        self->hitboxWidth = rec[2];
        self->hitboxHeight = rec[3];
    }
}

extern s32 D_us_80180848;
extern Entity g_Entities_160[];
extern u16 D_us_8018046C[];
extern u8* D_us_801807BC[];

void func_us_801959E0(Entity* self) {
    Entity* other;

    if (D_us_80180848 & 0x4) {
        DestroyEntity(self);
        return;
    }
    if (self->flags & 0x100) {
        other = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
        if (other != NULL) {
            func_us_80198D34(2, self, other);
            other->params = 1;
        }
        DestroyEntity(self);
        return;
    }
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_8018046C);
            self->velocityX = (self->facingLeft != 0) ? 0x60000 : 0xFFFA0000;
            /* fallthrough */
        case 1:
            MoveEntity();
            AnimateEntity(D_us_801807BC[self->params], self);
            break;
        default:
            break;
    }
}

extern u16 D_us_80180478[];
extern u16 D_us_801806E8[];
extern u8 D_us_801807C8[];
extern Entity g_Entities_128[];
extern s32 rcos(s32 angle);
extern s32 rsin(s32 angle);

void func_us_80195AF0(Entity* self) {
    Entity* other;
    s16 angle;

    if (D_us_80180848 & 0x4) {
        DestroyEntity(self);
        return;
    }
    if (self->step == 0) {
        InitializeEntity(D_us_80180478);
        self->drawFlags = 5;
        self->scaleX = 0x80;
        self->rotate = -0x280;
        angle = (self->facingLeft != 0) ? -0x180 : -0x680;
        self->palette = D_us_801806E8[self->params];
        self->velocityX = (rcos(angle) * 3) << 5;
        self->velocityY = (rsin(angle) * 3) << 5;
    }
    MoveEntity();
    if (!AnimateEntity(D_us_801807C8, self)) {
        DestroyEntity(self);
    }
    /* original reads pose(0x50)+poseTimer(0x52) as one 32-bit word for this check */
    if (*(s32*)((u8*)self + 0x50) == 8) {
        other = AllocEntity(g_Entities_128, (Entity*)((u8*)g_Entities_128 + 0x4680));
        if (other != NULL) {
            func_us_80198D34(0x1E, self, other);
            other->params = self->params;
            other->facingLeft = self->facingLeft;
        }
    }
}

extern u16 D_us_80180418[];
extern s16 PLAYER_posX_i_hi;
extern s32 PrimDecreaseBrightness(Primitive* prim, u8 arg1);

void func_us_80195C50(Entity* self) {
    Entity* other;
    Primitive* prim;
    Primitive* chain;
    Primitive* prim2;
    DR_ENV* dr_env;
    DRAWENV drawEnv;
    RECT clipRect;
    s32 idx;
    s32 i;
    s16 s1v, s3v;
    u32 a0, hi, v0, v1;
    s16 v0s;

    if (self->step >= 8) {
        return;
    }

    switch (self->step) {
        case 0: {
            s16 t2 = 0x110, t1 = 0x3F, t0 = 0x10, a3 = 0xF0, a2 = 0x40;
            s16 a1;

            InitializeEntity(D_us_80180418);
            self->flags |= 0x10000;
            idx = g_api_AllocPrimitives(PRIM_GT4, 7);
            if ((s16)idx == -1) {
                self->step = 0;
                return;
            }
            prim = &g_PrimBuf[(s16)idx];
            self->primIndex = (s16)idx;
            self->flags |= 0x800000;
            dr_env = g_api_func_800EDB08((POLY_GT4*)prim);
            if (dr_env == NULL) {
                goto L80195D68;
            }
            prim->type = PRIM_ENV;
            prim->drawMode = 8;
            chain = prim->next;
            g_api_func_800EDB08((POLY_GT4*)chain);
            if (chain == NULL) {
                goto L80195D68;
            }

            chain->type = PRIM_ENV;
            chain->drawMode = 8;
            chain = chain->next;
            *(Primitive**)((u8*)self + 0x7C) = chain;
            chain->type = 3;
            chain->b0 = 0;
            chain->g0 = 0;
            chain->r0 = 0;
            chain->r1 = chain->r0;
            chain->g1 = chain->g0;
            chain->b1 = chain->b0;
            chain->p1 = chain->type;
            chain->x3 = 0x42;
            chain->x1 = 0x42;
            chain->y3 = 0xB0;
            chain->y2 = 0xB0;
            chain->r2 = 0xFF;
            chain->p2 = 0;
            chain->g2 = 0;
            chain->r3 = chain->r2;
            chain->g3 = chain->g2;
            chain->b3 = chain->b2;
            chain->p3 = chain->p2;
            chain->clut = 0xF;
            chain->x2 = 0;
            chain->x0 = 0;
            chain->y1 = 0;
            chain->y0 = 0;
            chain->priority = 0xF;
            chain->drawMode = 0;
            chain = chain->next;

            for (i = 0, a1 = 0; i < 4; i++, a1++) {
                chain->tpage = t2;
                if (a1 & 1) {
                    chain->u2 = t1;
                    chain->u0 = t1;
                    chain->u3 = 0;
                    chain->u1 = 0;
                } else {
                    chain->u2 = 0;
                    chain->u0 = 0;
                    chain->u3 = t1;
                    chain->u1 = t1;
                }
                chain->x2 = a1 * 64;
                chain->x0 = a1 * 64;
                chain->priority = 0x13;
                chain->v1 = t0;
                chain->v0 = t0;
                chain->v3 = a3;
                chain->v2 = a3;
                chain->x3 = a2;
                chain->x1 = a2;
                chain->y1 = t0;
                chain->y0 = t0;
                chain->y3 = a3;
                chain->y2 = a3;
                chain->drawMode = 0;
                chain = chain->next;
                a2 += 0x40;
            }

            g_api_PlaySfx(0x7C5);
            s1v = PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi;
            if ((u32)(s1v - 0x41) < 0x17F) {
                self->step++;
                D_us_80180848 |= 1;
            }
            goto L80195F10;

        L80195D68:
            g_api_FreePrimitives((s16)idx);
            self->step = 0;
            self->flags &= ~0x800000;
            return;
        }

        case 1:
            goto L80195EC4;

        case 2:
            goto L80195F68;

        case 3:
            goto L80196034;

        case 4:
            goto L80196080;

        case 5:
            goto L801960D8;

        case 6:
            goto L801960F8;

        case 7:
            goto L80196188;

        default:
            return;
    }

L80195EC4:
    s1v = PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi;
    if ((u32)(s1v - 0x41) < 0x17F) {
        self->step++;
        D_us_80180848 |= 1;
    }

L80195F10:
    /* periodic trigger check; original uses a reciprocal-multiply idiom
     * (magic constant 0x58ED2309) whose exact divisor wasn't pinned down -
     * transcribed literally rather than guessed, flagged MEDIUM CONFIDENCE */
    a0 = g_Timer;
    hi = (u32)(((u64)a0 * 0x58ED2309ULL) >> 32);
    v0 = a0 - hi;
    v0 >>= 1;
    v1 = hi + v0;
    v1 >>= 6;
    v0 = v1 * 95;
    if (a0 != v0) {
        return;
    }
    g_api_PlaySfx(0x7C5);
    goto L801961D8;

L80195F68:
    a0 = g_Timer;
    hi = (u32)(((u64)a0 * 0x58ED2309ULL) >> 32);
    v0 = a0 - hi;
    v0 >>= 1;
    v1 = hi + v0;
    v1 >>= 6;
    v0 = v1 * 95;
    if (a0 == v0) {
        g_api_PlaySfx(0x7C5);
    }

    s1v = PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi;
    if ((u32)(s1v - 0x81) < 0xFF) {
        g_api_TimeAttackController(0xD, 2);
        stopMusicFlag = 1;
        currentMusicId = 0x31D;
        D_us_80180848 |= 2;
        self->step++;
    }
    return;

L80196034:
    if (!g_api_func_80131F68()) {
        g_api_PlaySfx((s16)currentMusicId);
        stopMusicFlag = 0;
        self->step++;
    }

L80196080:
    if (D_us_80180848 & 4) {
        g_api_TimeAttackController(0xD, 1);
        g_api_PlaySfx(0x90);
        currentMusicId = 0x319;
        v0 = self->step + 1;
        self->step = (s16)v0;
        return;
    }
    return;

L801960D8:
    if (D_us_80180848 & 8) {
        goto L801961C4;
    }
    return;

L801960F8:
    s1v = 0x100 - g_Tilemap.scrollX.i.hi;
    s3v = 0x80 - g_Tilemap.scrollY.i.hi;
    other = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
    if (other != NULL) {
        func_us_80198D34(0x21, self, other);
        other->posX.i.hi = s1v;
        other->posY.i.hi = s3v;
        other->params = 5;
        stopMusicFlag = 1;
        currentMusicId = 0x319;
        D_us_80180848 |= 0x10;
        self->step++;
    }
    /* fallthrough */

L80196188:
    if (!g_api_func_80131F68()) {
        g_api_PlaySfx((s16)currentMusicId);
        stopMusicFlag = 0;
    L801961C4:
        self->step++;
    }
    return;

L801961D8:
    prim = &g_PrimBuf[self->primIndex];
    dr_env = (DR_ENV*)LOW(prim->r1);
    drawEnv = g_CurrentBuffer->draw;
    drawEnv.isbg = 1;
    drawEnv.r0 = drawEnv.g0 = drawEnv.b0 = 0;
    clipRect.x = 0;
    clipRect.y = 0x100;
    clipRect.w = 0x7F;
    clipRect.h = 0xFF;
    drawEnv.clip = clipRect;
    drawEnv.ofs[0] = 0;
    drawEnv.ofs[1] = 0x100;
    SetDrawEnv(dr_env, &drawEnv);
    prim->priority = 0xE;
    prim->drawMode = 0;
    prim = prim->next;
    prim->priority = 0x11;
    prim->drawMode = 0x800;

    prim2 = *(Primitive**)((u8*)self + 0x7C);
    if (D_us_80180848 & 4) {
        self->rotate += 0x100;
        PrimDecreaseBrightness(prim2, 4);
    } else {
        self->rotate += 0x100;
        v0s = rcos(self->rotate);
        v0s = (v0s << 7) >> 12;
        if (v0s < 0) {
            v0s = -v0s;
        }
        prim2->g1 = (u8)v0s;
        prim2->r3 = prim2->r2;
        prim2->g3 = prim2->g2;
        prim2->b3 = prim2->b2;
        prim2->p3 = prim2->p2;
    }

    if (D_us_80180848 & 4) {
        return;
    }
    if (g_Timer & 3) {
        return;
    }
    other = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
    if (other != NULL) {
        func_us_80198CC0(0x20, other);
        other->posX.i.hi = (Random() & 0x3F) + 0xA;
        other->posY.i.hi = 0xB0;
        other->zPriority = 0x10;
        other->params = 0;
    }
}

extern u16 D_us_80180400[];
/* 12-byte records selected by (params & 0xF): u16 animSet, u16 unk5A,
 * u16 paletteBase (+0x2E0), u8 blendMode, pad, u32 ext80 (anim-data ptr). */
extern u8 D_us_801808A4[];

void func_us_801963D8(Entity* self) {
    u8* entry;
    s16 idx;

    if (self->step == 1) {
        goto L801964E0;
    }
    if (self->step < 2) {
        if (self->step == 0) {
            goto L8019642C;
        }
        return;
    }
    if (self->step == 2) {
        goto L80196554;
    }
    if (self->step == 3) {
        goto L80196588;
    }
    return;

L8019642C:
    InitializeEntity(D_us_80180400);
    idx = self->params & 0xF;
    entry = D_us_801808A4 + idx * 12;
    self->palette = *(u16*)(entry + 4) + 0x2E0;
    self->blendMode = *(u8*)(entry + 6);
    self->animSet = *(u16*)(entry + 0);
    self->unk5A = *(u16*)(entry + 2);
    *(u32*)((u8*)self + 0x80) = *(u32*)(entry + 8);
    self->step = idx + 1;
    if (self->params & 0xFF00) {
        self->zPriority = (u16)((self->params & 0xFF00) >> 8);
    }
    if (self->params & 0xF0) {
        self->palette = 0x819F;
        self->blendMode = 0x10;
        self->facingLeft = 1;
    }
    return;

L801964E0:
    if (self->step_s != 0) {
        goto L801965F0;
    }
    self->drawFlags = 8;
    self->opacity = 0x50;
    self->flags &= 0xF7FFFFFF;
    self->facingLeft = Random() & 1;
    self->velocityX = (Random() << 8) - 0x8000;
    self->velocityY = 0xFFFE8000; /* -0x18000 */
    *(s32*)((u8*)self + 0x8C) = -(Random() << 4) - 0x1000;
    self->step_s++;
    goto L801965F0;

L80196554:
    if (self->step_s == 0) {
        self->velocityY = -0x4000;
        self->step_s++;
    }
    MoveEntity();
    goto L8019661C;

L80196588:
    if (self->step_s != 0) {
        goto L801965F0;
    }
    self->drawFlags = 8;
    self->opacity = 0xC0;
    self->facingLeft = Random() & 1;
    self->velocityX = (Random() << 8) - 0x8000;
    self->velocityY = 0xFFFF4000; /* -0xC000 */
    *(s32*)((u8*)self + 0x8C) = -(Random() << 4) - 0x4000;
    self->step_s++;
    /* fallthrough */

L801965F0:
    MoveEntity();
    self->velocityY += *(s32*)((u8*)self + 0x8C);
    self->opacity -= 1;

L8019661C:
    if (!AnimateEntity(*(u8**)((u8*)self + 0x80), self)) {
        DestroyEntity(self);
    }
}
