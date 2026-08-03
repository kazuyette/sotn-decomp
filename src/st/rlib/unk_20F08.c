// SPDX-License-Identifier: AGPL-3.0-or-later
#include "rlib.h"

extern EInit g_EInitInteractable;
extern SVECTOR D_us_80180914;
extern SVECTOR D_us_8018091C;
extern SVECTOR D_us_80180924;
extern SVECTOR D_us_8018092C;
extern SVECTOR D_us_80180934;
extern SVECTOR D_us_8018093C;

void EntityBackgroundBookcase(Entity* self) {
    Primitive* prim;
    s32 primIndex;
    long p, flag;
    s16 x;
    long sxy;
    SVECTOR svector;
    VECTOR vector;
    MATRIX matrix;

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitInteractable);
        primIndex = g_api.AllocPrimitives(PRIM_GT4, 4);
        if (primIndex != -1) {
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            self->ext.prim = prim;
            while (prim != NULL) {
                prim->tpage = 0xF;
                prim->clut = 0x35;
                prim->u0 = prim->u2 = 130;
                prim->u1 = prim->u3 = 253;
                prim->v0 = prim->v1 = 50;
                prim->v2 = prim->v3 = 125;
                prim->priority = 0x40;
                prim->drawMode = DRAW_UNK02;
                prim = prim->next;
            }
        } else {
            DestroyEntity(self);
            return;
        }
        g_GpuBuffers[0].draw.r0 = 16;
        g_GpuBuffers[0].draw.g0 = 16;
        g_GpuBuffers[0].draw.b0 = 16;
        g_GpuBuffers[1].draw.r0 = 16;
        g_GpuBuffers[1].draw.g0 = 16;
        g_GpuBuffers[1].draw.b0 = 16;
        // fallthrough
    case 1:
        SetGeomScreen(1024);
        SetGeomOffset(128, self->posY.i.hi);
        RotMatrix(&D_us_8018091C, &matrix);
        vector.vx = self->posX.i.hi - 128;
        vector.vy = 0;
        vector.vz = 1024;
        TransMatrix(&matrix, &vector);
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        RotTransPers(&D_us_80180914, &sxy, &p, &flag);
        x = sxy & 0xFFFF;
        if (x > 0) {
            while (x > -64) {
                x -= 124;
            }
        } else {
            while (x < -128) {
                x += 124;
            }
        }
        prim = self->ext.prim;
        while (prim != NULL) {
            prim->x0 = prim->x2 = x;
            x += 124;
            prim->x1 = prim->x3 = x;
            prim->y0 = prim->y1 = self->posY.i.hi + 76;
            prim->y2 = prim->y3 = self->posY.i.hi;
            prim->drawMode = DRAW_UNK02;
            prim = prim->next;
            if (x > 256) {
                break;
            }
        }
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        break;
    }
}

void EntityBackgroundFurniture(Entity* self) {
    Primitive* prim;
    s32 primIndex;
    long p, flag;
    s16 x;
    long sxy;
    SVECTOR svector;
    VECTOR vector;
    MATRIX matrix;

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitInteractable);
        primIndex = g_api.AllocPrimitives(PRIM_GT4, 4);
        if (primIndex != -1) {
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            self->ext.prim = prim;
            while (prim != NULL) {
                prim->tpage = 0xF;
                prim->clut = 0x34;
                prim->u0 = prim->u2 = 128;
                prim->u1 = prim->u3 = 248;
                prim->v0 = prim->v1 = 0;
                prim->v2 = prim->v3 = 28;
                prim->priority = 0x50;
                prim->drawMode = DRAW_UNK02;
                prim = prim->next;
            }
        } else {
            DestroyEntity(self);
            return;
        }
        // fallthrough
    case 1:
        SetGeomScreen(1024);
        SetGeomOffset(128, self->posY.i.hi);
        RotMatrix(&D_us_8018092C, &matrix);
        vector.vx = self->posX.i.hi - 128;
        vector.vy = 0;
        vector.vz = 1024;
        TransMatrix(&matrix, &vector);
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        RotTransPers(&D_us_80180924, &sxy, &p, &flag);
        x = sxy & 0xFFFF;
        if (x > 0) {
            while (x > -64) {
                x -= 120;
            }
        } else {
            while (x < -128) {
                x += 120;
            }
        }
        prim = self->ext.prim;
        while (prim != NULL) {
            prim->x0 = prim->x2 = x;
            x += 120;
            prim->x1 = prim->x3 = x;
            prim->y0 = prim->y1 = self->posY.i.hi + 28;
            prim->y2 = prim->y3 = self->posY.i.hi;
            prim->drawMode = DRAW_UNK02;
            prim = prim->next;
            if (x > 256) {
                break;
            }
        }
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        break;
    }
}

