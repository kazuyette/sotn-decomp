// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo0.h"

extern s16 PLAYER_posX_i_hi;
extern s16 PLAYER_posY_i_hi;
extern u16 PLAYER_step;
extern u16 PLAYER_facingLeft;
extern u16 PLAYER_zPriority;
extern u8 GetSideToPlayer(void); // st_common.h, not pulled in by this overlay
extern s32 rcos(s32 angle);
extern s32 rsin(s32 angle);
extern u8 D_us_801813D4[]; // per-part GT4 U/V byte table (8-byte records: u0,u1,u2,u3,v0,v1,v2,v3)
extern u16 D_us_80181404[]; // floor tile IDs, 8 entries per params value (first 4 / last 4 halves)

s32 func_us_801BB014(Entity* entity) {
    s16 dx = PLAYER_posX_i_hi - entity->posX.i.hi;
    s16 dy;

    if (dx < 0) {
        dx = -dx;
    }
    if (dx >= 0x19) {
        return 0;
    }

    dy = PLAYER_posY_i_hi - entity->posY.i.hi;
    if (dy < 0) {
        dy = -dy;
    }
    return dy < 0x21;
}

void func_us_801BB08C(Entity* self) {
    s16* ext84 = (s16*)((u8*)self + 0x84); // angle, driven +-0x20 per frame between 0x800/0x1000 or 0xC00
    s16* ext86 = (s16*)((u8*)self + 0x86); // side offset (+-4), set once at init

    if (self->step >= 6) goto L801BB7F0;

    switch (self->step) {
    case 0: {
        s16 primIndex;
        u8* tbl;
        Primitive* prim;
        s32 i;

        InitializeEntity(g_EInitCommon);
        self->animSet = 7;
        if (self->params & 0x1000) {
            self->animCurFrame = 1;
            self->animSet = 0;
        }
        self->facingLeft = 0;
        self->zPriority = PLAYER_zPriority - 0x20;
        self->posY.i.hi += 0x1F;
        *ext86 = (self->params & 0x100) ? -4 : 4;
        self->posX.i.hi += *ext86;

        primIndex = (s16)g_api_AllocPrimitives(PRIM_GT4, 3);
        self->primIndex = primIndex;
        if (primIndex == -1) {
            DestroyEntity(self);
            return;
        }

        tbl = D_us_801813D4;
        self->flags |= 0x800000;
        if (self->params & 0x1000) {
            tbl += 0x18;
        }

        i = 0;
        for (prim = &g_PrimBuf[self->primIndex]; prim != NULL;
             prim = (Primitive*)prim->next, tbl += 8, i++) {
            prim->u0 = tbl[0];
            prim->u1 = tbl[1];
            prim->u2 = tbl[2];
            prim->u3 = tbl[3];
            prim->v0 = tbl[4];
            prim->v1 = tbl[5];
            prim->v2 = tbl[6];
            prim->v3 = tbl[7];

            if (self->params & 0x1000) {
                prim->tpage = 0x12;
                prim->clut = 0x21B;
            } else {
                prim->tpage = 0x1F;
                prim->clut = 0x198;
            }

            prim->y1 = prim->y0 = self->posY.i.hi - 0x1F;
            prim->y3 = prim->y2 = self->posY.i.hi + 0x1F;
            if (i == 0) {
                prim->priority = PLAYER_zPriority - 0x20;
                // redundant re-write of y0/y1/y2/y3, present in the original asm
                prim->y1 = prim->y0 = self->posY.i.hi - 0x1F;
                prim->y3 = prim->y2 = self->posY.i.hi + 0x1F;
            }

            prim->drawMode = 6;
            prim->r0 = prim->g0 = prim->b0 = 0x7F;
            prim->r1 = prim->g1 = prim->b1 = 0x7F;
            prim->r2 = prim->g2 = prim->b2 = 0x7F;
            prim->r3 = 0x7F;
            if (i == 2) {
                if (!(self->params & 0x100)) {
                    prim->drawMode |= 8;
                }
            } else if (i == 1) {
                if (self->params & 0x100) {
                    prim->drawMode |= 8;
                }
            }
        }
        return;
    }

    case 1: {
        if (PLAYER_step != 1) goto L801BB7F0;
        if (PLAYER_facingLeft == GetSideToPlayer()) goto L801BB7F0;
        if (!func_us_801BB014(self)) goto L801BB7F0;

        if (self->primIndex != 0) {
            Primitive* prim = &g_PrimBuf[self->primIndex];
            s32 i;

            for (i = 0; prim != NULL; prim = (Primitive*)prim->next, i++) {
                if (i == 1) {
                    if (!(self->params & 0x100)) prim->drawMode &= ~8;
                } else if (i == 2) {
                    if (self->params & 0x100) prim->drawMode &= ~8;
                } else if (i == 0) {
                    prim->drawMode &= ~8;
                }
            }
        }

        self->animCurFrame = 0;
        g_Player.padSim = 0;
        g_Player.demo_timer = 2;
        g_api_PlaySfx(0x642);
        self->step++;
        goto L801BB7F0;
    }

    case 2: {
        g_Player.padSim = 0;
        g_Player.demo_timer = 0x18;
        if (self->params & 0x100) {
            *ext84 -= 0x20;
            if (*ext84 < 0x801) {
                *ext84 = 0x800;
            }
            if (*ext84 != 0x800) goto L801BB7F0;
        } else {
            *ext84 += 0x20;
            if (*ext84 >= 0x1000) {
                *ext84 = 0x1000;
            }
            if (*ext84 != 0x1000) goto L801BB7F0;
        }
        self->step++;
        goto L801BB7F0;
    }

    case 3: {
        if (g_Player.demo_timer >= 4) return;
        g_Player.padSim = (self->params & 0x100) ? 0x2000 : 0x8000;
        g_Player.demo_timer = 3;
        goto L801BB7F0;
    }

    case 4: {
        g_Player.padSim = (self->params & 0x100) ? 0x8000 : 0x2000;
        g_Player.demo_timer = 4;
        if (!func_us_801BB014(self)) goto L801BB7F0;
        g_api_PlaySfx(0x642);
        self->step++;
        g_Player.demo_timer = 0;
        goto L801BB7F0;
    }

    case 5: {
        g_Player.padSim = 0;
        g_Player.demo_timer = 4;
        if (self->params & 0x100) {
            *ext84 += 0x20;
            if (*ext84 < 0xC00) goto L801BB764;
            *ext84 = 0xC00;
        } else {
            *ext84 -= 0x20;
            if (*ext84 >= 0xC01) goto L801BB764;
            *ext84 = 0xC00;
        }
    L801BB764:
        if (*ext84 != 0xC00) goto L801BB7F0;

        if (self->primIndex != 0) {
            Primitive* prim = &g_PrimBuf[self->primIndex];
            s32 i;

            for (i = 0; prim != NULL; prim = (Primitive*)prim->next, i++) {
                if (i == 0) {
                    if (!(self->params & 0x100)) prim->drawMode |= 8;
                } else if (i == 1) {
                    if (self->params & 0x100) prim->drawMode |= 8;
                } else {
                    prim->drawMode |= 8;
                }
            }
        }
        self->step++;
        goto L801BB7F0;
    }
    }

L801BB7F0:
    if (self->step != 1) {
        g_api_func_8010E168(1, 0x20);
        g_api_func_8010DFF0(1, 1);
    }

    if (self->primIndex != 0) {
        Primitive* prim = &g_PrimBuf[self->primIndex];
        s32 angle = *ext84;
        s32 s4 = self->posX.i.hi - *ext86;
        bool flag1 = angle < 0xE01;
        s32 baseVal = (u16)(*ext84 & 0x3FF) >> 4;
        s32 s6 = baseVal;
        s32 s5 = baseVal + 0x3F;
        bool flag2 = angle < 0xA00;
        bool flag3 = angle < 0xA01;
        s32 s2;
        s32 s7 = 0;

        s4 += (self->params & 0x100) ? -1 : 1;

        for (s2 = 0; prim != NULL; prim = (Primitive*)prim->next, s2++) {
            if (prim->drawMode & 8) continue;

            if (!(self->params & 0x100)) {
                // non-ALT ring segment
                if (s2 == 0) {
                    s32 v0 = rcos(angle) >> 8;
                    v0 <<= 1;
                    v0 = s4 + v0;
                    s7 = v0;
                    prim->x2 = (s16)s7;
                    prim->x0 = (s16)s7;
                    {
                        s32 sinv = rsin(angle);
                        s32 v1 = (sinv >> 4) * 3;
                        v1 >>= 7;
                        v0 = s7 - v1;
                        prim->x3 = (s16)v0;
                        prim->x1 = (s16)v0;
                    }
                    if (angle >= 0xF81) {
                        s32 v = prim->x0 + 1;
                        prim->x3 = (s16)v;
                        prim->x1 = (s16)v;
                    }

                    if (!flag1) {
                        prim->u2 = 0xB2;
                        prim->u0 = 0xB2;
                        prim->u3 = 0xB6;
                        prim->u1 = 0xB6;
                        if (self->params & 0x1000) {
                            prim->u2 = 4;
                            prim->u0 = 4;
                            prim->u3 = 0xC;
                            prim->u1 = 0xC;
                        }
                    } else {
                        prim->u2 = 0xB1;
                        prim->u0 = 0xB1;
                        prim->u3 = 0xB7;
                        prim->u1 = 0xB7;
                        if (self->params & 0x1000) {
                            prim->u2 = 3;
                            prim->u0 = 3;
                            prim->u3 = 0xD;
                            prim->u1 = 0xD;
                        }
                    }

                    {
                        s32 v = (angle == 0x1000) ? 0x3F : (0x7F - s6);
                        prim->g1 = v;
                        prim->b1 = v;
                        prim->r1 = v;
                        prim->g3 = v;
                        prim->b3 = v;
                        prim->r3 = v;
                    }
                } else {
                    s32 v;
                    prim->x2 = (s16)s4;
                    prim->x0 = (s16)s4;
                    prim->x3 = (s16)s7;
                    prim->x1 = (s16)s7;
                    v = (angle == 0x1000) ? 0x3F : s6;
                    prim->g0 = v;
                    prim->b0 = v;
                    prim->r0 = v;
                    prim->g2 = v;
                    prim->b2 = v;
                    prim->r2 = v;
                }
            } else {
                // ALT ring segment (params & 0x100)
                if (s2 == 0) {
                    s32 v0 = rcos(angle) >> 8;
                    v0 <<= 1;
                    v0 = s4 + v0;
                    s7 = v0;
                    prim->x3 = (s16)s7;
                    prim->x1 = (s16)s7;
                    {
                        s32 sinv = rsin(angle);
                        s32 v1 = (sinv >> 4) * 3;
                        v1 >>= 7;
                        v0 = prim->x1 + v1;
                        prim->x2 = (s16)v0;
                        prim->x0 = (s16)v0;
                    }
                    if (angle < 0x880) {
                        s32 v = prim->x1 - 1;
                        prim->x2 = (s16)v;
                        prim->x0 = (s16)v;
                    }

                    if (flag2) {
                        prim->u2 = 0xB2;
                        prim->u0 = 0xB2;
                        prim->u3 = 0xB6;
                        prim->u1 = 0xB6;
                        if (self->params & 0x1000) {
                            prim->u2 = 4;
                            prim->u0 = 4;
                            prim->u3 = 0xC;
                            prim->u1 = 0xC;
                        }
                    }
                    if (!flag3) {
                        prim->u2 = 0xB1;
                        prim->u0 = 0xB1;
                        prim->u3 = 0xB7;
                        prim->u1 = 0xB7;
                        if (self->params & 0x1000) {
                            prim->u2 = 3;
                            prim->u0 = 3;
                            prim->u3 = 0xD;
                            prim->u1 = 0xD;
                        }
                    }

                    {
                        s32 v = (angle == 0x800) ? 0x7F : s5;
                        prim->g0 = v;
                        prim->b0 = v;
                        prim->r0 = v;
                        prim->g2 = v;
                        prim->b2 = v;
                        prim->r2 = v;
                    }
                } else {
                    s32 v = s7 - 1;
                    prim->x2 = (s16)v;
                    prim->x0 = (s16)v;
                    prim->x3 = (s16)s4;
                    prim->x1 = (s16)s4;
                    v = (angle == 0x800) ? 0x3F : (0x3F - baseVal);
                    prim->g1 = v;
                    prim->b1 = v;
                    prim->r1 = v;
                    prim->g3 = v;
                    prim->b3 = v;
                    prim->r3 = v;
                }
            }
        }
    }

    {
        u8 params = (u8)self->params;
        u16* tbl = &D_us_80181404[params * 8];
        s32 a1 = -0x18;
        s32 i;

        if (self->animCurFrame == 0) {
            tbl += 4;
        }
        for (i = 0; i < 4; i++, a1 += 0x10) {
            s32 row = (s32)(s16)(self->posY.i.hi + a1 + g_Tilemap.scrollY.i.hi) >> 4;
            s32 col = (s32)(s16)(self->posX.i.hi + g_Tilemap.scrollX.i.hi) >> 4;
            g_Tilemap.fg[col + row * (s32)g_Tilemap.hSize * 16] = *tbl++;
        }
    }
}