void EntityBackgroundShadow(Entity* self) {
    Primitive *prim, *last;
    s32 primIndex;
    long p, flag;
    s16 x;
    long sxy;
    SVECTOR svector;
    VECTOR vector;
    MATRIX matrix;

    switch (self->step) {
    case 0:
        InitializeEntity(g_EInitInteractable);
        primIndex = g_api.func_800EDB58(PRIM_TILE_ALT, 10);
        if (primIndex != -1) {
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            prim = &g_PrimBuf[primIndex];
            self->ext.libraryShadow.prim = prim;
            while (prim != NULL) {
                prim->u0 = 24;
                prim->v0 = 64;
                prim->r0 = prim->g0 = prim->b0 = 0;
                prim->priority = 0x60;
                prim->drawMode = DRAW_TPAGE | DRAW_UNK02 | DRAW_TRANSP;
                last = prim;
                prim = prim->next;
            }
            prim = last;
            self->ext.libraryShadow.lastPrim = prim;
            prim->u0 = 255;
            prim->v0 = 48;
            prim->x0 = 0;
            prim->y0 = self->posY.i.hi + 112;
            prim->r0 = prim->g0 = prim->b0 = 0;
            prim->priority = 0x60;
            prim->drawMode = DRAW_TPAGE | DRAW_UNK02 | DRAW_TRANSP;
        } else {
            DestroyEntity(self);
            return;
        }
        // fallthrough
    case 1:
        SetGeomScreen(1024);
        SetGeomOffset(128, self->posY.i.hi);
        RotMatrix(&D_us_8018093C, &matrix);
        vector.vx = self->posX.i.hi - 128;
        vector.vy = 0;
        vector.vz = 1024;
        TransMatrix(&matrix, &vector);
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        RotTransPers(&D_us_80180934, &sxy, &p, &flag);
        x = sxy & 0xFFFF;
        if (x > 0) {
            while (x > -64) {
                x -= 56;
            }
        } else {
            while (x < -128) {
                x += 56;
            }
        }
        prim = self->ext.libraryShadow.prim;
        while (prim != NULL) {
            prim->x0 = x;
            x += 56;
            prim->y0 = self->posY.i.hi;
            prim->drawMode = DRAW_TPAGE | DRAW_UNK02 | DRAW_TRANSP;
            prim = prim->next;
            if (x > 256) {
                break;
            }
        }
        while (prim != NULL) {
            prim->drawMode = DRAW_HIDE;
            prim = prim->next;
        }
        prim = self->ext.libraryShadow.lastPrim;
        prim->u0 = 255;
        prim->v0 = 48;
        prim->x0 = 0;
        prim->y0 = self->posY.i.hi + 64;
        prim->r0 = prim->g0 = prim->b0 = 0;
        prim->priority = 0x60;
        prim->drawMode = DRAW_TPAGE | DRAW_UNK02 | DRAW_TRANSP;
        break;
    }
}

INCLUDE_ASM("st/rlib/nonmatchings/unk_20F08", func_us_801AFA80_from_lib);

INCLUDE_ASM("st/rlib/nonmatchings/unk_20F08", func_us_801AE8E8_from_lib);
