#include "stage.h"

typedef struct {
    u8 pad0[8];     // ext+0x0-0x7 (unused by this function)
    s16 speed;      // ext+0x8
    u8 pad1[2];     // ext+0xA-0xB (unused by this function)
    s8 dir;         // ext+0xC
    s8 countdown;   // ext+0xD
} ET_801A5E4C;

extern void InitializeEntity(u16 params[]);
extern void DestroyEntity(Entity* entity);
extern void AnimateEntity(u8* anim, Entity* self);
extern void* AllocEntity(Entity* pool, Entity* poolEnd);
extern void ReplaceBreakableWithItemDrop(Entity* entity);
extern void PreventEntityFromRespawning(Entity* entity);
// func_us_801A8D7C/func_us_801A8DF0 are now known to be
// CreateEntityFromCurrentEntity/CreateEntityFromEntity (create_entity.c was
// migrated to the shared create_entity.h; the symbols.us.bobo3.txt addresses
// for these functions were off by 4 bytes, which made them look like
// separate 1-instruction "orphan" functions until fixed). Aliased here
// rather than updating every call site/removing the old signature, since
// this file predates the shared-header migration and doesn't include it.
#define func_us_801A8D7C CreateEntityFromCurrentEntity
#define func_us_801A8DF0 CreateEntityFromEntity
extern void func_us_801A8D7C(s32 type, Entity* newEntity); // boss/bo3/create_entity.c, still a stub (2-arg sibling of func_us_801A8DF0)
extern void func_us_801A8DF0(s32 type, Entity* parent, Entity* newEntity); // boss/bo3/create_entity.c, still a stub
extern Entity g_Entities_224[];
extern Entity g_Entities_160[];

extern u16 D_us_80180534[];  // InitializeEntity params (step 0)
extern u8 D_us_801807B4[];   // per-subtype blendMode
extern u8 D_us_80180760[];   // per-subtype hitboxHeight
extern u8* D_us_80180738[];  // per-subtype animation data pointer
extern s16 D_us_8018078C[];  // per-subtype animSet
extern s16 D_us_801807A0[];  // per-subtype unk5A
extern u16 D_us_80180778[];  // per-subtype palette
extern s16 D_us_801807C0[];  // per-subtype hitboxOffY
extern u8 D_us_8018076C[];   // per-subtype value applied to a spawned type-2 "reward" entity's params

// Breakable-object spawn/init dispatcher. subtype = params bits 12-15 (params >> 0xC).
// step 0: one-time init of blendMode/hitboxHeight/animSet/unk5A/palette/hitboxOffY from
// parallel per-subtype tables, plus zPriority set relative to g_zEntityCenter (same
// "zPriority = g_zEntityCenter - 0x14" idiom already confirmed in zone2's
// func_us_801AB53C). Every frame: animates via a per-subtype anim pointer table.
// If hitParams is set (object was just broken/hit), spawns a companion "reward" entity
// of type 2 (params set from a per-subtype table), then dispatches by subtype: 9 additionally
// spawns a type-0x43 entity before falling into the same item-drop path as 2/3; 7/8 spawn
// a type-0xC child entity (params = self->params & 0x1FF for 7, literal 0x29 for 8) and
// destroy self directly (no item drop); anything else (default) plays SFX 0x634 then
// still drops an item, same as 9.
// Confidence: high on control flow and Entity fields (self->step/params/hitParams
// confirmed via game.h, g_zEntityCenter cross-checked against game.h's real offset
// annotations rather than trusting m2c's raw "unk10" guess); table contents read
// directly from asm/us/boss/bo3/data/0.data.s, not guessed. func_us_801A8D7C's exact
// purpose is unconfirmed (create_entity.c dispatcher stub, same family as
// func_us_801A8DF0).
void func_us_8019CEEC(Entity* self)
{
    u16 subtype = (u16) self->params >> 0xC;

    if (self->step == 0) {
        InitializeEntity(D_us_80180534);
        self->zPriority = (u16)(g_unkGraphicsStruct.g_zEntityCenter - 0x14);
        self->blendMode = D_us_801807B4[subtype];
        self->hitboxHeight = D_us_80180760[subtype];
        self->animSet = D_us_8018078C[subtype];
        self->unk5A = D_us_801807A0[subtype];
        self->palette = D_us_80180778[subtype];
        self->hitboxOffY = D_us_801807C0[subtype];
    }

    AnimateEntity(D_us_80180738[subtype], self);

    if (self->hitParams != 0) {
        Entity* reward = AllocEntity(g_Entities_224, (Entity*)((u8*) g_Entities_224 + 0x1780));
        if (reward != NULL) {
            func_us_801A8D7C(2, reward);
            reward->params = (s16) D_us_8018076C[subtype];
        }

        switch (subtype) {
            case 9: {
                Entity* extra = AllocEntity(g_Entities_160, (Entity*)((u8*) g_Entities_160 + 0x1780));
                if (extra != NULL) {
                    func_us_801A8D7C(0x43, extra);
                    extra->params = 0x100;
                }
                g_api_PlaySfx(0x61D);
                ReplaceBreakableWithItemDrop(self);
                break;
            }

            case 2:
            case 3:
                ReplaceBreakableWithItemDrop(self);
                break;

            case 7: {
                Entity* child;
                g_api_PlaySfx(0x61D);
                child = AllocEntity(g_Entities_160, (Entity*)((u8*) g_Entities_160 + 0x1780));
                if (child != NULL) {
                    func_us_801A8DF0(0xC, self, child);
                    child->params = self->params & 0x1FF;
                }
                PreventEntityFromRespawning(self);
                DestroyEntity(self);
                return;
            }

            case 8: {
                Entity* child;
                g_api_PlaySfx(0x61D);
                child = AllocEntity(g_Entities_160, (Entity*)((u8*) g_Entities_160 + 0x1780));
                if (child != NULL) {
                    func_us_801A8DF0(0xC, self, child);
                    child->params = 0x29;
                }
                PreventEntityFromRespawning(self);
                DestroyEntity(self);
                return;
            }

            default:
                g_api_PlaySfx(0x634);
                ReplaceBreakableWithItemDrop(self);
                break;
        }
    }
}

extern u16 D_us_80180564[]; // InitializeEntity params (step 0, func_us_8019D160)

// Entity init: standard "first frame" setup (sprite/anim/priority), Scylla-specific
// (not a shared pickup like the ones above). unk54 sentinel (-0x7FF5) forces a fresh
// animSet on the first frame, same convention seen throughout bo1.
// Confidence: high (field names match Entity layout exactly; table contents unverified).
void func_us_8019D160(Entity* self)
{
    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = -0x7FF5;
        self->unk5A = 0x5B;
        self->palette = 0x228;
        self->animCurFrame = 0x15;
        self->zPriority = 0x6A;
        self->step = 0x100;
    }
}

typedef struct {
    s16 leftX;    // +0x0: world-space left edge
    s16 width;    // +0x2
    s16 topY;     // +0x4: world-space top edge
    s16 pad;      // +0x6: unused by this function
    s16 phaseInc; // +0x8: per-record UV scroll speed, 0 = static (no scroll)
} D_us_80180830_Entry;
extern D_us_80180830_Entry D_us_80180830[];

typedef struct {
    u8 uBase; // +0x0: base U offset for the repeating (mod 126) texture
    u8 vRow;  // +0x1: V row (2-texel-tall strip: vRow, vRow+1)
    u8 pad[2];
} D_us_8018080C_Entry;
extern D_us_8018080C_Entry D_us_8018080C[];

typedef struct {
    u8 pad[4];         // ext+0x0-0x3 (0x7C-0x7F): `prim`, accessed via self->ext.prim directly
    u16 unusedCounter; // ext+0x4 (0x80): set to 4 on step 0, never read again in this function
    u16 phaseAccum;    // ext+0x6 (0x82): persistent UV scroll phase, advanced by phaseInc
} ET_8019D1D4;

extern u16 D_us_80180564[];
extern u8 D_us_80180824[]; // anim table for AnimateEntity

/*
 * Clipped, tiled decorative wall segment renderer. Draws up to 16 GT4
 * quads covering a run of world-space rectangles (D_us_80180830, stride
 * 10 bytes: leftX/width/topY/pad/phaseInc), starting at record index
 * (params & 0xFF) for (params >> 8) records, each clipped against the
 * current camera viewport and horizontally tiled with a repeating (mod
 * 126) texture pattern -- same wrap-around idiom as the other "cascade/
 * repeating strip" functions in this file (e.g. func_us_8019FDC4). Each
 * record can carry its own per-call UV scroll speed (phaseInc); records
 * with phaseInc==0 use a static (non-scrolling) texture alignment
 * instead. The V row (and the row below it) come from
 * D_us_8018080C[animCurFrame], i.e. the anim system selects which
 * 2-texel-tall strip of the tilesheet is used, not just a fixed row.
 * Confirmed directly against the asm: each drawn quad is only 1 screen
 * pixel tall (y1 = topY-scrollY, y2 = that+1) -- a thin trim/line effect,
 * not a full wall texture.
 * ext+0x80 is set to a fixed 4 on step 0 and never read again here --
 * presumably consumed by another, not-yet-decompiled bo3 function that
 * shares this entity type.
 * Confidence: high on control flow, clipping, and the mod-126 tiling (all
 * directly confirmed against the asm, including m2c's own div/mod idiom
 * recognition for the mod-126 arithmetic); medium on the exact meaning of
 * ext+0x80 and of the table's unused offset+6 gap.
 */
void func_us_8019D1D4(Entity* self)
{
    ET_8019D1D4* ext = (ET_8019D1D4*)&self->ext;
    Primitive* prim;
    s32 slot;
    s32 count;
    D_us_80180830_Entry* e;
    D_us_8018080C_Entry* frame;
    s16 viewLeft, viewRight;
    u8 vRow0, vRow1;
    s16 left, right, top, phaseInc, topScreen, bottomScreen, u, x;
    s32 width;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        ext->unusedCounter = 4;
        slot = g_api_AllocPrimitives(4, 0x10);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[slot];
        self->primIndex = slot;
        self->ext.prim = prim;
        ext->phaseAccum = 0;
        self->flags |= 0x800000;
        while (prim != NULL) {
            prim->tpage = 0xF;
            prim->clut = 0x5E;
            prim->priority = 0x9C;
            prim->drawMode = 8;
            prim = prim->next;
        }
    }

    prim = self->ext.prim;
    AnimateEntity(D_us_80180824, self);

    count = self->params >> 8;
    e = &D_us_80180830[(u8)self->params];
    frame = &D_us_8018080C[self->animCurFrame];
    vRow0 = frame->vRow;
    vRow1 = vRow0 + 1;
    viewLeft = g_Tilemap.scrollX.i.hi - 0x10;
    viewRight = g_Tilemap.scrollX.i.hi + 0x110;

    for (; count > 0; count--, e++) {
        left = e->leftX;
        right = left + e->width;
        if (viewLeft >= right || viewRight < left) {
            continue;
        }
        top = e->topY;
        if (top < g_Tilemap.scrollY.i.hi - 4 || g_Tilemap.scrollY.i.hi + 0xE0 < top) {
            continue;
        }
        phaseInc = e->phaseInc;
        topScreen = top - g_Tilemap.scrollY.i.hi;
        bottomScreen = topScreen + 1;

        if (left < viewLeft) {
            left = viewLeft;
        }
        if (viewRight < right) {
            right = viewRight;
        }
        if (phaseInc != 0) {
            ext->phaseAccum += phaseInc;
            u = (left - (s16)ext->phaseAccum) % 126;
        } else {
            u = left % 126;
        }
        width = right - left;
        x = (left - 0x10) - viewLeft;
        u += frame->uBase;

        do {
            s16 remaining = 126 - (u - frame->uBase);
            s32 quadWidth = (width < remaining) ? width : remaining;
            s16 uEnd = u + quadWidth;
            s16 xEnd = x + quadWidth;

            prim->u2 = prim->u0 = (u8)u;
            prim->u3 = prim->u1 = (u8)uEnd;
            prim->v1 = prim->v0 = vRow0;
            prim->v3 = prim->v2 = vRow1;
            prim->x2 = prim->x0 = x;
            prim->x3 = prim->x1 = xEnd;
            prim->y1 = prim->y0 = topScreen;
            prim->y3 = prim->y2 = bottomScreen;
            prim->drawMode = 0x13;
            prim = prim->next;

            x = xEnd;
            width -= quadWidth;
            u = uEnd;
        } while (width != 0);
    }

    while (prim != NULL) {
        prim->drawMode = 8;
        prim = prim->next;
    }
}

typedef struct {
    s16 leftX;
    s16 width;
    s16 topY;
    s16 bottomY;
} D_us_8018092C_Entry;
extern D_us_8018092C_Entry D_us_8018092C[];

typedef struct {
    u8 pad[4];         // ext+0x0-0x3 (0x7C-0x7F): `prim`, accessed via self->ext.prim directly
    u16 unusedCounter; // ext+0x4 (0x80): set to 4 on step 0, never read again in this function
} ET_8019D51C;

/*
 * Clipped 2D-box tiled decorative wall segment renderer (TILE primitives,
 * up to 16), sibling of func_us_8019D1D4 but clipping BOTH X and Y (that
 * one only clipped X, always drawing a fixed 1px-tall strip) -- table
 * D_us_8018092C (stride 8: leftX/width/topY/bottomY), starting at record
 * index (params & 0xFF) for (params >> 8) records. Each record is clipped
 * against the camera viewport (viewport padding here is 0x10/0xF0, versus
 * 4/0xE0 for func_us_8019D1D4 -- different visual context), then drawn as
 * one or more TILE quads no wider/taller than 0xFF (TILE's u0/v0 width/
 * height fields are u8): height is clamped once per record (never tiled/
 * repeated if it exceeds 0xFF -- any excess height is simply not drawn),
 * while width chunks in a loop similar to func_us_8019D1D4's mod-126 wrap,
 * just without the wraparound (each TILE quad is a flat color, no texture
 * to tile).
 * ext+0x80 is set to a fixed 4 on step 0 and never read again here -- same
 * as func_us_8019D1D4, presumably consumed elsewhere.
 * Confidence: high -- control flow and Primitive fields directly confirmed
 * against the asm (m2c flagged the flowgraph as irreducible/gotos-only,
 * but it fully corresponds to ordinary nested-if + chunking-loop control
 * flow once traced by hand).
 */
void func_us_8019D51C(Entity* self)
{
    ET_8019D51C* ext = (ET_8019D51C*)&self->ext;
    Primitive* prim;
    s32 slot;
    s32 count;
    D_us_8018092C_Entry* e;
    s16 viewLeft, viewRight, viewTop, viewBottom;
    s16 left, right, top, bottom, y, x;
    s32 width, height, chunkWidth;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        ext->unusedCounter = 4;
        slot = g_api_AllocPrimitives(1, 0x10);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[slot];
        self->primIndex = slot;
        self->ext.prim = prim;
        self->flags |= 0x800000;
        while (prim != NULL) {
            prim->r0 = 0;
            prim->g0 = 0x10;
            prim->b0 = 0x20;
            prim->priority = 0x9B;
            prim = prim->next;
        }
    }

    prim = self->ext.prim;
    e = &D_us_8018092C[(u8)self->params];
    count = self->params >> 8;
    viewLeft = g_Tilemap.scrollX.i.hi - 0x10;
    viewTop = g_Tilemap.scrollY.i.hi - 0x10;
    viewRight = g_Tilemap.scrollX.i.hi + 0x110;
    viewBottom = g_Tilemap.scrollY.i.hi + 0xF0;

    for (; count > 0; count--, e++) {
        left = e->leftX;
        right = left + e->width;
        if (viewLeft >= right || viewRight < left) {
            continue;
        }
        top = e->topY;
        bottom = e->bottomY;
        if (viewTop >= bottom || viewBottom < top) {
            continue;
        }

        if (left < viewLeft) {
            left = viewLeft;
        }
        width = right - left;
        if (viewRight < right) {
            width = viewRight - left;
        }
        x = (left - 0x10) - viewLeft;
        if (top < viewTop) {
            top = viewTop;
        }
        height = bottom - top;
        if (viewBottom < bottom) {
            height = viewBottom - top;
        }
        if (height >= 0x100) {
            height = 0xFF;
        }
        y = (top - 0x10) - viewTop;

        do {
            chunkWidth = (width < 0x100) ? width : 0xFF;

            prim->x0 = x;
            prim->u0 = (u8)chunkWidth;
            prim->v0 = (u8)height;
            prim->y0 = y;
            prim->drawMode = 0x13;
            prim = prim->next;

            x += chunkWidth;
            width -= chunkWidth;
        } while (width != 0);
    }

    while (prim != NULL) {
        prim->drawMode = 8;
        prim = prim->next;
    }
}

typedef struct {
    s16 leftX;
    s16 rightX;
    s16 topY;
    s16 bottomY;
} D_us_80180A1C_Entry;
extern D_us_80180A1C_Entry D_us_80180A1C[];

typedef struct {
    u8 uBase;
    u8 vBase;
} D_us_80180820_t;
extern D_us_80180820_t D_us_80180820;

typedef struct {
    u8 pad[4];         // ext+0x0-0x3 (0x7C-0x7F): `prim`, accessed via self->ext.prim directly
    u16 unusedCounter; // ext+0x4 (0x80): set to 4 on step 0, never read again in this function
} ET_8019D768;

/*
 * Third and last of the "clipping" trio (see func_us_8019D1D4/8019D51C).
 * This one draws a shimmering underwater-light-shaft style GT4 quad set
 * (up to 16) and ALSO tints the ambient background buffers every call
 * (g_GpuBuffers[0/1].draw.r0/g0/b0 = a fixed dark blue, same mechanism as
 * the library/clouds background effects elsewhere in the codebase, e.g.
 * src/st/lib/e_library_bg.c).
 *
 * Walks D_us_80180A1C (stride 8: leftX/rightX/topY/bottomY, direct edges
 * this time rather than left+width) starting at record index
 * (params & 0xFF) for (params >> 8) records. Each record is first clipped
 * against the camera viewport, THEN clipped AGAIN against a second,
 * slower-scrolling "inner window" derived from scrollY/4 (rounded toward
 * zero) -- a parallax aperture effect, confirmed directly against the asm
 * (two independent >>2 idioms, one for the horizontal U phase and one for
 * this vertical inner window, both using the classic pre-shift +3-if-
 * negative rounding adjustment). U wraps mod 126 and V wraps mod 70 (both
 * m2c-recognized div-by-constant idioms, kept as `%` rather than
 * re-derived from the raw multiply/shift sequences), offset by a single
 * shared base (D_us_80180820), not per-record. Confirmed directly against
 * the asm: unlike func_us_8019D1D4, U resets to the shared base for every
 * width chunk after the first (it does NOT continue from where the
 * previous chunk's wrap ended) -- height/V, by contrast, are computed
 * once per record and reused unchanged for every chunk. Each quad's color
 * is a symmetric triangle-wave pulse of the low 6 bits of g_Timer (a
 * 256-frame shimmer cycle: r/b ramp from -0x80 to -0x41 and back, g fixed
 * at 0x80) -- confirmed directly against the asm.
 * ext+0x80 is set to a fixed 4 on step 0 and never read again here -- same
 * as func_us_8019D1D4/8019D51C, presumably consumed elsewhere.
 * Confidence: high on control flow, clipping (both stages), and the mod
 * idioms (all directly confirmed against the asm); the exact in-game
 * purpose (parallax light shafts through the water, given the ambient
 * tint + shimmer) is inferred, not confirmed.
 */
void func_us_8019D768(Entity* self)
{
    ET_8019D768* ext = (ET_8019D768*)&self->ext;
    Primitive* prim;
    s32 slot;
    s32 count;
    D_us_80180A1C_Entry* e;
    s16 viewLeft, viewRight, viewBottom;
    s16 scrollY;
    s16 left, right, top, bottom;
    s16 x, topScreen, bottomScreen;
    s32 width, height;
    s16 adjViewLeft, adjScrollY;
    s32 scrollYq;
    s16 innerTopThresh, innerBottomThresh;
    s16 u, v;
    u32 timer;
    s16 pulse;
    u8 color;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        ext->unusedCounter = 4;
        slot = g_api_AllocPrimitives(4, 0x10);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[slot];
        self->primIndex = slot;
        self->ext.prim = prim;
        self->flags |= 0x800000;
        while (prim != NULL) {
            prim->tpage = 0xF;
            prim->clut = 0x85;
            prim->priority = 0x18;
            prim->drawMode = 8;
            prim = prim->next;
        }
    }

    g_GpuBuffers[0].draw.r0 = 8;
    g_GpuBuffers[0].draw.g0 = 8;
    g_GpuBuffers[0].draw.b0 = 0x10;
    g_GpuBuffers[1].draw.r0 = 8;
    g_GpuBuffers[1].draw.g0 = 8;
    g_GpuBuffers[1].draw.b0 = 0x10;

    prim = self->ext.prim;
    count = self->params >> 8;
    e = &D_us_80180A1C[(u8)self->params];
    scrollY = g_Tilemap.scrollY.i.hi;
    viewBottom = scrollY + 0xE0;
    viewLeft = g_Tilemap.scrollX.i.hi - 0x10;
    viewRight = g_Tilemap.scrollX.i.hi + 0x110;

    for (; count > 0; count--, e++) {
        left = e->leftX;
        right = e->rightX;
        if (viewLeft >= right || viewRight < left) {
            continue;
        }
        top = e->topY;
        bottom = e->bottomY;
        if (!(scrollY < bottom) || viewBottom < top) {
            continue;
        }

        if (left < viewLeft) {
            left = viewLeft;
        }
        adjViewLeft = viewLeft;
        if (viewRight < right) {
            right = viewRight;
        }
        width = right - left;
        x = (left - 0x10) - viewLeft;
        if (adjViewLeft < 0) {
            adjViewLeft += 3;
        }

        if (top < scrollY) {
            top = scrollY;
        }
        adjScrollY = scrollY;
        if (viewBottom < bottom) {
            bottom = viewBottom;
        }
        if (adjScrollY < 0) {
            adjScrollY += 3;
        }
        scrollYq = adjScrollY >> 2;
        innerTopThresh = 0x58 - scrollYq;
        innerBottomThresh = viewBottom + 0x46;
        bottomScreen = bottom - scrollY;
        topScreen = top - scrollY;

        if (innerTopThresh >= bottomScreen || innerBottomThresh < topScreen) {
            continue;
        }

        if (topScreen < innerTopThresh) {
            topScreen = innerTopThresh;
        }
        if (innerBottomThresh < bottomScreen) {
            bottomScreen = innerBottomThresh;
        }
        height = bottomScreen - topScreen;
        u = ((adjViewLeft >> 2) + x) % 126;
        u += D_us_80180820.uBase;

        do {
            s32 remainU = 0x7E - (u - D_us_80180820.uBase);
            s32 chunkW = (width < remainU) ? width : remainU;
            s16 xEnd = x + chunkW;
            s32 remainV;
            s32 chunkH;
            s16 yEnd;

            prim->u2 = prim->u0 = (u8)u;
            u += chunkW;
            prim->u3 = prim->u1 = (u8)u;
            prim->x2 = prim->x0 = x;
            prim->x3 = prim->x1 = xEnd;
            u = D_us_80180820.uBase;
            width -= chunkW;
            x = xEnd;

            v = ((scrollYq - 0x58 + topScreen) % 70) + D_us_80180820.vBase;
            remainV = 0x46 - (v - D_us_80180820.vBase);
            chunkH = (height < remainV) ? height : remainV;
            yEnd = topScreen + chunkH;
            prim->v1 = prim->v0 = (u8)v;
            prim->v3 = prim->v2 = (u8)(v + chunkH);
            prim->y3 = prim->y2 = yEnd;
            prim->y1 = prim->y0 = topScreen;
            prim->drawMode = 4;

            timer = g_Timer;
            if (timer & 0x80) {
                if (timer & 0x40) {
                    pulse = 0x3F - (timer & 0x3F);
                } else {
                    pulse = timer & 0x3F;
                }
            } else {
                pulse = 0;
            }
            color = (u8)(pulse - 0x80);
            prim->b3 = prim->b2 = prim->b1 = prim->b0 = color;
            prim->r3 = prim->r2 = prim->r1 = prim->r0 = color;
            prim->g3 = prim->g2 = prim->g1 = prim->g0 = 0x80;

            prim = prim->next;
        } while (width != 0);
    }

    while (prim != NULL) {
        prim->drawMode = 8;
        prim = prim->next;
    }
}

extern u16 D_us_80180564[];
extern s16 D_us_80180BC4[];

typedef struct { u8* firstPrim; } ET_8019DBB8; // ext+0x0 (only field used)

// Parallax decoration wall, scrolling at HALF camera speed (the classic
// "(v + sign-adjust) >> 1" GCC round-toward-zero div-by-2 idiom, confirmed
// and translated directly as C's `/ 2`, which the same compiler re-derives
// into the identical instruction sequence). Walks a table of {x, y, flags}
// triples (D_us_80180BC4, stride 3 s16 = 6 bytes) and, for each visible
// entry (screen-space X in [-0x14, 0x100), Y in a fixed vertical band),
// activates the next Primitive in the chain as a quad positioned there,
// mirrored horizontally/vertically per two flag bits (flags&1, flags&2) --
// the standard "shared tile art, flipped for variety" idiom. Once an entry's
// X is off-screen to the right, the table is assumed exhausted (sorted
// ascending) and remaining primitives are deactivated. The extra second
// argument m2c showed on DestroyEntity is a leftover register value (real
// signature is DestroyEntity(Entity*), confirmed via include/stage.h) and is
// dropped here, consistent with the same artifact class already fixed
// elsewhere in this file.
// Confidence: medium-high -- the half-scroll parallax rate and the mirror
// flag bits are both unambiguous from the arithmetic/branch shape; the
// table's exact source data is inferred from usage, not cross-checked
// against another table already named in this codebase.
void func_us_8019DBB8(Entity* self)
{
    ET_8019DBB8* ext = (ET_8019DBB8*)&self->ext;
    u8* p;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 0xA);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x1A) = 0xE;
            *(u16*)(p + 0xE) = 0x84;
            *(u8*)(p + 0x24) = 0xE9;
            *(u8*)(p + 0xC) = 0xE9;
            *(u8*)(p + 0x30) = 0xFD;
            *(u8*)(p + 0x18) = 0xFD;
            *(u8*)(p + 0x19) = 1;
            *(u8*)(p + 0xD) = 1;
            *(u8*)(p + 0x31) = 0x7F;
            *(u8*)(p + 0x25) = 0x7F;
            *(u16*)(p + 0x26) = 0x1C;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    if (g_Tilemap.scrollY.i.hi < 0xB0) {
        s16* entry = D_us_80180BC4;
        s32 halfScrollX = g_Tilemap.scrollX.i.hi / 2;
        s32 halfScrollY = g_Tilemap.scrollY.i.hi / 2;
        while (1) {
            s16 x = entry[0] - halfScrollX;
            s16 y = entry[1] - halfScrollY;
            u16 flags = (u16)entry[2];
            s16 yOther = y + 0x7E;
            entry += 3;
            if (x < -0x14) {
                continue;
            }
            if (x >= 0x100) {
                break;
            }
            if (yOther >= 0 && y < 0xE0) {
                s16 cornerA, cornerB, cornerC, cornerD;
                if (flags & 1) {
                    cornerA = x + 0x14;
                    cornerB = x;
                } else {
                    cornerA = x;
                    cornerB = x + 0x14;
                }
                if (flags & 2) {
                    cornerC = y;
                    cornerD = yOther;
                } else {
                    cornerC = yOther;
                    cornerD = y;
                }
                *(s16*)(p + 0x20) = cornerA;
                *(s16*)(p + 0x8) = cornerA;
                *(s16*)(p + 0x2C) = cornerB;
                *(s16*)(p + 0x14) = cornerB;
                *(s16*)(p + 0x16) = cornerC;
                *(s16*)(p + 0xA) = cornerC;
                *(s16*)(p + 0x2E) = cornerD;
                *(s16*)(p + 0x22) = cornerD;
                *(u16*)(p + 0x32) = 0;
                p = *(u8**)(p + 0);
            }
        }
    }
    if (p != NULL) {
        do {
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        } while (p != NULL);
    }
}

extern u16 D_us_80180564[];
extern s16 D_us_80180BF4[];

typedef struct {
    u8* firstPrim;    // ext+0x0
    u8 pad4[4];       // ext+0x4 (unused by this function)
    s32 frameCounter; // ext+0x8
} ET_8019DE08;

// Scrolling two-row horizontal tile strip (4 slots, AllocPrimitives(4,0x10)),
// same family as func_us_8019FDC4: walks a position table
// (D_us_80180BF4, stride 2 s16, only every-other entry read) and, for each
// visible column, draws a quad for the top row and -- if the visible band
// splits across a screen seam (temp_t6 != 0) -- a second quad for the
// remainder as a bottom row. Verified directly against the .s-derived m2c
// output for its control flow (no primitive-chain NULL guard inside the
// per-column draw loop -- preserved as-is, matching the original rather than
// defensively "fixed"). The divisions/modulo are the compiler's own
// round-toward-zero div-by-8 and mod-32 idiom (explicit "+7"/"+0x1F"
// pre-adjustment before the arithmetic shift for negative operands),
// translated directly via C's / and % (already round-toward-zero), not
// approximated.
// Confidence: medium -- overall shape (two-row split tile strip) and the
// div/mod translation are solid; individual scratch-field roles (uv bytes)
// are kept as raw offsets.
void func_us_8019DE08(Entity* self)
{
    ET_8019DE08* ext = (ET_8019DE08*)&self->ext;
    u8* p;
    s16* col;
    s32 frame;
    s32 q8;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 0x10);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        ext->frameCounter = 0;
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x1A) = 0xE;
            *(u8*)(p + 0x24) = 0xFC;
            *(u8*)(p + 0xC) = 0xFC;
            *(u8*)(p + 0x30) = 0xFE;
            *(u8*)(p + 0x18) = 0xFE;
            *(u16*)(p + 0x26) = 0x1A;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    col = D_us_80180BF4;
    p = ext->firstPrim;
    frame = ext->frameCounter + 1;
    ext->frameCounter = frame;
    if (frame >= 0xE) {
        ext->frameCounter = 0;
    }
    q8 = ((g_Tilemap.scrollX.i.hi * 3) / 8) & 0xFF;

    {
        s32 topBase = 0x50 - g_Tilemap.scrollY.i.hi;
        s32 bottomLimit = 0xB0 - g_Tilemap.scrollY.i.hi;
        if (topBase < 0) {
            topBase = 0;
        }
        if (bottomLimit >= 0) {
            s32 q8y = (g_Tilemap.scrollY.i.hi * 3) / 8;
            s32 phase = (q8y + topBase) % 32;
            s32 topH = bottomLimit - topBase;
            s32 clippedTopH = topH;
            s32 bottomH;
            s16 uvTop, uvTopEnd, uvBottom;
            s16 yTopBottom;
            s16 yBottomBottom;
            s16 primE;

            if ((phase + topH) >= 0x61) {
                clippedTopH = 0x60 - phase;
            }
            bottomH = topH - clippedTopH;
            yTopBottom = (s16)(clippedTopH + topBase);
            uvTop = (s16)(phase - 0x7C);
            uvTopEnd = (s16)(uvTop + clippedTopH);
            uvBottom = (s16)(bottomH - 0x7C);
            yBottomBottom = (s16)(yTopBottom + bottomH);
            primE = (s16)(ext->frameCounter + 0x90);

            while (1) {
                s16 x = *col - q8;
                col += 2;
                if (x < -2) {
                    continue;
                }
                if (x >= 0x100) {
                    break;
                }
                {
                    s16 xEnd = x + 2;
                    *(u16*)(p + 0xE) = (u16)primE;
                    *(u8*)(p + 0x19) = (u8)uvTop;
                    *(u8*)(p + 0xD) = (u8)uvTop;
                    *(u8*)(p + 0x31) = (u8)uvTopEnd;
                    *(u8*)(p + 0x25) = (u8)uvTopEnd;
                    *(s16*)(p + 0x20) = x;
                    *(s16*)(p + 0x8) = x;
                    *(s16*)(p + 0x2C) = xEnd;
                    *(s16*)(p + 0x14) = xEnd;
                    *(s16*)(p + 0x16) = (s16)topBase;
                    *(s16*)(p + 0xA) = (s16)topBase;
                    *(s16*)(p + 0x2E) = yTopBottom;
                    *(s16*)(p + 0x22) = yTopBottom;
                    *(u16*)(p + 0x32) = 0x71;
                    p = *(u8**)(p + 0);
                    if (bottomH != 0) {
                        *(u16*)(p + 0xE) = (u16)primE;
                        *(u8*)(p + 0x19) = 0x84;
                        *(u8*)(p + 0xD) = 0x84;
                        *(u8*)(p + 0x31) = (u8)uvBottom;
                        *(u8*)(p + 0x25) = (u8)uvBottom;
                        *(s16*)(p + 0x20) = x;
                        *(s16*)(p + 0x8) = x;
                        *(s16*)(p + 0x2C) = xEnd;
                        *(s16*)(p + 0x14) = xEnd;
                        *(s16*)(p + 0x16) = yTopBottom;
                        *(s16*)(p + 0xA) = yTopBottom;
                        *(s16*)(p + 0x2E) = yBottomBottom;
                        *(s16*)(p + 0x22) = yBottomBottom;
                        *(u16*)(p + 0x32) = 0x71;
                        p = *(u8**)(p + 0);
                    }
                }
            }
        }
    }
    if (p != NULL) {
        do {
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        } while (p != NULL);
    }
}

extern s32 D_us_8018054C;

typedef struct {
    u8* firstPrim;    // ext+0x0
    u8 pad4[4];       // ext+0x4 (unused by this function)
    s32 frameCounter; // ext+0x8
} ET_8019E0D0;

// Vertical wall-strip renderer, same two-segment split idiom as
// func_us_8019DE08 but oriented vertically (split against a Y-scroll-modulo
// seam rather than horizontal scroll), with two texture variants selected by
// the entity's own `params` (checked once at entity-init time for which
// clut/tpage to use, and again every frame for which UV/width constants to
// draw with -- both reads of the same field, kept as `variant` here since
// params doesn't change after spawn).
// Confidence: medium -- overall two-segment split shape and the mod-32
// phase computation match the established idiom exactly (translated
// directly via C's %); the two texture-variant constant sets are transcribed
// as literal per-branch values rather than named, since their visual meaning
// isn't determinable from arithmetic alone.
void func_us_8019E0D0(Entity* self)
{
    ET_8019E0D0* ext = (ET_8019E0D0*)&self->ext;
    u16 variant = self->params;
    u8* p;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_8018054C);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 2);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0xC0800000;
        while (p != NULL) {
            if (variant != 0) {
                *(u16*)(p + 0x1A) = 0xF;
                *(u8*)(p + 0x24) = 0x82;
                *(u8*)(p + 0xC) = 0x82;
                *(u8*)(p + 0x30) = 0x9D;
                *(u8*)(p + 0x18) = 0x9D;
            } else {
                *(u16*)(p + 0x1A) = 0xE;
                *(u8*)(p + 0x24) = 0xE9;
                *(u8*)(p + 0xC) = 0xE9;
                *(u8*)(p + 0x30) = 0xF7;
                *(u8*)(p + 0x18) = 0xF7;
            }
            *(u16*)(p + 0x26) = 0x62;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    {
        s32 frame = ext->frameCounter + 1;
        ext->frameCounter = frame;
        if (frame >= 0xE) {
            ext->frameCounter = 0;
        }
    }
    {
        s16 heightWanted = self->posY.i.hi;
        if (heightWanted >= 0) {
            s32 phase = g_Tilemap.scrollY.i.hi % 32;
            s16 primE = (s16)(ext->frameCounter + 0xB0);
            s32 clippedH = heightWanted;
            s32 remainH;

            if ((phase + heightWanted) >= 0x61) {
                clippedH = 0x60 - phase;
            }
            remainH = heightWanted - clippedH;
            *(u16*)(p + 0xE) = (u16)primE;

            {
                s16 posX = self->posX.i.hi;
                s16 xLeft, xRight;
                s8 uv0, uv1;

                if (variant != 0) {
                    uv0 = (s8)(phase + 4);
                    xLeft = posX - 0xD;
                    xRight = posX + 0xE;
                } else {
                    uv0 = (s8)(phase - 0x7C);
                    xLeft = posX - 7;
                    xRight = posX + 7;
                }
                uv1 = (s8)(uv0 + clippedH);
                *(u8*)(p + 0x19) = (u8)uv0;
                *(u8*)(p + 0xD) = (u8)uv0;
                *(u16*)(p + 0x32) = 0x73;
                *(s16*)(p + 0x2C) = xRight;
                *(s16*)(p + 0x14) = xRight;
                *(s16*)(p + 0x20) = xLeft;
                *(s16*)(p + 0x8) = xLeft;
                *(s16*)(p + 0x16) = 0;
                *(s16*)(p + 0xA) = 0;
                *(s16*)(p + 0x2E) = (s16)clippedH;
                *(s16*)(p + 0x22) = (s16)clippedH;
                *(u8*)(p + 0x31) = (u8)uv1;
                *(u8*)(p + 0x25) = (u8)uv1;
                p = *(u8**)(p + 0);

                if (remainH != 0) {
                    s16 xRight2;
                    s8 uv2, uv3;
                    *(u16*)(p + 0xE) = (u16)primE;
                    if (variant != 0) {
                        uv3 = (s8)(remainH + 4);
                        xRight2 = xLeft + 0x1B;
                        uv2 = 4;
                    } else {
                        uv3 = (s8)(remainH - 0x7C);
                        xRight2 = xLeft + 0xE;
                        uv2 = (s8)0x84;
                    }
                    *(u8*)(p + 0x19) = (u8)uv2;
                    *(u8*)(p + 0xD) = (u8)uv2;
                    *(u8*)(p + 0x31) = (u8)uv3;
                    *(u8*)(p + 0x25) = (u8)uv3;
                    *(s16*)(p + 0x20) = xLeft;
                    *(s16*)(p + 0x8) = xLeft;
                    *(s16*)(p + 0x2C) = xRight2;
                    *(s16*)(p + 0x14) = xRight2;
                    *(u16*)(p + 0x32) = 0x73;
                    {
                        s16 yBottom = (s16)(clippedH + remainH);
                        *(s16*)(p + 0x16) = (s16)clippedH;
                        *(s16*)(p + 0xA) = (s16)clippedH;
                        *(s16*)(p + 0x2E) = yBottom;
                        *(s16*)(p + 0x22) = yBottom;
                    }
                    p = *(u8**)(p + 0);
                }
            }
        }
    }
    if (p != NULL) {
        do {
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        } while (p != NULL);
    }
}

typedef struct {
    Primitive* prim; // ext+0x0: head of the 4-quad GT4 chain allocated at step 0
    u8 pad4[4];       // ext+0x4-0x7 (unused by this function)
    s32 animFrame;    // ext+0x8: cycles 0-13, added to 0xA0 for the clut index
                       // (palette-cycling animation on the scrolling texture)
} ET_8019E398;
extern u16 D_us_80180564[];
extern s16 PLAYER_posX_i_hi;
extern s16 PLAYER_posY_i_hi;

// Scrolling two-texture-page vertical strip effect (quake/rumble visual,
// e.g. Scylla's tidal-wave or tremor attack): allocates 4 GT4 quads once
// (step 0) and every call re-tiles them to cover a vertical band of the
// screen that scrolls with g_Tilemap, wrapping across two texture pages
// (tpage 0xE for the first ~0xFE texels, tpage 0xF for the remainder via a
// mod-126 wrap) since the source texture is taller than one page. The sfx
// volume/pan (sfxVol/sfxPan) use the standard player-relative positional
// audio formula seen elsewhere in this file. Any quads left unused once the
// vertical band is fully covered get drawMode=8 (hidden) so stale prims
// from a taller previous band don't linger.
// Confidence: high on control flow and Primitive fields (m2c already
// resolved these against primitive.h); the exact texture/tile semantics of
// the tpage/u/v magic constants are inferred from usage only.
void func_us_8019E398(Entity* self)
{
    ET_8019E398* ext = (ET_8019E398*) &self->ext;
    s16 scrollY = g_Tilemap.scrollY.i.hi;
    s16 scrollX = g_Tilemap.scrollX.i.hi;
    s32 sfxVol;
    s32 sfxPan;
    s32 panBase;
    s16 y;
    s16 yBudget;
    s16 yStep;
    s16 xLeft;
    s16 xRightA;
    s16 xRightB;
    Primitive* prim;

    sfxVol = scrollY + PLAYER_posY_i_hi;
    if (sfxVol < 0) {
        sfxVol += 0x1F;
    }
    sfxVol = (sfxVol >> 5) + 0x58;
    if (sfxVol >= 0x80) {
        sfxVol = 0x80;
    }

    panBase = scrollX + PLAYER_posX_i_hi;
    sfxPan = panBase - 0xC8;
    if (sfxPan < 0) {
        sfxPan = panBase - 0xA9;
    }
    sfxPan = -(sfxPan >> 5);
    if (sfxPan < -8) {
        sfxPan = -8;
    } else if (sfxPan >= 9) {
        sfxPan = 8;
    }

    if (self->step == 0) {
        s16 primStart;

        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        primStart = g_api_AllocPrimitives(PRIM_GT4, 4);
        if (primStart == -1) {
            DestroyEntity(self);
            return;
        }
        g_api_PlaySfxVolPan(0x797, sfxVol, sfxPan);
        prim = &g_PrimBuf[primStart];
        ext->animFrame = 0;
        self->primIndex = (s32) primStart;
        ext->prim = prim;
        self->flags |= 0x800000;
        while (prim != NULL) {
            prim->priority = 0x9C;
            prim->drawMode = 8;
            prim = prim->next;
        }
    }

    g_api_SetVolumeCommand22_23(sfxVol, sfxPan);
    ext->animFrame++;
    if (ext->animFrame >= 0xE) {
        ext->animFrame = 0;
    }

    y = 0xB0 - scrollY;
    if (y < 0) {
        y = 0;
    }
    yBudget = 0x5B0 - scrollY;
    if (yBudget >= 0xF1) {
        yBudget = 0xF0;
    }
    yBudget -= y;

    xLeft = 0xC0 - scrollX;
    xRightA = 0x120 - scrollX; // texture-page-0xE variant
    xRightB = 0xF1 - scrollX;  // texture-page-0xF (wrapped) variant

    prim = ext->prim;
    do {
        s16 v;
        s16 vRemaining;
        s16 yTop = y;

        if ((y + scrollY) - 0xB0 < 0xFE) {
            s16 texelRow = (y + scrollY) - 0xAF;

            prim->tpage = 0xE;
            prim->u3 = prim->u1 = 0xE7;
            prim->u2 = prim->u0 = 0x87;
            prim->v1 = prim->v0 = texelRow;
            if ((y + scrollY) - 0xB0 + yBudget >= 0xFF) {
                yStep = 0xFE - ((y + scrollY) - 0xB0);
                v = 0xFF;
            } else {
                v = (y + scrollY) - 0xB0 + yBudget + 1;
                yStep = yBudget;
            }
            prim->v3 = prim->v2 = v;
            prim->x2 = prim->x0 = xLeft;
            prim->x3 = prim->x1 = xRightA;
        } else {
            s16 wrapRow = ((y + scrollY) - 0x1AE) % 126;

            prim->tpage = 0xF;
            prim->u2 = prim->u0 = 0xCC;
            prim->u3 = prim->u1 = 0xFD;
            vRemaining = wrapRow + 1;
            prim->v1 = prim->v0 = vRemaining;
            if (wrapRow + yBudget >= 0x7F) {
                yStep = 0x7E - wrapRow;
                v = 0x7F;
            } else {
                v = wrapRow + yBudget + 1;
                yStep = yBudget;
            }
            prim->v3 = prim->v2 = v;
            prim->x2 = prim->x0 = xLeft;
            prim->x3 = prim->x1 = xRightB;
        }

        y += yStep;
        yBudget -= yStep;
        prim->y1 = prim->y0 = yTop;
        prim->y3 = prim->y2 = y;
        prim->clut = ext->animFrame + 0xA0;
        prim->drawMode = 0x13;
        prim = prim->next;
    } while (yBudget != 0);

    while (prim != NULL) {
        prim->drawMode = 8;
        prim = prim->next;
    }
}

// Horizontal color-gradient "vignette" overlay, drawn only once the camera
// has scrolled deep enough (scrollY.i.hi >= 0x301): three quads spanning a
// fixed vertical band (var_a3..var_a2, clamped/wrapped against the same
// mod-8 scroll-phase pattern as elsewhere) and tiling the screen width in
// three horizontal bands anchored at world X 0x20/0x80/0x120/0x180. The left
// and right bands fade their outer vertex colors to 0 (transparent/black)
// while the middle band stays fully colored on both edges -- a vignette that
// fades to black at the left/right screen edges with a solid-color band in
// the middle. The top/bottom edge colors themselves (temp_t1_3/temp_t3_2)
// alternate between two slightly different multiply-then-div-by-10 ratios
// (19/10 vs 18/10) depending on frame parity, for a subtle flicker -- the
// classic GCC MULT_HI(x,0x66666667) signed-div-by-10 idiom, translated
// directly via C's `/`.
// Confidence: medium-high -- the vignette structure (per-vertex color
// pattern across the three quads) and the div-by-10/div-by-40 arithmetic are
// unambiguous from the field values and branch shape; the poly type 0x437
// and the exact color semantics (likely an additive water-current tint) are
// inferred, not cross-checked against another named constant.
typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019E72C; // ext+0x0, ext+0x4

void func_us_8019E72C(Entity* self)
{
    ET_8019E72C* ext = (ET_8019E72C*)&self->ext;
    u8* p;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(3, 3);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x26) = 0x9E;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    ext->frameCounter = (u16)(ext->frameCounter + 1);

    if (g_Tilemap.scrollY.i.hi >= 0x301) {
        s16 yTop = 0x3E0 - g_Tilemap.scrollY.i.hi;
        s16 yBottom = 0x5E0 - g_Tilemap.scrollY.i.hi;
        s32 phase8 = g_Tilemap.scrollY.i.hi & 7;
        s32 wTop, wBottom;
        s8 colorTop, colorBottom;
        s16 xMid0, xMid1, xLeft0, xRight1;

        if (yTop < -0x10) {
            yTop = -0x10 - phase8;
        }
        wTop = yTop + g_Tilemap.scrollY.i.hi;
        if (yBottom >= 0xF1) {
            yBottom = (-phase8 & 7) + 0xF0;
            wTop = yTop + g_Tilemap.scrollY.i.hi;
        }
        wTop -= 0x3E0;
        wBottom = (yBottom + g_Tilemap.scrollY.i.hi) - 0x3E0;

        if (ext->frameCounter & 1) {
            colorTop = (s8)((wTop * 0x13) / 10);
            colorBottom = (s8)((wBottom * 0x13) / 40);
        } else {
            colorTop = (s8)((wTop * 0x12) / 10);
            colorBottom = (s8)((wBottom * 0x12) / 40);
        }

        xMid0 = 0x80 - g_Tilemap.scrollX.i.hi;
        xMid1 = 0x120 - g_Tilemap.scrollX.i.hi;
        xLeft0 = 0x20 - g_Tilemap.scrollX.i.hi;
        xRight1 = 0x180 - g_Tilemap.scrollX.i.hi;

        /* middle band: solid color on both edges */
        *(s8*)(p + 0x11) = colorTop;
        *(s8*)(p + 5) = colorTop;
        *(s8*)(p + 0x12) = colorTop;
        *(s8*)(p + 6) = colorTop;
        *(s8*)(p + 0x10) = colorTop;
        *(s8*)(p + 4) = colorTop;
        *(s8*)(p + 0x29) = colorBottom;
        *(s8*)(p + 0x1D) = colorBottom;
        *(s8*)(p + 0x2A) = colorBottom;
        *(s8*)(p + 0x1E) = colorBottom;
        *(s8*)(p + 0x28) = colorBottom;
        *(s8*)(p + 0x1C) = colorBottom;
        *(s16*)(p + 0x20) = xMid0;
        *(s16*)(p + 8) = xMid0;
        *(s16*)(p + 0x2C) = xMid1;
        *(s16*)(p + 0x14) = xMid1;
        *(s16*)(p + 0x16) = yTop;
        *(s16*)(p + 0xA) = yTop;
        *(s16*)(p + 0x2E) = yBottom;
        *(s16*)(p + 0x22) = yBottom;
        *(u16*)(p + 0x32) = 0x437;
        p = *(u8**)(p + 0);

        /* left band: fades to 0 on the left edge */
        {
            s16 xLeftEnd = xMid0;
            *(s8*)(p + 0x1D) = 0;
            *(s8*)(p + 5) = 0;
            *(s8*)(p + 0x1E) = 0;
            *(s8*)(p + 6) = 0;
            *(s8*)(p + 0x1C) = 0;
            *(s8*)(p + 4) = 0;
            *(s8*)(p + 0x11) = colorTop;
            *(s8*)(p + 0x12) = colorTop;
            *(s8*)(p + 0x10) = colorTop;
            *(s8*)(p + 0x29) = colorBottom;
            *(s8*)(p + 0x2A) = colorBottom;
            *(s8*)(p + 0x28) = colorBottom;
            *(s16*)(p + 0x20) = xLeft0;
            *(s16*)(p + 8) = xLeft0;
            *(s16*)(p + 0x2C) = xLeftEnd;
            *(s16*)(p + 0x14) = xLeftEnd;
            *(s16*)(p + 0x16) = yTop;
            *(s16*)(p + 0xA) = yTop;
            *(s16*)(p + 0x2E) = yBottom;
            *(s16*)(p + 0x22) = yBottom;
            *(u16*)(p + 0x32) = 0x437;
            p = *(u8**)(p + 0);
        }

        /* right band: fades to 0 on the right edge */
        {
            s16 xRightStart = xMid1;
            *(s8*)(p + 5) = colorTop;
            *(s8*)(p + 6) = colorTop;
            *(s8*)(p + 4) = colorTop;
            *(s8*)(p + 0x1D) = colorBottom;
            *(s8*)(p + 0x1E) = colorBottom;
            *(s8*)(p + 0x1C) = colorBottom;
            *(s8*)(p + 0x29) = 0;
            *(s8*)(p + 0x2A) = 0;
            *(s8*)(p + 0x28) = 0;
            *(s8*)(p + 0x11) = 0;
            *(s8*)(p + 0x12) = 0;
            *(s8*)(p + 0x10) = 0;
            *(s16*)(p + 0x20) = xRightStart;
            *(s16*)(p + 8) = xRightStart;
            *(s16*)(p + 0x2C) = xRight1;
            *(s16*)(p + 0x14) = xRight1;
            *(s16*)(p + 0x16) = yTop;
            *(s16*)(p + 0xA) = yTop;
            *(s16*)(p + 0x2E) = yBottom;
            *(s16*)(p + 0x22) = yBottom;
            *(u16*)(p + 0x32) = 0x437;
            p = *(u8**)(p + 0);
        }
    }
    if (p != NULL) {
        do {
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        } while (p != NULL);
    }
}

extern u16 D_us_80180564[]; // InitializeEntity params (step 0, shared with several other functions in this file)

// Underwater bubble particles. Allocates a chain of Primitives (drawMode==8
// marks a slot "inactive"). Once the camera has scrolled far enough down
// (scrollY >= 0x4D1, i.e. near/below the water line), tries up to 4 times per
// frame to activate an inactive slot as a new bubble with a randomized
// position/rise-speed/drift, reusing the Primitive's own paint fields
// (x0/y0/u0/v0/etc, here addressed as raw offsets since they're being used as
// physics scratch space rather than actual paint data) to store velocity and
// a countdown lifetime. Every frame, active bubbles rise (position -= a decay
// term each axis) and are deactivated once their lifetime countdown reaches
// zero. Same "abuse Primitive's numeric fields as physics scratch storage"
// idiom already documented for bo1's debris-trail effects.
// Confidence: medium -- the linked-list walk and activate/deactivate control
// flow are unambiguous; the exact per-field physics roles (which offset is
// velocity vs. decel vs. lifetime) are inferred from the arithmetic shape
// and kept as raw offsets rather than invented field names, to avoid
// asserting more precision than the evidence supports.
typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019EA54; // ext+0x0, ext+0x4

void func_us_8019EA54(Entity* self)
{
    ET_8019EA54* ext = (ET_8019EA54*)&self->ext;
    u8* prim;
    u8* p;
    s32 i;
    s32 spawnBase;
    s32 rnd;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(1, 0x80);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = (u8*)prim;
        self->flags = self->flags | 0x800000;
        p = prim;
        while (p != NULL) {
            *(u16*)(p + 0x26) = 0x6C;   // priority
            *(u16*)(p + 0x32) = 8;      // drawMode (inactive marker)
            p = *(u8**)(p + 0);         // next
        }
    }
    p = ext->firstPrim;
    ext->frameCounter = ext->frameCounter + 1;
    if (g_Tilemap.scrollY.i.hi >= 0x4D1) {
        spawnBase = 0x5B0 - g_Tilemap.scrollY.i.hi;
        for (i = 0; i < 4; i++) {
            u8* q = p;
            while (q != NULL) {
                if (*(u16*)(q + 0x32) == 8) {
                    *(u8*)(q + 5) = 0x80;
                    *(u8*)(q + 6) = 0x80;
                    *(u8*)(q + 4) = 0x80;
                    rnd = Random();
                    *(s16*)(q + 8) = (s16)((((rnd % 11) * 4) - (g_Tilemap.scrollX.i.hi - 0xBF)) + i);
                    *(s16*)(q + 0xA) = (s16)(spawnBase + ((Random() & 7) - 4));
                    *(s32*)(q + 0x14) = 0;
                    *(s32*)(q + 0x10) = 0;
                    *(u8*)(q + 0xD) = 2;
                    *(u8*)(q + 0xC) = 2;
                    *(s32*)(q + 0x1C) = 0x40000;
                    *(s32*)(q + 0x18) = 0;
                    *(s32*)(q + 0x2C) = 0x4000 - (Random() & 0xFF0);
                    *(s32*)(q + 0x28) = (Random() & 0x1F00) - 0xF80;
                    *(u16*)(q + 0xE) = 0x20;
                    *(u16*)(q + 0x32) = 2;
                    break;
                }
                q = *(u8**)(q + 0);
            }
        }
    }
    p = ext->firstPrim;
    while (p != NULL) {
        if (*(u16*)(p + 0x32) != 8) {
            s32 velX = *(s32*)(p + 0x1C);
            u16 lifetime;
            *(u16*)(p + 0x16) = *(u16*)(p + 0xA);
            *(u16*)(p + 0x12) = *(u16*)(p + 8);
            lifetime = *(u16*)(p + 0xE) - 1;
            *(u16*)(p + 0xE) = lifetime;
            *(s32*)(p + 0x1C) = *(s32*)(p + 0x1C) - *(s32*)(p + 0x2C);
            *(s32*)(p + 0x10) = *(s32*)(p + 0x10) + *(s32*)(p + 0x18);
            *(s32*)(p + 0x14) = *(s32*)(p + 0x14) + velX;
            *(s32*)(p + 0x18) = *(s32*)(p + 0x18) - *(s32*)(p + 0x28);
            *(u16*)(p + 8) = *(u16*)(p + 0x12);
            *(u16*)(p + 0xA) = *(u16*)(p + 0x16);
            if (lifetime == 0) {
                *(u16*)(p + 0x32) = 8;
            }
        }
        p = *(u8**)(p + 0);
    }
}

extern u16 D_us_80180564[];
extern s16 D_us_80180C0C[];

typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019ED3C; // ext+0x0, ext+0x4

// Same "activate at most one inactive slot per frame" rising particle effect
// as func_us_8019FAD8 (4 slots, size 0x20), gated here on the camera having
// scrolled far enough down (scrollY.i.hi >= 0x481, i.e. near/below the water
// line -- same threshold family as func_us_8019EA54's bubbles), spawning
// near the bottom of that band. X position read from a 14-entry phase table
// (D_us_80180C0C[frameCounter]) relative to camera scrollX (not the entity's
// own X, unlike func_us_8019FAD8), plus a rand()&3-9 jitter. Fade/rise tail
// loop is identical to func_us_8019FAD8, including the same real
// leftover-rand-seed carry-over between the spawn and fade passes.
// Confidence: medium -- Entity/Primitive-chain conventions match the
// established idiom exactly; D_us_80180C0C's exact contents are inferred
// from usage.
void func_us_8019ED3C(Entity* self)
{
    ET_8019ED3C* ext = (ET_8019ED3C*)&self->ext;
    u8* p;
    s16 randSeed;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 0x20);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0xE) = 0x161;
            *(u16*)(p + 0x1A) = 0x1A;
            *(u8*)(p + 0x24) = 0;
            *(u8*)(p + 0xC) = 0;
            *(u8*)(p + 0x30) = 0x1E;
            *(u8*)(p + 0x18) = 0x1E;
            *(u8*)(p + 0x19) = 0x60;
            *(u8*)(p + 0xD) = 0x60;
            *(u8*)(p + 0x31) = 0x7C;
            *(u8*)(p + 0x25) = 0x7C;
            *(u16*)(p + 0x26) = 0x9D;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    randSeed = g_Tilemap.scrollY.i.hi;
    {
        s32 frame = ext->frameCounter + 1;
        ext->frameCounter = (u16)frame;
        if ((s16)frame >= 0xD) {
            ext->frameCounter = 0;
        }
    }

    if (randSeed >= 0x481) {
        s16 spawnY = 0x5B0 - randSeed;
        if (p != NULL) {
            u8* q = p;
            randSeed = 8;
            while (1) {
                if (*(u16*)(q + 0x32) != 8) {
                    q = *(u8**)(q + 0);
                    if (q == NULL) {
                        break;
                    }
                    continue;
                }
                {
                    s32 spawnX;
                    s16 x2;
                    s32 jitter = (Random(8) & 3) - 9;
                    *(s16*)(q + 0x2E) = spawnY;
                    *(s16*)(q + 0x22) = spawnY;
                    *(s16*)(q + 0x16) = spawnY;
                    *(s16*)(q + 0xA) = spawnY;
                    *(u8*)(q + 0x12) = 0xFF;
                    *(u8*)(q + 0x11) = 0xFF;
                    *(u8*)(q + 0x10) = 0xFF;
                    *(u8*)(q + 6) = 0xFF;
                    *(u8*)(q + 5) = 0xFF;
                    *(u8*)(q + 4) = 0xFF;
                    *(u8*)(q + 0x2A) = 0x80;
                    *(u8*)(q + 0x29) = 0x80;
                    *(u8*)(q + 0x28) = 0x80;
                    *(u8*)(q + 0x1E) = 0x80;
                    *(u8*)(q + 0x1D) = 0x80;
                    *(u8*)(q + 0x1C) = 0x80;
                    *(u8*)(q + 0x13) = 0;
                    spawnX = ((D_us_80180C0C[(u16)ext->frameCounter] * 4) - (g_Tilemap.scrollX.i.hi - 0xBB)) + jitter;
                    *(s16*)(q + 8) = (s16)spawnX;
                    *(u16*)(q + 0x32) = 0x77;
                    *(s16*)(q + 0x20) = (s16)spawnX;
                    x2 = (u16)(*(s16*)(q + 8)) + 0x12;
                    *(s16*)(q + 0x14) = x2;
                    *(s16*)(q + 0x2C) = x2;
                    randSeed = (s16)spawnX;
                }
                break;
            }
        }
    }

    {
        u8* q = ext->firstPrim;
        while (q != NULL) {
            if (*(u16*)(q + 0x32) != 8) {
                s16 rise = *(s16*)(q + 0xA) - 1;
                *(s16*)(q + 0xA) = rise;
                if (Random((u16)randSeed) & 7) {
                    rise = *(s16*)(q + 0xA) - 1;
                    *(s16*)(q + 0xA) = rise;
                }
                randSeed = *(s16*)(q + 0xA);
                *(u8*)(q + 0x13) = *(u8*)(q + 0x13) + 1;
                {
                    u8 fadeA = *(u8*)(q + 0x12) + 0xF0;
                    *(u8*)(q + 0x12) = fadeA;
                    *(u8*)(q + 0x11) = fadeA;
                    *(u8*)(q + 0x10) = fadeA;
                    *(u8*)(q + 6) = fadeA;
                    *(u8*)(q + 5) = fadeA;
                    *(u8*)(q + 4) = fadeA;
                }
                {
                    u8 fadeB = *(u8*)(q + 0x2A) + 0xF8;
                    *(u8*)(q + 0x2A) = fadeB;
                    *(u8*)(q + 0x29) = fadeB;
                    *(u8*)(q + 0x28) = fadeB;
                    *(u8*)(q + 0x1E) = fadeB;
                    *(u8*)(q + 0x1D) = fadeB;
                    *(u8*)(q + 0x1C) = fadeB;
                }
                if (*(u8*)(q + 4) < 0x11U) {
                    *(u16*)(q + 0x32) = 8;
                }
            }
            q = *(u8**)(q + 0);
        }
    }
}

extern u16 D_us_80180564[];

typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019F03C; // ext+0x0, ext+0x4

// Rising particle-column effect (rain/bubbles), structurally identical to
// func_us_8019EA54: allocates a chain of Primitives (drawMode==8 = inactive)
// activated up to 4x/frame while the camera is above a scroll threshold
// (scrollY.i.hi < 0xF0), with spawn position jittered by rand() and rand()/10
// (the standard GCC signed-div-by-10 MULT_HI(x,0x66666667) compiler idiom --
// translated directly as C's `/`, which the same compiler re-derives back
// into the identical instruction sequence, not guessed). Active particles
// fade color by -8/frame and accumulate position via a constant
// velocity/accel pair, reusing the Primitive's paint fields as physics
// scratch storage -- same idiom as bo1's debris trail and this file's own
// bubble effect (func_us_8019EA54).
// Confidence: medium -- Entity/Primitive-chain/Tilemap conventions match the
// established idiom exactly; individual scratch-field roles beyond
// position/color are inferred from arithmetic shape and kept as raw offsets
// rather than invented field names, to avoid overstating confidence.
void func_us_8019F03C(Entity* self)
{
    ET_8019F03C* ext = (ET_8019F03C*)&self->ext;
    u8* p;
    s32 i;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(1, 0x80);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x26) = 0x6C;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    ext->frameCounter = (u16)(ext->frameCounter + 1);
    if (g_Tilemap.scrollY.i.hi < 0xF0) {
        s32 rndSeed = g_Tilemap.scrollX.i.hi;
        s32 baseY = 0xB0 - g_Tilemap.scrollY.i.hi;
        s32 baseX = 0x100 - rndSeed;
        for (i = 0; i < 4; i++) {
            u8* q = p;
            while (q != NULL) {
                if (*(u16*)(q + 0x32) == 8) {
                    s32 jitter = Random(rndSeed) & 0x1F;
                    s32 r;
                    s32 div10;
                    *(u8*)(q + 5) = 0x80;
                    *(u8*)(q + 6) = 0x80;
                    *(u8*)(q + 4) = 0x80;
                    *(s16*)(q + 0xA) = (s16)(baseY + jitter);
                    r = Random();
                    div10 = r / 10;
                    *(s32*)(q + 0x18) = 0xFFFE0000;
                    *(s32*)(q + 0x2C) = 0x4000;
                    *(u16*)(q + 0xE) = 0x10;
                    *(s32*)(q + 0x14) = 0;
                    *(s32*)(q + 0x10) = 0;
                    *(u8*)(q + 0xD) = 2;
                    *(u8*)(q + 0xC) = 2;
                    *(s32*)(q + 0x1C) = 0;
                    *(s32*)(q + 0x28) = 0;
                    *(u16*)(q + 0x32) = 2;
                    rndSeed = div10;
                    *(s16*)(q + 8) = (s16)((baseX - (r - (div10 * 0x28))) - jitter);
                    break;
                }
                q = *(u8**)(q + 0);
            }
        }
    }
    p = ext->firstPrim;
    while (p != NULL) {
        if (*(u16*)(p + 0x32) != 8) {
            u16 savedA = *(u16*)(p + 0xA);
            u16 saved8 = *(u16*)(p + 8);
            u8 fadeColor = *(u8*)(p + 5) - 8;
            u16 lifetime = *(u16*)(p + 0xE) - 1;
            *(u16*)(p + 0xE) = lifetime;
            *(u8*)(p + 5) = fadeColor;
            *(u8*)(p + 6) = fadeColor;
            *(u8*)(p + 4) = fadeColor;
            *(s32*)(p + 0x10) = *(s32*)(p + 0x10) + *(s32*)(p + 0x18);
            *(s32*)(p + 0x14) = *(s32*)(p + 0x14) + *(s32*)(p + 0x1C);
            *(s32*)(p + 0x1C) = *(s32*)(p + 0x1C) + *(s32*)(p + 0x2C);
            *(u16*)(p + 8) = saved8;
            *(u16*)(p + 0xA) = savedA;
            if (lifetime == 0) {
                *(u16*)(p + 0x32) = 8;
            }
        }
        p = *(u8**)(p + 0);
    }
}

extern u16 D_us_80180564[];
extern s32 rand();

typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019F30C; // ext+0x0, ext+0x4

typedef struct {
    u16 baseX;  // +0x0: world-space X anchor (scroll-relative spawn X)
    u16 xRange; // +0x2: modulus for the X-jitter random offset
    u16 yRange; // +0x4: modulus for the Y-jitter random offset
    u16 tpage;  // +0x6: fixed tpage value; also doubles as the integer part
                //       of the per-frame velocity folded into x0 (see below)
} D_us_80180C28_Entry;
extern D_us_80180C28_Entry D_us_80180C28[];

/*
 * Slow dust/spark particle column, TILE-primitive variant (64 slots). Same
 * "activate only the first inactive slot found, once per frame" idiom as
 * func_us_8019FAD8/func_us_8019ED3C, using a per-params table
 * (D_us_80180C28) for the spawn X anchor and the X/Y jitter ranges.
 *
 * Confirmed directly against the asm (not just m2c): x0 is saved into the
 * otherwise-unused second RGB quad of the TILE primitive (ext+0x12, since
 * TILE-type primitives never read past their first quad) as the HIGH 16
 * bits of the 32-bit "accumulator" at ext+0x10, which is then added to a
 * per-frame velocity (ext+0x18, low 16 bits always 0, high 16 bits =
 * table->tpage) and its high 16 bits read back into x0 -- net effect: x0
 * increases by table->tpage every frame (a plain rightward drift), done via
 * this roundabout 32-bit overlap rather than a direct `x0 += tpage`. The
 * low 16 bits of the accumulator are never read back as anything
 * meaningful. Kept exactly as found (raw offsets, no named accumulator
 * field) since collapsing it to `x0 += tpage` would be a guess about why
 * the compiler emitted it this way, not something the asm alone confirms.
 */
void func_us_8019F30C(Entity* self)
{
    ET_8019F30C* ext = (ET_8019F30C*)&self->ext;
    u8* p;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(1, 0x40);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x26) = 0x9B; // priority
            *(u16*)(p + 0x32) = 8;    // drawMode (inactive marker)
            p = *(u8**)(p + 0);       // next
        }
    }

    p = ext->firstPrim;
    ext->frameCounter = (u16)(ext->frameCounter + 1);
    if (g_Tilemap.scrollY.i.hi < 0xF0) {
        D_us_80180C28_Entry* table = &D_us_80180C28[self->params];
        s32 baseY = 0xB0 - g_Tilemap.scrollY.i.hi;
        s16 scrollX = g_Tilemap.scrollX.i.hi;
        u8* q = p;
        while (q != NULL) {
            if (*(u16*)(q + 0x32) == 8) {
                *(u8*)(q + 5) = 0x80;
                *(u8*)(q + 6) = 0x80;
                *(u8*)(q + 4) = 0x80;
                *(s16*)(q + 0xA) = (s16)(baseY + (rand() % (s16)table->yRange));
                *(u8*)(q + 0xD) = 2;
                *(u8*)(q + 0xC) = 2;
                *(u16*)(q + 0x32) = 2;
                *(s16*)(q + 0x14) = 0;
                *(s32*)(q + 0x10) = 0;
                *(s32*)(q + 0x18) = 0;
                *(u16*)(q + 0x1A) = table->tpage;
                *(s16*)(q + 8) = (s16)((table->baseX - scrollX) + (rand() % (s16)table->xRange));
                break;
            }
            q = *(u8**)(q + 0);
        }
    }

    p = ext->firstPrim;
    while (p != NULL) {
        if (*(u16*)(p + 0x32) != 8) {
            *(u16*)(p + 0x12) = *(u16*)(p + 8);                          // stash x0 into the accumulator's high half
            *(s32*)(p + 0x10) = *(s32*)(p + 0x10) + *(s32*)(p + 0x18);   // accumulate; high half becomes x0+tpage
            *(u8*)(p + 4) = *(u8*)(p + 4) - 8;
            *(u8*)(p + 5) = *(u8*)(p + 4);
            *(u8*)(p + 6) = *(u8*)(p + 4);
            *(u16*)(p + 8) = *(u16*)(p + 0x12); // x0 = accumulator's high half (x0 + tpage)
            if (*(u8*)(p + 4) < 8) {
                *(u16*)(p + 0x32) = 8;
            }
        }
        p = *(u8**)(p + 0);
    }
}

typedef struct {
    Primitive* prim;    // ext+0x0 (0x7C)
    u8 pad4[4];          // ext+0x4-0x7 (0x80-0x83), unused by this function
    u16 shimmerCounter;  // ext+0x8 (0x84): CLUT-shimmer frame counter, 0-13
} ET_8019F5DC;

extern u16 D_us_80180876; // shared "wall shimmer" jitter globals -- all 6 are
extern u16 D_us_80180878; // written here every call from a single rand()&0x1F
extern u16 D_us_80180882; // roll, but never read by this function itself;
extern u16 D_us_80180858; // presumably consumed by another, not-yet-decompiled
extern u16 D_us_8018085A; // bo3 function.
extern u16 D_us_80180864;

/*
 * Wall-hugging shimmering energy strip (single GT4 quad), anchored to one of
 * two fixed world-X positions (0x218 / 0x468 -- likely two Scylla arena
 * wall/pillar spots), only drawn while the camera scroll is in range of one
 * of them (transcribed directly from the signed-distance comparisons below;
 * the exact intended on-screen window isn't independently re-derived).
 * U-coordinates mirror (0x11 <-> 0x80 on the left/right columns) depending
 * on which of the two anchors is active, and the CLUT cycles 0xA0..0xAD (a
 * 14-frame shimmer animation) via shimmerCounter.
 * Confidence: high on control flow and quad geometry (verified against the
 * asm directly); the roles of the two anchor X positions and of the jitter
 * globals below are inferred from arithmetic shape only.
 */
void func_us_8019F5DC(Entity* self)
{
    ET_8019F5DC* ext = (ET_8019F5DC*)&self->ext;
    Primitive* prim;
    s32 rnd;
    s32 off;
    s16 scrollX;
    s16 dist;
    s16 y0;
    u16 clut;

    if (self->step == 0) {
        s16 slot;

        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 1);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[slot];
        ext->shimmerCounter = 0;
        self->primIndex = slot;
        ext->prim = prim;
        self->flags |= 0x800000;
        prim->tpage = 0xF;
        prim->v1 = 1;
        prim->v0 = 1;
        prim->v3 = 0x7F;
        prim->v2 = 0x7F;
        prim->priority = 0x9C;
        prim->drawMode = 8;
    }

    rnd = rand() & 0x1F;
    off = rnd - 0x10;
    D_us_80180876 = rnd + 0x1C0;
    D_us_80180878 = 0x4C - off;
    D_us_80180882 = rnd + 0x170;
    D_us_80180858 = rnd + 0x530;
    D_us_8018085A = 0x2B0 - off;
    D_us_80180864 = rnd + 0x60;

    ext->shimmerCounter++;
    if (ext->shimmerCounter >= 0xE) {
        ext->shimmerCounter = 0;
    }

    prim = ext->prim;
    clut = ext->shimmerCounter + 0xA0;
    scrollX = g_Tilemap.scrollX.i.hi;

    if (scrollX < 0x288) {
        dist = 0x218 - scrollX;
        if (dist >= 0x100) {
            prim->drawMode = 8;
            return;
        }
        prim->u2 = 0x11;
        prim->u0 = 0x11;
        prim->u3 = 0x80;
        prim->u1 = 0x80;
    } else if (scrollX < 0x4D8) {
        dist = 0x468 - scrollX;
        if (dist >= 0x100) {
            prim->drawMode = 8;
            return;
        }
        prim->u2 = 0x80;
        prim->u0 = 0x80;
        prim->u3 = 0x11;
        prim->u1 = 0x11;
    } else {
        prim->drawMode = 8;
        return;
    }

    y0 = 0xB0 - (s16)g_Tilemap.scrollY.i.hi;
    prim->y1 = y0;
    prim->y0 = y0;
    prim->clut = clut;
    prim->x2 = dist;
    prim->x0 = dist;
    prim->x3 = dist + 0x6F;
    prim->x1 = dist + 0x6F;
    prim->y3 = y0 + 0x7F;
    prim->y2 = y0 + 0x7F;
    prim->drawMode = 0x13;
}

extern s32 rand();
extern u16 D_us_80180588[];

typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019F818; // ext+0x0, ext+0x4

// Rising spark/ember particle effect -- same family and physics-scratch
// idiom as func_us_8019EA54/func_us_8019F03C/func_us_801A27EC: allocates a
// chain of Primitives (drawMode==8 = inactive), tries to activate up to 2
// per frame at a position offset from the entity's own X (self->posX.i.hi),
// jittered by rand()%16 (translated directly via C's % after undoing the
// compiler's round-toward-zero div-by-16 pre-adjustment) with a small extra
// spacing term per attempt (0 then +1 slot). Active particles decelerate via
// the same unk1C-=unk2C / unk18-=unk28 pair already documented, reusing the
// Primitive's paint fields as physics scratch storage.
// Confidence: medium -- Entity/Primitive-chain/Tilemap conventions match the
// established idiom exactly; individual scratch-field roles are inferred
// from arithmetic shape and kept as raw offsets.
void func_us_8019F818(Entity* self)
{
    ET_8019F818* ext = (ET_8019F818*)&self->ext;
    u8* p;
    s32 attempt;
    s16 posX;
    s32 spawnY;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180588);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(1, 0x40);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x26) = 0x6C;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    posX = self->posX.i.hi;
    spawnY = 0x120 - g_Tilemap.scrollY.i.hi;
    ext->frameCounter = (u16)(ext->frameCounter + 1);
    for (attempt = 0; attempt < 2; attempt++) {
        u8* q = p;
        while (q != NULL) {
            if (*(u16*)(q + 0x32) == 8) {
                s32 r = rand();
                *(u8*)(q + 5) = 0x80;
                *(u8*)(q + 6) = 0x80;
                *(u8*)(q + 4) = 0x80;
                *(s16*)(q + 8) = (s16)(((r % 16) * 2) + attempt + posX);
                *(s16*)(q + 0xA) = (s16)(spawnY + ((rand() & 7) - 4));
                *(s32*)(q + 0x14) = 0;
                *(s32*)(q + 0x10) = 0;
                *(u8*)(q + 0xD) = 2;
                *(u8*)(q + 0xC) = 2;
                *(s32*)(q + 0x1C) = 0x40000;
                *(s32*)(q + 0x18) = 0;
                *(s32*)(q + 0x2C) = 0x4000 - (rand() & 0xFF0);
                *(s32*)(q + 0x28) = (rand() & 0x1F00) - 0xF80;
                *(u16*)(q + 0xE) = 0x20;
                *(u16*)(q + 0x32) = 2;
                break;
            }
            q = *(u8**)(q + 0);
        }
    }
    p = ext->firstPrim;
    while (p != NULL) {
        if (*(u16*)(p + 0x32) != 8) {
            s32 velX = *(s32*)(p + 0x1C);
            u16 savedA = *(u16*)(p + 0xA);
            u16 saved8 = *(u16*)(p + 8);
            u16 lifetime = *(u16*)(p + 0xE) - 1;
            *(u16*)(p + 0xE) = lifetime;
            *(s32*)(p + 0x1C) = *(s32*)(p + 0x1C) - *(s32*)(p + 0x2C);
            *(s32*)(p + 0x10) = *(s32*)(p + 0x10) + *(s32*)(p + 0x18);
            *(s32*)(p + 0x14) = *(s32*)(p + 0x14) + velX;
            *(s32*)(p + 0x18) = *(s32*)(p + 0x18) - *(s32*)(p + 0x28);
            *(u16*)(p + 8) = saved8;
            *(u16*)(p + 0xA) = savedA;
            if (lifetime == 0) {
                *(u16*)(p + 0x32) = 8;
            }
        }
        p = *(u8**)(p + 0);
    }
}

extern u16 D_us_80180564[];
extern s16 D_us_80180C30[];

typedef struct { u8* firstPrim; u16 frameCounter; } ET_8019FAD8; // ext+0x0, ext+0x4

// Another rising particle-column effect (4 slots, size 0x20), same family as
// func_us_8019EA54/func_us_8019F03C: each frame, walks the Primitive chain
// looking for the FIRST inactive slot (drawMode==8) and activates at most
// one, spawning it near the bottom of the screen at an X position read from
// a small per-frame-phase offset table (D_us_80180C30[frameCounter], 0..8),
// relative to the entity's own X position, plus a small rand()&3 jitter.
// Every active particle then fades its paint-color fields toward black while
// rising (position -= 1 or -2, chosen by rand()&3) and is deactivated once
// faded past a threshold (paint byte < 0x11). The rand() seed argument used
// in the fade pass is a leftover value carried over from the spawn pass
// (scrollY.i.hi if the chain was empty, the constant 8 if the chain had no
// free slot, or the newly computed spawn X if one was activated) -- this is
// real carried-over state, not an m2c artifact, and is preserved exactly via
// a single shared variable.
// Confidence: medium -- Entity/Primitive-chain conventions match the
// established idiom exactly; D_us_80180C30's exact contents (a small phase
// table) are inferred from usage, and per-field scratch roles are kept as
// raw offsets rather than invented names.
void func_us_8019FAD8(Entity* self)
{
    ET_8019FAD8* ext = (ET_8019FAD8*)&self->ext;
    u8* p;
    s16 randSeed;
    s16 spawnY;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 0x20);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0xE) = 0x161;
            *(u16*)(p + 0x1A) = 0x1A;
            *(u8*)(p + 0x24) = 0;
            *(u8*)(p + 0xC) = 0;
            *(u8*)(p + 0x30) = 0x1E;
            *(u8*)(p + 0x18) = 0x1E;
            *(u8*)(p + 0x19) = 0x60;
            *(u8*)(p + 0xD) = 0x60;
            *(u8*)(p + 0x31) = 0x7C;
            *(u8*)(p + 0x25) = 0x7C;
            *(u16*)(p + 0x26) = 0x9D;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    p = ext->firstPrim;
    randSeed = g_Tilemap.scrollY.i.hi;
    {
        s32 frame = ext->frameCounter + 1;
        ext->frameCounter = (u16)frame;
        if ((s16)frame >= 9) {
            ext->frameCounter = 0;
        }
    }
    spawnY = 0x120 - randSeed;

    if (p != NULL) {
        u8* q = p;
        randSeed = 8;
        while (1) {
            if (*(u16*)(q + 0x32) != 8) {
                /* active slot: skip to next */
                q = *(u8**)(q + 0);
                if (q == NULL) {
                    break;
                }
                continue;
            }
            /* first inactive slot found: activate it */
            {
                s32 spawnX;
                s16 x2;
                *(s16*)(q + 0x2E) = spawnY;
                *(s16*)(q + 0x22) = spawnY;
                *(s16*)(q + 0x16) = spawnY;
                *(s16*)(q + 0xA) = spawnY;
                randSeed = Random(8) & 3;
                *(u8*)(q + 0x12) = 0xFF;
                *(u8*)(q + 0x11) = 0xFF;
                *(u8*)(q + 0x10) = 0xFF;
                *(u8*)(q + 6) = 0xFF;
                *(u8*)(q + 5) = 0xFF;
                *(u8*)(q + 4) = 0xFF;
                *(u8*)(q + 0x2A) = 0x80;
                *(u8*)(q + 0x29) = 0x80;
                *(u8*)(q + 0x28) = 0x80;
                *(u8*)(q + 0x1E) = 0x80;
                *(u8*)(q + 0x1D) = 0x80;
                *(u8*)(q + 0x1C) = 0x80;
                *(u8*)(q + 0x13) = 0;
                spawnX = (D_us_80180C30[(u16)ext->frameCounter] * 4) + randSeed + (self->posX.i.hi - 0x19);
                *(s16*)(q + 8) = (s16)spawnX;
                *(u16*)(q + 0x32) = 0x77;
                *(s16*)(q + 0x20) = (s16)spawnX;
                x2 = (u16)(*(s16*)(q + 8)) + 0x12;
                *(s16*)(q + 0x14) = x2;
                *(s16*)(q + 0x2C) = x2;
                randSeed = (s16)spawnX;
            }
            break;
        }
    }

    {
        u8* q = ext->firstPrim;
        while (q != NULL) {
            if (*(u16*)(q + 0x32) != 8) {
                s16 rise = *(s16*)(q + 0xA) - 1;
                *(s16*)(q + 0xA) = rise;
                if (Random((u16)randSeed) & 3) {
                    rise = *(s16*)(q + 0xA) - 1;
                    *(s16*)(q + 0xA) = rise;
                }
                randSeed = *(s16*)(q + 0xA);
                *(u8*)(q + 0x13) = *(u8*)(q + 0x13) + 1;
                {
                    u8 fadeA = *(u8*)(q + 0x12) + 0xF0;
                    *(u8*)(q + 0x12) = fadeA;
                    *(u8*)(q + 0x11) = fadeA;
                    *(u8*)(q + 0x10) = fadeA;
                    *(u8*)(q + 6) = fadeA;
                    *(u8*)(q + 5) = fadeA;
                    *(u8*)(q + 4) = fadeA;
                }
                {
                    u8 fadeB = *(u8*)(q + 0x2A) + 0xF8;
                    *(u8*)(q + 0x2A) = fadeB;
                    *(u8*)(q + 0x29) = fadeB;
                    *(u8*)(q + 0x28) = fadeB;
                    *(u8*)(q + 0x1E) = fadeB;
                    *(u8*)(q + 0x1D) = fadeB;
                    *(u8*)(q + 0x1C) = fadeB;
                }
                if (*(u8*)(q + 4) < 0x11U) {
                    *(u16*)(q + 0x32) = 8;
                }
            }
            q = *(u8**)(q + 0);
        }
    }
}

extern u16 D_us_80180564[];
extern s16 D_us_80180C44[];

typedef struct {
    u8* firstPrim; // ext+0x0
    u8 pad4[4];    // ext+0x4 (unused by this function)
    s32 frameCounter; // ext+0x8
} ET_8019FDC4;

// Scrolling horizontal "curtain" of colored quad Primitives (4 slots,
// AllocPrimitives(4, 0x10)). Each frame walks a 14-entry position table
// (D_us_80180C44, stride 2 s16 -- only every-other entry is read; the
// skipped half's purpose is not determined) and, for each entry whose
// table-derived X falls in the visible band [-0x1B, 0x100), draws a run of
// quads across the frame from the allocated Primitive chain (clipping the
// last segment against a 0xE8 width limit); entries outside that band, or
// once the chain is exhausted, mark remaining primitives inactive
// (drawMode==8). Verified directly against the .s file (not guessed): the
// two divisions (scroll*5/4) and the modulo-32 are the compiler's own
// round-toward-zero div/mod-by-power-of-2 idiom (explicit "+3"/"+0x1F"
// pre-adjustment before the arithmetic shift for negative operands),
// translated directly via C's / and % (which already truncate toward zero),
// not approximated.
// Confidence: medium-high -- control flow (including the two merged cleanup
// paths, preserved here as literal gotos matching the two real asm labels
// FFFC/0004) was traced through the raw MIPS, not inferred from m2c alone;
// the table's semantic meaning and a few scratch fields remain raw offsets.
void func_us_8019FDC4(Entity* self)
{
    ET_8019FDC4* ext = (ET_8019FDC4*)&self->ext;
    u8* p;
    s16* col;
    s32 q4, q2, r32;
    s32 frame;
    s16 primE;
    s16 slot;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(4, 0x10);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        p = (u8*)&g_PrimBuf[slot];
        ext->frameCounter = 0;
        self->primIndex = slot;
        ext->firstPrim = p;
        self->flags = self->flags | 0x800000;
        while (p != NULL) {
            *(u16*)(p + 0x1A) = 0xF;
            *(u8*)(p + 0x24) = 0x82;
            *(u8*)(p + 0xC) = 0x82;
            *(u8*)(p + 0x30) = 0x9D;
            *(u8*)(p + 0x18) = 0x9D;
            *(u16*)(p + 0x26) = 0xC0;
            *(u16*)(p + 0x32) = 8;
            p = *(u8**)(p + 0);
        }
    }
    col = D_us_80180C44;
    p = ext->firstPrim;
    frame = ext->frameCounter + 1;
    ext->frameCounter = frame;
    if (frame >= 0xE) {
        ext->frameCounter = 0;
    }
    q4 = (g_Tilemap.scrollX.i.hi * 5) / 4;
    q2 = (g_Tilemap.scrollY.i.hi * 5) / 4;
    r32 = q2 % 32;

    if (p == NULL) {
        return;
    }
    primE = (s16)(ext->frameCounter + 0x90);

    while (1) {
        s16 x = *col - q4;
        col += 2;
        if (x >= -0x1B) {
            if (x < 0x100) {
                s32 width = 0x60 - r32;
                s32 xpos = 0;
                if (p == NULL) {
                    return;
                }
                {
                    s16 x2Src = x + 0x1B;
                    s32 uv = r32 + 4;
                    while (1) {
                        s32 uvNext = uv + width;
                        s32 xposPrev = xpos;
                        xpos += width;
                        *(u8*)(p + 0x19) = (u8)uv;
                        *(u8*)(p + 0xD) = (u8)uv;
                        *(s16*)(p + 0x2E) = (s16)xpos;
                        *(s16*)(p + 0x22) = (s16)xpos;
                        *(u16*)(p + 0xE) = (u16)primE;
                        *(u8*)(p + 0x31) = (u8)uvNext;
                        *(u8*)(p + 0x25) = (u8)uvNext;
                        *(s16*)(p + 0x20) = x;
                        *(s16*)(p + 0x8) = x;
                        *(s16*)(p + 0x2C) = x2Src;
                        *(s16*)(p + 0x14) = x2Src;
                        *(s16*)(p + 0x16) = (s16)xposPrev;
                        *(s16*)(p + 0xA) = (s16)xposPrev;
                        *(u16*)(p + 0x32) = 0x71;
                        p = *(u8**)(p + 0);
                        if (xpos < 0xE8) {
                            width = 0x60;
                            if (xpos >= 0x89) {
                                width = 0xE8 - xpos;
                            }
                            uv = 4;
                            if (p != NULL) {
                                continue;
                            }
                        }
                        break;
                    }
                }
            } else {
                /* x >= 0x100: skip the p==NULL check, go straight to the
                   deactivate-rest cleanup (matches asm label 0004->000C) */
                goto deactivate_rest;
            }
        }
        /* shared merge point (asm label FFFC): continue to next table
           entry while primitives remain, otherwise stop */
        if (p != NULL) {
            continue;
        }
        return;
    deactivate_rest:
        if (p != NULL) {
            do {
                *(u16*)(p + 0x32) = 8;
                p = *(u8**)(p + 0);
            } while (p != NULL);
        }
        return;
    }
}

extern s16 PLAYER_posX_i_hi;

typedef struct { Primitive* prim; } ET_801A0034; // ext+0x0

/*
 * Fixed-width composite bar/marker (up to 4 G4 quads, no loops -- always
 * drawn directly rather than iterated by params/count). Anchored either to
 * the player's own X (once the camera has scrolled far enough right,
 * scrollX.i.hi >= 0x91) or to a fixed on-screen X otherwise, at a fixed
 * world Y (0x550-scrollY.i.hi) and a fixed height (0x80px), only drawn at
 * all while that box is still on-screen. Segment 1 is a 0x30px-wide
 * black-to-white horizontal gradient backdrop; segment 2 is a thin 2px
 * accent stripe; segment 3 is a 45px body using the plain dim color set at
 * spawn (priority 0xC1, drawMode 0x37); segment 4, only drawn if there is
 * still room before screen X 0x110, fills the remaining width with that
 * same spawn color using a different drawMode (2). Any prim beyond
 * whatever was actually needed this frame is hidden (drawMode 8).
 * Confidence: high on control flow and Primitive fields (both directly
 * confirmed against include/primitive.h and the asm); the exact in-game
 * purpose of this bar (some kind of boundary/level marker, given the fixed
 * world-Y and camera-scroll-gated behavior) isn't independently confirmed.
 */
void func_us_801A0034(Entity* self)
{
    ET_801A0034* ext = (ET_801A0034*)&self->ext;
    Primitive* prim;
    s16 slot;
    s16 x;
    s16 y0;
    s16 y1;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
        slot = g_api_AllocPrimitives(3, 4);
        if (slot == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[slot];
        self->primIndex = slot;
        ext->prim = prim;
        self->flags |= 0x800000;
        while (prim != NULL) {
            prim->priority = 0xC0;
            prim->g3 = prim->g1 = prim->g2 = prim->g0 = 0x10;
            prim->r3 = prim->r1 = prim->r2 = prim->r0 = 0x10;
            prim->b3 = prim->b1 = prim->b2 = prim->b0 = 0;
            prim = prim->next;
        }
    }

    prim = ext->prim;

    if (g_Tilemap.scrollX.i.hi >= 0x91) {
        x = PLAYER_posX_i_hi + 0x10;
    } else {
        x = 0x120 - g_Tilemap.scrollX.i.hi;
    }
    y0 = 0x550 - g_Tilemap.scrollY.i.hi;
    y1 = y0 + 0x80;

    if (y1 > 0 && x < 0x110) {
        s16 x2 = x + 0x30;
        s16 x3 = x + 1;
        s16 x4 = x2 + 3;
        s16 x5 = x4 + 0x2D;

        prim->x2 = prim->x0 = x;
        prim->x3 = prim->x1 = x2;
        prim->y1 = prim->y0 = y0;
        prim->y3 = prim->y2 = y1;
        prim->b3 = prim->b1 = 0xFF;
        prim->g3 = prim->g1 = 0xFF;
        prim->r3 = prim->r1 = 0xFF;
        prim->b2 = prim->b0 = 0;
        prim->g2 = prim->g0 = 0;
        prim->r2 = prim->r0 = 0;
        prim->drawMode = 0x57;
        prim = prim->next;

        prim->x2 = prim->x0 = x3;
        prim->x3 = prim->x1 = x4;
        prim->y1 = prim->y0 = y0;
        prim->y3 = prim->y2 = y1;
        prim->g2 = prim->g0 = 7;
        prim->r2 = prim->r0 = 7;
        prim->b3 = prim->b1 = prim->b2 = prim->b0 = 0;
        prim->g3 = prim->g1 = 8;
        prim->r3 = prim->r1 = 8;
        prim->drawMode = 0x37;
        prim->priority = 0xC1;
        prim = prim->next;

        prim->x2 = prim->x0 = x4;
        prim->x3 = prim->x1 = x5;
        prim->g3 = prim->g1 = prim->g2 = prim->g0 = 0x10;
        prim->r3 = prim->r1 = prim->r2 = prim->r0 = 0x10;
        prim->y1 = prim->y0 = y0;
        prim->y3 = prim->y2 = y1;
        prim->b3 = prim->b1 = prim->b2 = prim->b0 = 0;
        prim->drawMode = 0x37;
        prim->priority = 0xC1;
        prim = prim->next;

        if (x5 < 0x110) {
            prim->x3 = prim->x1 = 0x110;
            prim->x2 = prim->x0 = x5;
            prim->y1 = prim->y0 = y0;
            prim->y3 = prim->y2 = y1;
            prim->drawMode = 2;
            prim = prim->next;
        }
    }

    while (prim != NULL) {
        prim->drawMode = 8;
        prim = prim->next;
    }
}

extern s32 GetPlayerCollisionWith(Entity* entity, s32 width, s32 height, s32 unk);
extern s16 PLAYER_posX_i_hi;
extern u16 g_pads;

/*
 * Castle-lever entity (bo3 "reverse castle" trigger). step 0 positions
 * itself relative to g_CastleFlags[0xC1] (already-triggered flag). Each
 * frame, if the player overlaps a box around it (GetPlayerCollisionWith)
 * and g_Player.vram_flag bit 0 is set (matches the real g_Player+0x350
 * symbol exactly, per func_us_801A032C.s -- semantically odd but that is
 * the confirmed field at this offset), the player's dpad left/right nudges
 * the lever's posX (and the player's own posX in lockstep) with a 3-frame
 * cooldown stored raw in self->ext (s16 at ext+0). Reaching X==0x720
 * (world space) sets g_CastleFlags[0xC1]. Confidence: high, cross-checked
 * against func_us_801A032C.s throughout.
 */
void func_us_801A032C(Entity* self)
{
    s16 posX;
    s16 cooldown;
    s16 newCooldown;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = -0x7FFF;
        self->animCurFrame = 6;
        if (g_CastleFlags[0xC1] != 0) {
            posX = 0x720 - g_Tilemap.scrollX.i.hi;
        } else {
            posX = 0x760 - g_Tilemap.scrollX.i.hi;
        }
        self->posX.i.hi = posX;
    }
    if ((GetPlayerCollisionWith(self, 0x10, 0x11, 5) & 1) && (g_Player.vram_flag & 1)) {
        posX = self->posX.i.hi;
        if (PLAYER_posX_i_hi < posX) {
            if (g_pads & 0x2000) {
                cooldown = *(s16*)&self->ext;
                if (cooldown != 0) {
                    newCooldown = cooldown - 1;
                } else if ((posX + g_Tilemap.scrollX.i.hi) < 0x7A0) {
                    self->posX.i.hi = posX + 1;
                    PLAYER_posX_i_hi += 1;
                    newCooldown = 3;
                } else {
                    newCooldown = 3;
                }
                *(s16*)&self->ext = newCooldown;
            }
        } else if (g_pads & 0x8000) {
            cooldown = *(s16*)&self->ext;
            if (cooldown != 0) {
                newCooldown = cooldown - 1;
            } else {
                newCooldown = 3;
                if ((posX + g_Tilemap.scrollX.i.hi) >= 0x721) {
                    self->posX.i.hi = posX - 1;
                    PLAYER_posX_i_hi -= 1;
                    newCooldown = 3;
                    if ((self->posX.i.hi + g_Tilemap.scrollX.i.hi) == 0x720) {
                        g_CastleFlags[0xC1] = 1;
                    }
                }
            }
            *(s16*)&self->ext = newCooldown;
        }
    }
}

extern u16 D_us_801808B6;
extern u16 D_us_80180998;
extern u8 D_us_80180C50[]; // anim table for AnimateEntity

typedef struct {
    u16 counter;      // ext+0x0
    u8 pad[0x12];      // ext+0x2..0x13
    u16 frameCounter;   // ext+0x14
} ET_801A051C;

/*
 * Companion lever/gate entity, same g_CastleFlags[0xC1] mechanism as
 * func_us_801A032C. step 0 positions itself and arms its counter (ext+0)
 * to 0x40 if the castle flag is already set, else 0. Every frame it
 * animates; once the flag is set and the counter is below 0x40, it
 * advances the counter every 8 frames (ext+0x14 divides the rate),
 * bumping the SHARED castle flag again at counter==0x14 and ==0x34
 * (staged transition checkpoints). Publishes the counter into two shared
 * globals (likely camera/scroll-limit values) and self-destroys once the
 * counter reaches 0x40. Confidence: medium (m2c output only, not
 * independently .s-verified).
 */
void func_us_801A051C(Entity* self)
{
    ET_801A051C* ext = (ET_801A051C*) &self->ext;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = -0x7FFF;
        self->palette = 0x44;
        self->drawFlags = 0x10;
        self->posX.i.hi = (s16)(0x712 - g_Tilemap.scrollX.i.hi);
        ext->counter = (g_CastleFlags[0xC1] != 0) ? 0x40 : 0;
    }
    AnimateEntity(D_us_80180C50, self);
    if ((g_CastleFlags[0xC1] != 0) && (ext->counter < 0x40)) {
        if ((ext->frameCounter & 7) == 0) {
            ext->counter++;
            if (ext->counter == 0x14) {
                g_CastleFlags[0xC1]++;
            }
            if (ext->counter == 0x34) {
                g_CastleFlags[0xC1]++;
            }
        }
        ext->frameCounter++;
    }
    D_us_80180998 = 0xB1 - ext->counter;
    D_us_801808B6 = 0xB0 - ext->counter;
    if (ext->counter >= 0x40) {
        DestroyEntity(self);
    }
}

extern u8 D_us_80180588[]; // InitializeEntity params
typedef struct {
    u16 width;
    u16 height;
} D_us_80180C58_Entry;
extern D_us_80180C58_Entry D_us_80180C58[];

/*
 * Confidence: medium (m2c output only, not independently .s-verified).
 * Simple collision-box entity: step 0 sets a per-params anim frame, then
 * every frame checks player overlap using a per-params box size from
 * D_us_80180C58[params].
 */
void func_us_801A069C(Entity* self)
{
    u16 params = self->params;
    D_us_80180C58_Entry* box = &D_us_80180C58[params];

    if (self->step == 0) {
        InitializeEntity(D_us_80180588);
        self->animSet = -0x7FFF;
        self->animCurFrame = (s16)(params + 0x19);
    }
    GetPlayerCollisionWith(self, box->width, box->height, 4);
}

extern s16 D_us_80180C64;
extern u16 D_us_80180C6C[];
extern u16 D_us_80180910[];
extern u16 D_us_80180A08[];

/*
 * Scroll-limit table sync (bo3, likely camera/room-transition scroll
 * bounds -- three related tables kept in lockstep). Confidence: medium,
 * transcribed directly from func_us_801A071C.s's raw pointer-offset shape;
 * the exact near/far-table layout implied by the +8/+0xA/+0x10/+0x14
 * writes isn't resolved into named fields since the byte layout differs
 * between the `scrollY < 0xC0` and `>= 0xC0` cases.
 */
void func_us_801A071C(void)
{
    u8* a0 = (u8*) D_us_80180C6C;
    u8* a1 = (u8*) D_us_80180910;
    u8* a2 = (u8*) D_us_80180A08;
    s16 scrollY = D_us_80180C64;
    s16 v1 = 0xD8 - scrollY;

    if (v1 < 0x18) {
        v1 = 0;
    }
    *(s16*)(a0 + 0) = v1;
    a0 += 8;
    *(s16*)(a2 + 0) = v1;
    a2 += 8;
    *(s16*)(a1 + 0) = v1;

    if (scrollY < 0xC0) {
        a1 += 0xA;
        v1 = 0xF0;
    } else {
        v1 = 0x1A8 - scrollY;
    }

    *(s16*)(a0 + 0) = v1;
    *(s16*)(a0 + 8) = v1;
    if (v1 >= 0xE9) {
        v1 = 0xE8;
    }

    *(s16*)(a1 + 0) = v1;
    *(s16*)(a1 + 0xA) = v1;
    v1 += 1;
    *(s16*)(a2 + 0) = v1;
    *(s16*)(a2 + 8) = v1;
}

extern u16 D_us_80180C66;   // shared 1-bit toggle, meaning not resolved beyond usage here
extern s16 D_us_801B2870;   // per-frame water-level delta signal (-1 rising/+1 falling/0 idle);
                            // consumed by other entities riding the surface, e.g. func_us_801A1120

// Water level (D_us_80180C64, the same "scrollY" already named in func_us_801A071C) rise/fall
// driver for bo3 (Scylla's flooding room). self->step machine: 0 picks the initial direction
// from the current level and g_CastleFlags[0xC3] (a "flood the room" trigger flag); 1/2 wait
// until the player crosses world X 0x38, then a further ~0x10-frame delay, before motion
// starts; 3 raises the level by 1 every 3 frames (D_us_801B2870=-1) until it reaches 0xD0,
// then advances; 4 waits for the flag to clear and toggles D_us_80180C66 when it does; 5
// lowers the level by 1 every 2 frames (D_us_801B2870=+1) back down to 0. Steps 3/5 also
// re-sync the scroll-limit tables via func_us_801A071C() every frame.
// Confidence: high on control flow and Entity fields; medium on globals' exact semantics
// (D_us_80180C64 confirmed via func_us_801A071C's existing "scrollY" naming/comment, but
// D_us_80180C66/g_CastleFlags[0xC3]'s specific purpose is inferred from usage only).
typedef struct {
    s16 timer; // ext+0x0 (0x7C): countdown between level increments, and pre-motion delay
} ET_801A07CC;

void func_us_801A07CC(Entity* self)
{
    ET_801A07CC* ext = (ET_801A07CC*) &self->ext;

    D_us_801B2870 = 0;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180564);
            if (D_us_80180C64 != 0) {
                self->step = (g_CastleFlags[0xC3] != 0) ? 5 : 3;
                return;
            }
            if (g_CastleFlags[0xC3] != 0) {
                self->step = 6;
                return;
            }
            return;

        case 1:
            if (g_Entities[0].posX.i.hi >= 0x38) {
                ext->timer = 0x10;
                self->step++;
                D_us_80180C66 |= 1;
            }
            break;

        case 2:
            if (--ext->timer == 0) {
                self->step++;
            }
            break;

        case 3:
            if (ext->timer == 0) {
                ext->timer = 3;
                if (D_us_80180C64 < 0xD0) {
                    D_us_80180C64++;
                    D_us_801B2870 = -1;
                } else {
                    self->step++;
                }
            } else {
                ext->timer--;
            }
            func_us_801A071C();
            break;

        case 4:
            if (g_CastleFlags[0xC3] != 0) {
                self->step++;
                D_us_80180C66 ^= 1;
            }
            break;

        case 5:
            if (ext->timer != 0) {
                ext->timer--;
            } else {
                ext->timer = 1;
                if (D_us_80180C64 > 0) {
                    D_us_80180C64--;
                    D_us_801B2870 = 1;
                } else {
                    self->step++;
                }
            }
            func_us_801A071C();
            break;

        default:
            break;
    }
}

typedef struct {
    u16 threshold;
    u16 rangeLow;
    u16 boundMin;
    s16 boundMax;
} D_us_80180C68_Entry;
extern D_us_80180C68_Entry D_us_80180C68[];

/*
 * Confidence: high, cross-checked against func_us_801A09DC.s. Generic
 * "signed sub-range test" helper: looks up D_us_80180C68[arg0], computes
 * dist = arg1 - threshold; if dist is negative, or dist doesn't fit within
 * [1, rangeLow], or arg2 isn't within [boundMin, boundMax), returns 0.
 * Otherwise writes dist into *arg3 and returns a wrapped 16-bit offset
 * ((arg2 - 0x8000) - rangeLow) & 0xFFFF (likely a UV/animation-frame
 * remap value).
 */
s32 func_us_801A09DC(s32 arg0, s32 arg1, s16 arg2, void* arg3)
{
    D_us_80180C68_Entry* e = &D_us_80180C68[arg0];
    s16 dist = (s16)(arg1 - e->threshold);
    s16 remaining;

    if (dist < 0) {
        return 0;
    }
    *(s16*)arg3 = dist;
    remaining = (s16)(e->rangeLow - dist);
    if (remaining <= 0) {
        return 0;
    }
    if (arg2 < (s16) e->boundMin) {
        return 0;
    }
    if (arg2 >= e->boundMax) {
        return 0;
    }
    return ((arg2 - 0x8000) - e->rangeLow) & 0xFFFF;
}

extern u16 D_us_80180558[]; // InitializeEntity params (step 0)
extern char D_us_8019CA00[]; // FntPrint format string (debug numeric display)
extern s16 D_us_801B2874;    // cached "gap value" from the previous call, persists across frames

typedef struct {
    u8 pad0[2];   // ext+0x0-1 (unused by this function)
    s16 armed;     // ext+0x2 (abs 0x7E): spawn-cooldown counter, armed to 8 after a type-0x30 spawn
    s16 screenX;    // ext+0x4 (abs 0x80): cached player screen-X from the previous call
    s16 probeY;      // ext+0x6 (abs 0x82): cached probe Y from the previous call
    u8 pad8[8];        // ext+0x8-0xF (unused by this function)
    s16 dist1;           // ext+0x10 (abs 0x8C): cached probeDist[0] from the previous call
    s16 dist2;            // ext+0x12 (abs 0x8E): cached probeDist[1] (see note below)
} ET_801A0A80;

/*
 * bo3 (Scylla's flooding room) water-front tracker. g_Entities[0] here is
 * Scylla's own root/anchor entity (same convention already documented on
 * func_us_801A4988). Every call: picks a vertical search range/offset
 * (rangeThresh/probeOffset) from the player's current form (bat/mist/wolf)
 * and crouch state, then runs func_us_801A09DC against D_us_80180C68 up to
 * (self->params>>8) times (starting at subtype self->params&0xFF) looking
 * for a matching gap band near the player. If self->step==0 this is a
 * one-time InitializeEntity only; otherwise, depending on Scylla's own
 * velocity sign and the player's transform state, it spawns either a
 * splash/debris chunk (type 0x2F, func_us_801A1120) or a water-ripple
 * effect (type 0x30, func_us_801A16E4) positioned off Scylla's own
 * position. Every call (both step 0 and otherwise) ends by caching the
 * search results into self's own ext fields and the shared global
 * D_us_801B2874 for comparison next frame. probeDist[1] (ext+0x12/abs
 * 0x8E) is read several times but this function never writes it -- the
 * .s confirms only probeDist[0] is ever stored (func_us_801A09DC's single
 * `*(s16*)arg3 = dist` write); probeDist[1] is a genuine uninitialized-stack
 * read in the original binary, preserved here as-is.
 * Confidence: high on control flow (traced register-by-register against
 * the .s, including the dead `newEntity->params` store right before the
 * final one in the type-0x30 spawn path); medium on the exact
 * tile/gameplay meaning of the search result and of the g_Player.status
 * bit combinations and Scylla-velocity-sign branches.
 */
void func_us_801A0A80(Entity* self)
{
    ET_801A0A80* ext = (ET_801A0A80*) &self->ext;
    s16 screenX = (s16)(PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi);
    s32 status = g_Player.status;
    s16 rangeThresh;
    s16 probeOffset;
    s32 matchCount;
    s16 probeY;
    s16 subtypeIdx;
    s16 searchCount;
    s16 searchResult = 0;
    s16 probeDist[2];

    if (status & 0x27) {
        if (status & 0x20) {
            rangeThresh = 0x14;
            probeOffset = 0x19;
            if (status & 0x4) {
                rangeThresh = 0xA;
            }
        } else if (status & 0x3) {
            rangeThresh = 0xC;
            probeOffset = 0x5;
        } else if (status & 0x4) {
            rangeThresh = 0x14;
            probeOffset = 0x19;
        } else {
            // unreachable given status&0x27 != 0 with none of CROUCH/BAT/MIST/WOLF set;
            // kept for byte-exact fidelity with the .s's exhaustive branch tree
            rangeThresh = 0x14;
            probeOffset = 0x19;
        }
    } else {
        rangeThresh = 0x28;
        probeOffset = 0x19;
    }
    matchCount = 0;

    probeY = (s16)(probeOffset + g_Entities[0].posY.i.hi + g_Tilemap.scrollY.i.hi);
    subtypeIdx = self->params;
    searchCount = (s8)(subtypeIdx >> 8);

    if (searchCount > 0) {
        subtypeIdx &= 0xFF;
        do {
            searchResult = (s16) func_us_801A09DC(subtypeIdx, screenX, probeY, &probeDist[0]);
            matchCount++;
            if (searchResult != 0) {
                break;
            }
            subtypeIdx++;
        } while (matchCount < searchCount);
    }

    if (self->step == 0) {
        InitializeEntity(D_us_80180558);
    } else {
        s16 gapValue = searchResult & 0x7FFF;
        s16 velYHi = *((s16*) &g_Entities[0].velocityY + 1);

        if (velYHi != 0 && !(status & 0x3)) {
            if (velYHi < 0) {
                // spawn variant A: uses the PREVIOUS frame's cached dist1/dist2
                // (ext->dist1/ext->dist2), not this frame's probeDist[]
                if (searchResult == 0) {
                    if (D_us_801B2874 != 0 && (D_us_801B2874 & 0x7FFF) < 0x11) {
                        s16 spawnBase;
                        Entity* newEntity;

                        if (ext->dist1 < 0xE) {
                            spawnBase = (s16)(0xE - ext->dist1);
                        } else if (ext->dist2 < 0xE) {
                            spawnBase = (s16)(ext->dist2 + 0xE);
                        } else {
                            spawnBase = 0;
                        }

                        newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (newEntity != NULL) {
                            func_us_801A8DF0(0x2F, g_Entities, newEntity);
                            // positions derived from THIS entity's own previous-frame
                            // cached screenX/probeY (ext->screenX/ext->probeY), overwriting
                            // whatever CreateEntityFromEntity just set
                            newEntity->posX.i.hi = (s16)(ext->screenX - g_Tilemap.scrollX.i.hi);
                            newEntity->posY.i.hi = (s16)(ext->probeY - (D_us_801B2874 & 0x7FFF) - g_Tilemap.scrollY.i.hi);
                            newEntity->zPriority = g_Entities[0].zPriority;
                            if (g_Entities[0].velocityY > -0x40000) {
                                newEntity->params = spawnBase + 1;
                            } else {
                                newEntity->params = spawnBase;
                            }
                        }
                    }
                }
            } else {
                // spawn variant B: uses THIS frame's probeDist[]
                if (searchResult != 0 && gapValue < 9 && D_us_801B2874 == 0) {
                    s16 spawnBase;
                    Entity* newEntity;

                    if (probeDist[0] < 0xE) {
                        spawnBase = (s16)(0xE - probeDist[0]);
                    } else if (probeDist[1] < 0xE) {
                        spawnBase = (s16)(probeDist[1] + 0xE);
                    } else {
                        spawnBase = 0;
                    }

                    newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                    if (newEntity != NULL) {
                        func_us_801A8DF0(0x2F, g_Entities, newEntity);
                        if (g_Entities[0].velocityY > 0x40000) {
                            newEntity->params = spawnBase + 1; // dead store, immediately overwritten below
                        } else {
                            newEntity->params = spawnBase + 2; // dead store, immediately overwritten below
                        }
                        newEntity->params = spawnBase;
                        newEntity->posY.i.hi = (s16)(newEntity->posY.i.hi + probeOffset - gapValue);
                        newEntity->zPriority = g_Entities[0].zPriority;
                    }
                }
            }
        } else if (!(status & 0x2)) {
            if (searchResult != 0) {
                if (ext->armed == 0 && screenX != ext->screenX) {
                    if (rangeThresh >= gapValue && probeDist[0] >= 6 && probeDist[1] >= 6) {
                        Entity* newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (newEntity != NULL) {
                            func_us_801A8DF0(0x30, g_Entities, newEntity);
                            newEntity->posY.i.hi = (s16)(newEntity->posY.i.hi + probeOffset - gapValue);
                            if (g_Entities[0].velocityX != 0) {
                                newEntity->params = (s16)((rangeThresh - gapValue) >> 3);
                                if (newEntity->params == 5) {
                                    newEntity->params = 4;
                                }
                            } else {
                                newEntity->params = 0;
                            }
                            *(u16*)((u8*) newEntity + 0x8A) = 0;
                            newEntity->zPriority = g_Entities[0].zPriority;
                            ext->armed = 8;
                        }
                    }
                }
            }
        }

        if (ext->armed != 0) {
            ext->armed--;
        }

        // Debug numeric HUD (FntPrint) showing gapValue and two derived tiers,
        // bucketed by player form. D_80097450 (and, in the wolf-alone case,
        // D_8009744C) is left untouched -- not zeroed -- when the corresponding
        // "unreachable" branch (0x27 set but no individual bit matches) is taken;
        // matches the .s exactly.
        g_unkGraphicsStruct.D_80097448 = gapValue;

        if (status & 0x27) {
            if (status & 0x20) {
                if (status & 0x4) {
                    g_unkGraphicsStruct.D_8009744C = (gapValue < 5) ? 0 : (gapValue - 4);
                    g_unkGraphicsStruct.D_80097450 = (gapValue < 0xD) ? 0 : (gapValue - 0xC);
                } else {
                    g_unkGraphicsStruct.D_8009744C = (gapValue < 9) ? 0 : (gapValue - 8);
                    g_unkGraphicsStruct.D_80097450 = (gapValue < 0x19) ? 0 : (gapValue - 0x18);
                }
            } else if (status & 0x3) {
                g_unkGraphicsStruct.D_8009744C = (gapValue < 7) ? 0 : (gapValue - 6);
                g_unkGraphicsStruct.D_80097450 = (gapValue < 0x11) ? 0 : (gapValue - 0x10);
            } else if (status & 0x4) {
                g_unkGraphicsStruct.D_8009744C = (gapValue < 9) ? 0 : (gapValue - 8);
                g_unkGraphicsStruct.D_80097450 = (gapValue < 0x19) ? 0 : (gapValue - 0x18);
            }
        } else {
            g_unkGraphicsStruct.D_8009744C = (gapValue < 0x11) ? 0 : (gapValue - 0x10);
            g_unkGraphicsStruct.D_80097450 = (gapValue < 0x31) ? 0 : (gapValue - 0x30);
        }

        FntPrint(D_us_8019CA00, gapValue);
    }

    ext->screenX = screenX;
    ext->probeY = probeY;
    ext->dist1 = probeDist[0];
    D_us_801B2874 = searchResult;
    ext->dist2 = probeDist[1];
}
typedef struct {
    s32 velocityY; // initial velocityY for this subtype
    s32 gravity;    // per-frame velocityY delta while falling (step 1)
} D_us_80180C80_Entry;
extern D_us_80180C80_Entry D_us_80180C80[];

typedef struct {
    s32 gravity;    // ext+0x0 (abs 0x7C): NOT a Primitive* despite sitting at
                     // the same offset as the common "prim" union member --
                     // re-purposed here to hold a per-frame velocityY delta,
                     // copied from D_us_80180C80[subtype].gravity at step 0
                     // (confirmed against the .s: stored via a plain `sw`,
                     // never dereferenced as a pointer)
    u8 pad4[2];       // ext+0x4-5 (unused by this function)
    u16 baseY;        // ext+0x6 (abs 0x82): posY.i.hi+scrollY snapshot,
                       // set once at step 0
} ET_801A1120;

// Splash/debris chunk spawned by a Scylla water impact: 2 stacked GT4 quads
// (prim0 = bright highlight, prim1 = an identically-shaped dark "shadow"
// copy, built by literally copying prim0's already-computed U/V/X/Y fields
// rather than recomputing them). prim0's shape depends on params>>0xB (a
// 5-bit launch-angle-like value): if nonzero, one edge of the quad is
// stretched by an angle-proportional amount, extending either the right or
// left side depending on whether the angle is past the halfway point
// (0xF/0x1F). params&0xF selects an entry in D_us_80180C80 for the initial
// velocityY and a per-frame gravity value (stored in ext, despite m2c
// initially labeling that field "prim" -- confirmed not a pointer since
// step 1 adds it directly to velocityY as a plain s32). Also fires off a
// companion type-0x1B entity sharing the same initial velocityY. step 1
// applies gravity until velocityY exceeds 0x28000 (i.e. falling fast) then
// advances to step 2; step 2 just watches prim0's fade-out and self-
// destroys once it's nearly black. Every step (0 after init, 1, 2) shares
// one tail: both quads drift with the water level (D_us_801B2870, same
// global as func_us_801A07CC/func_us_801A16E4) and fade toward black in two
// stages (dark shadow quad fades first, then the bright quad's outer ring,
// then its inner highlight once the outer ring is dark enough).
// Confidence: high on control flow (re-verified the "saved_reg_s0" the raw
// dump flagged: it's just the previous loop iteration's primitive pointer,
// always written on the loop's first pass before being read on the second
// -- not real cross-call persistent state, unlike func_us_801A0A80).
// D_us_80180C80/D_us_80180C84 in the raw dump are actually one contiguous
// 8-byte-stride struct array (confirmed directly against
// asm/us/boss/bo3/data/0.data.s -- D_us_80180C84 begins exactly 4 bytes
// after D_us_80180C80 with no gap), collapsed into a single typed array here
// rather than kept as two separately-named globals.
void func_us_801A1120(Entity* self)
{
    ET_801A1120* ext = (ET_801A1120*) &self->ext;
    s16 angle = (s16) ((u16) self->params >> 0xB);
    u16 subtype = self->params & 0xF;
    s16 deltaY;
    Primitive* prim;
    s32 i;

    switch (self->step) {
        case 0: {
            s16 allocIdx = g_api_AllocPrimitives(PRIM_GT4, 2);
            Primitive* prev = NULL;
            s16 y;
            s16 x;
            s16 xLeft;
            s16 xRight;
            s32 pan;

            if (allocIdx == -1) {
                DestroyEntity(self);
                return;
            }
            prim = &g_PrimBuf[allocIdx];
            self->primIndex = (s32) allocIdx;
            y = self->posY.i.hi;
            self->flags |= 0x800000;
            x = self->posX.i.hi;
            ext->baseY = y + (u16) g_Tilemap.scrollY.i.hi;
            xLeft = x - 0xE;
            xRight = x + 0xE;

            for (i = 0; prim != NULL; i++) {
                if (i & 1) {
                    // odd quad: copy prim0's already-computed shape verbatim
                    prim->u2 = prim->u0 = prev->u0;
                    prim->u3 = prim->u1 = prev->u1;
                    prim->v1 = prim->v0 = prev->v0;
                    prim->v3 = prim->v2 = prev->v2;
                    prim->y2 = prev->y2;
                    prim->y3 = prev->y3;
                    prim->x0 = prim->x2 = prev->x0;
                    prim->x1 = prim->x3 = prev->x1;
                } else {
                    prim->u3 = prim->u1 = 0x20;
                    prim->u2 = prim->u0 = 0;
                    prim->v1 = prim->v0 = 0x60;
                    prim->v3 = prim->v2 = 0x7C;
                    prim->y3 = prim->y2 = y;
                    prim->x0 = prim->x2 = xLeft;
                    prim->x1 = prim->x3 = xRight;
                    if (angle != 0) {
                        s16 stretch = (s16) (((s32) angle << 5) / 28);

                        if ((u16) angle >= 0xF) {
                            prim->u3 = prim->u1 = (u8) (prim->u0 + stretch);
                            prim->x3 = prim->x1 = (s16) (angle + prim->x0);
                        } else {
                            prim->u2 = prim->u0 = (u8) (prim->u2 + stretch);
                            prim->x2 = prim->x0 = (s16) (angle + prim->x2);
                        }
                    }
                }

                prim->b1 = prim->g1 = prim->r1 = 0xFF;
                prim->b0 = prim->g0 = prim->r0 = 0xFF;
                prim->b3 = prim->g3 = prim->r3 = 0x80;
                prim->b2 = prim->g2 = prim->r2 = 0x80;
                prim->clut = 0x162;
                prim->tpage = 0x1A;
                prim->drawMode = 0x77;
                prim->priority = (s16) (self->zPriority + 2);
                if (i & 1) {
                    prim->clut = 0x15F;
                    prim->b1 = prim->g1 = prim->r1 = 0;
                    prim->b0 = prim->g0 = prim->r0 = 0;
                    prim->b3 = prim->g3 = prim->r3 = 0x60;
                    prim->b2 = prim->g2 = prim->r2 = 0x60;
                    prim->drawMode = 0x37;
                    prim->priority += 2;
                }

                prev = prim;
                prim = prim->next;
            }

            pan = ((s16) self->posX.i.hi - 0x78) >> 4;
            if (pan < -8) {
                pan = -8;
            }
            if ((s16) pan >= 9) {
                pan = 8;
            }
            g_api_PlaySfxVolPan(0x7C2, 0x7F, pan);

            self->velocityY = D_us_80180C80[subtype].velocityY;
            ext->gravity = D_us_80180C80[subtype].gravity;
            {
                Entity* companion = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (companion != NULL) {
                    func_us_801A8D7C(0x1B, companion);
                    companion->velocityY = self->velocityY;
                }
            }
            break;
        }
        case 1:
            MoveEntity();
            self->velocityY += ext->gravity;
            if (self->velocityY > 0x28000) {
                self->step++;
            }
            break;
        case 2:
            MoveEntity();
            if ((u8) g_PrimBuf[self->primIndex].r0 < 9U) {
                DestroyEntity(self);
                return;
            }
            break;
        default:
            DestroyEntity(self);
            return;
    }

    // shared per-frame tail: drift with the water level, fade toward black
    deltaY = (ext->baseY - (u16) self->posY.i.hi) - (u16) g_Tilemap.scrollY.i.hi;
    prim = &g_PrimBuf[self->primIndex];
    for (i = 0; prim != NULL; i++) {
        u16 newY = (u16) prim->y3 + (u16) D_us_801B2870;

        prim->y3 = prim->y2 = newY;
        prim->y0 = newY - deltaY;
        prim->y1 = (u16) prim->y3 - deltaY;

        if (i & 1) {
            if ((u8) prim->b3 >= 4U) {
                prim->b3 += 0xFC;
            }
            prim->g3 = prim->r3 = prim->b2 = prim->g2 = prim->r2 = prim->b3;
        } else {
            if ((u8) prim->b3 >= 8U) {
                prim->b3 += 0xFC;
            }
            prim->g3 = prim->r3 = prim->b2 = prim->g2 = prim->r2 = prim->b3;
            if ((u8) prim->b3 < 9U) {
                if ((u8) prim->b1 >= 8U) {
                    prim->b1 += 0xF8;
                }
                prim->g1 = prim->r1 = prim->b0 = prim->g0 = prim->r0 = prim->b1;
            }
        }
        prim = prim->next;
    }
}

extern s32 rand();
extern u16 D_us_80180CA0[]; // per-(params&0xFF) base ripple height
typedef struct {
    u8 pad0[6];       // ext+0x0-5 (unused by this function)
    u16 baseY;        // ext+0x6 (abs 0x82): posY.i.hi+scrollY snapshot, refreshed
                       // every completed tail call; used to re-derive the
                       // on-screen Y delta after the water-level drift is applied
    f32 rippleAccum;   // ext+0x8 (abs 0x84): fixed-point accumulator -- .i.lo is a
                       // sub-frame remainder decremented by 0x4000/frame in step 1,
                       // .i.hi is the actual ripple height in pixels (shrinks by 1
                       // every few frames as .val borrows out of the low half)
} ET_801A16E4;

// Water-ripple/splash visual: 2 stacked GT4 quads spanning two texture pages
// (tpage 0x1A both, but distinct clut 0x162/0x161 and priority
// zPriority+2/+4) drawn around the entity's X position, following the water
// surface's rise/fall via the shared D_us_801B2870 drift global (same one
// func_us_801A07CC uses to raise/lower the water level). Every call, both
// quads' colors fade toward black (corners 0/1 by 8, corners 2/3 by 4) and
// once the fully-faded red channel drops under 9 the entity self-destroys.
// step 0 allocates the quads and seeds the ripple height from a per-(params
// & 0xFF) table plus a small random jitter; step 1 (only) drains the
// fixed-point ripple-height accumulator by a fixed amount per frame so the
// ripple visually shrinks over time; any other step just runs the shared
// per-frame quad-update tail.
// Confidence: high on control flow and Primitive fields (re-verified byte
// offsets directly against the .s, not just the raw m2c dump -- the dump's
// address arithmetic for the primitive pointer was algebraically simplified
// here to a plain `&g_PrimBuf[allocIdx]` after confirming the constant folds
// to exactly sizeof(Primitive)); the ext+8 fixed-point split (accumulator +
// ripple height sharing one 32-bit word) is inferred from the matching
// `f32`-style union already used for posX/posY elsewhere in this codebase,
// not independently confirmed against a named source.
void func_us_801A16E4(Entity* self)
{
    ET_801A16E4* ext = (ET_801A16E4*) &self->ext;
    Primitive* prim;
    s16 x;
    s16 xLeft;
    s16 xRight;
    s32 i;

    if (self->step == 0) {
        s16 allocIdx;
        s32 pan;
        s16 y;

        InitializeEntity(D_us_80180588);
        allocIdx = g_api_AllocPrimitives(PRIM_GT4, 2);
        if (allocIdx == -1) {
            DestroyEntity(self);
            return;
        }
        self->primIndex = (s32) allocIdx;
        pan = (self->posX.i.hi - 0x78) >> 4;
        self->flags |= 0x800000;
        if (pan < -8) {
            pan = -8;
        }
        if ((s16) pan >= 9) {
            pan = 8;
        }
        if (!(self->params & 0x8000)) {
            g_api_PlaySfxVolPan(0x7C3, 0x7F, pan);
        }

        prim = &g_PrimBuf[allocIdx];
        x = self->posX.i.hi;
        y = self->posY.i.hi;
        xLeft = x - 9;
        xRight = x + 9;
        ext->baseY = y + (u16) g_Tilemap.scrollY.i.hi;
        for (i = 0; i < 2; i++) {
            prim->clut = 0x162;
            prim->u2 = 0;
            prim->u0 = 0;
            prim->u3 = 0x1E;
            prim->u1 = 0x1E;
            prim->v1 = 0x60;
            prim->v0 = 0x60;
            prim->v3 = 0x7C;
            prim->v2 = 0x7C;
            prim->y3 = y;
            prim->y2 = y;
            prim->x0 = xLeft;
            prim->x2 = xLeft;
            prim->x1 = xRight;
            prim->x3 = xRight;
            prim->b1 = prim->g1 = prim->r1 = 0xFF;
            prim->b0 = prim->g0 = prim->r0 = 0xFF;
            prim->b3 = prim->g3 = prim->r3 = 0x80;
            prim->b2 = prim->g2 = prim->r2 = 0x80;
            prim->tpage = 0x1A;
            prim->drawMode = 0x77;
            prim->priority = (s16) (self->zPriority + 2);
            if (i != 0) {
                prim->clut = 0x161;
                prim->drawMode = 0x77;
                prim->priority = (s16) (self->zPriority + 4);
            }
            prim = prim->next;
        }
        ext->rippleAccum.i.hi = D_us_80180CA0[(u8) self->params] + ((rand() & 1) + 0xC);
    } else if (self->step == 1) {
        ext->rippleAccum.val -= 0x4000;
    }

    prim = &g_PrimBuf[self->primIndex];
    x = self->posX.i.hi;
    xLeft = x - 9;
    xRight = x + 9;
    self->posY.i.hi = (u16) self->posY.i.hi + (u16) D_us_801B2870;
    {
        s16 deltaY = (ext->baseY - (u16) self->posY.i.hi) - (u16) g_Tilemap.scrollY.i.hi;

        for (i = 0; i < 2; i++) {
            u8 fade01;
            u8 fade23;

            prim->y2 = (u16) prim->y2 - deltaY;
            prim->y3 = (u16) prim->y3 - deltaY;
            prim->y0 = (u16) prim->y2 - ext->rippleAccum.i.hi;
            prim->x0 = xLeft;
            prim->x2 = xLeft;
            prim->x1 = xRight;
            prim->x3 = xRight;

            fade01 = prim->b1 + 0xF8;
            prim->b1 = prim->g1 = prim->r1 = fade01;
            prim->b0 = prim->g0 = prim->r0 = fade01;

            fade23 = prim->b3 + 0xFC;
            prim->b3 = prim->g3 = prim->r3 = fade23;
            prim->b2 = prim->g2 = prim->r2 = fade23;

            prim->y1 = (u16) prim->y3 - ext->rippleAccum.i.hi;

            if ((u8) prim->r0 < 9U) {
                DestroyEntity(self);
                return;
            }
            prim = prim->next;
        }
    }
    ext->baseY = (u16) self->posY.i.hi + (u16) g_Tilemap.scrollY.i.hi;
}

typedef struct {
    u16 boxX;
    s16 threshX;
    u16 boxY;
    s16 threshY;
} D_us_80180CAC_Entry;
extern D_us_80180CAC_Entry D_us_80180CAC[];
extern s8 D_80073421;
extern s16 PLAYER_posY_i_hi;
extern s32 PLAYER_velocityY;

/*
 * Confidence: high, cross-checked against func_us_801A1A6C.s. "Ceiling
 * crusher hitbox" check: if the player's position (world-space, offset by
 * g_Tilemap scroll) falls within a box read from D_us_80180CAC[params],
 * dampens the player's downward velocity (velocityY *= 7/8, round toward
 * zero) if falling, or sets D_80073421 (a global flag, likely a "player
 * squashed/hit" trigger) if moving upward.
 */
void func_us_801A1A6C(Entity* self)
{
    D_us_80180CAC_Entry* box = &D_us_80180CAC[self->params];
    s32 velocityY;

    if (self->step == 0) {
        InitializeEntity(D_us_80180564);
        self->animSet = 0;
    }

    if ((s16)((PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi) - box->boxX) > box->threshX) {
        return;
    }
    if ((s16)((PLAYER_posY_i_hi + g_Tilemap.scrollY.i.hi) - box->boxY) > box->threshY) {
        return;
    }

    velocityY = PLAYER_velocityY;
    if (velocityY < 0) {
        velocityY *= 7;
        if (velocityY < 0) {
            velocityY += 7;
        }
        PLAYER_velocityY = velocityY >> 3;
    } else if (velocityY > 0) {
        D_80073421 = 1;
    }
}

extern u16 D_us_80180CC4[];
extern u16 D_us_80180CD8[];

/*
 * Writes a 4-tile-tall vertical strip into g_Tilemap's foreground buffer:
 * starting tile offset comes from D_us_80180CC4 (indexed by `column`
 * rounded down to even, as a raw byte offset), and the 4 tile values come
 * from D_us_80180CD8[column] (an 8-byte/4-u16 record). Each subsequent
 * tile is one tilemap row down (g_Tilemap.hSize*16 elements). Confidence:
 * high, cross-checked against func_us_801A1B7C.s.
 */
void func_us_801A1B7C(s32 column)
{
    u16* src = (u16*)((u8*) D_us_80180CD8 + column * 8);
    s16 rowOffset = *(u16*)((u8*) D_us_80180CC4 + (column & ~1));
    s32 i;

    for (i = 0; i < 4; i++) {
        g_Tilemap.fg[rowOffset] = *src;
        rowOffset += g_Tilemap.hSize * 16;
        src++;
    }
}

extern void MoveEntity(void);
extern u8 D_us_80180CD0[]; // per-(params>>1) facingLeft lookup

// Rising/falling platform tied to the water level (shares D_us_80180C66 with
// func_us_801A07CC: bit `params` of that mask reflects whether this platform's
// phase is currently active). step 0: init, look up facingLeft from a small
// table, then either (bit set) rewrite the tile strip one column over and
// advance to step 4, or (bit clear) rewrite the strip in place and drop posY by
// 0x40 (rest at the lower position). step 1: once the bit is set, launch
// upward (velocityY=0x40000) and advance. step 2/5: nudge posX by +-8 around
// GetPlayerCollisionWith (a hit-detection idiom also seen elsewhere in this
// file) then move; step 2 continues rising until posY crosses the top of the
// screen (then plays a splash-ish sfx, redraws the strip, reverses velocity,
// advances); step 5 continues until posY drops back below a lower threshold
// (then reverses back to the rising branch). step 3: falls (velocityY+=0x2000)
// until hitting the floor (posY clamped, advances). step 4: once the bit
// clears, launches back downward (velocityY=-0x10000), redraws the strip, and
// advances.
// Confidence: high on control flow and Entity fields (self->params/step/
// facingLeft/velocityY/posX/posY confirmed via game.h); medium on
// D_us_80180C66/D_us_80180CD0's precise semantics (inferred from usage, cross-
// checked against func_us_801A07CC's identical bitmask access but not named
// beyond that).
void func_us_801A1BE8(Entity* self)
{
    u16 subtype = self->params;
    s32 activePhase = D_us_80180C66 & (1 << subtype);

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180588);
            self->animSet = -0x7FFF;
            self->animCurFrame = 0x25;
            self->zPriority = 0x9E;
            self->facingLeft = D_us_80180CD0[subtype >> 1];
            if (activePhase != 0) {
                func_us_801A1B7C(subtype + 1);
                self->step = 4;
                return;
            }
            func_us_801A1B7C(subtype);
            self->posY.i.hi -= 0x40;
            return;

        case 1:
            if (activePhase != 0) {
                self->velocityY = 0x40000;
                self->step++;
                return;
            }
            break;

        case 2:
            self->posX.i.hi += self->facingLeft ? 8 : -8;
            GetPlayerCollisionWith(self, 0x10, 0x20, 0xB);
            self->posX.i.hi += self->facingLeft ? -8 : 8;
            MoveEntity();
            if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x80) {
                g_api_PlaySfxVolPan(0x655, 0x7F, -6);
                func_us_801A1B7C(subtype + 1);
                self->velocityY = -0x10000;
                self->step++;
                return;
            }
            break;

        case 3:
            self->velocityY += 0x2000;
            MoveEntity();
            if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x80) {
                self->posY.i.hi = 0x80 - g_Tilemap.scrollY.i.hi;
                self->step++;
                return;
            }
            break;

        case 4:
            if (activePhase == 0) {
                self->velocityY = -0x10000;
                func_us_801A1B7C(subtype);
                self->step++;
                return;
            }
            break;

        case 5:
            self->posX.i.hi += self->facingLeft ? -8 : 8;
            GetPlayerCollisionWith(self, 0x10, 0x20, 0xB);
            self->posX.i.hi += self->facingLeft ? 8 : -8;
            MoveEntity();
            if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x41) {
                self->step++;
                return;
            }
            break;
    }
}

extern s16 D_us_80180D38[]; // tilt-slope table, indexed by a 3-bit params sub-field

typedef struct {
    s32 gravity; // ext+0x0: same "ext.prim re-purposed as s32" idiom as
                  // func_us_801A1120 -- confirmed the same way, via a plain
                  // `sw`/`lw` at abs 0x7C, never dereferenced as a pointer
    u8 pad4[2];    // ext+0x4-5 (unused by this function)
    u16 baseY;     // ext+0x6 (abs 0x82): posY.i.hi+scrollY snapshot
} ET_801A1EE4;

// Sibling of func_us_801A1120 (same splash/debris-chunk family, same overall
// step machine and per-frame fade/drift tail), extended with two optional
// extra quads: params bits 8-10 pick a 4-vs-2-quad layout (4 only if the
// launch angle, params>>0xB, is nonzero AND params bits 5-7 != 7), and if
// present, quads 2/3 are a "connector" bridging quad0's stretched edge back
// toward the impact point, tilted via a slope looked up in D_us_80180D38 by
// params bits 5-7. Quad 0 additionally gets its own independent tilt (same
// D_us_80180D38 table, this time indexed by params bits 8-10) applied AFTER
// the angle-stretch, tilting y2/y3 based on how far the entity's own posX
// sits from the quad's x0/x1 span. Quads 1 and 3 are still plain dark
// "shadow" copies of quads 0 and 2 respectively (verbatim field copies, same
// as func_us_801A1120).
// Confidence: high on control flow, cross-checked field-by-field against the
// already-verified func_us_801A1120 (near-identical shape) plus direct
// re-reading of the raw dump's tilt-math branches; D_us_80180D38's per-slope
// semantics are inferred from usage only.
void func_us_801A1EE4(Entity* self)
{
    ET_801A1EE4* ext = (ET_801A1EE4*) &self->ext;
    s16 angle = (s16) ((u16) self->params >> 0xB);
    u16 connectorSlopeSel = (self->params >> 8) & 7;
    u16 tiltSlopeSel = (self->params >> 5) & 7;
    u16 subtype = self->params & 0xF;
    s16 deltaY;
    Primitive* prim;
    s32 i;

    switch (self->step) {
        case 0: {
            s32 quadCount = (angle != 0 && connectorSlopeSel != 7) ? 4 : 2;
            s16 allocIdx = g_api_AllocPrimitives(PRIM_GT4, quadCount);
            Primitive* prev = NULL;
            s16 y;
            s16 x;
            s16 xLeft;
            s16 xRight;
            s32 pan;

            if (allocIdx == -1) {
                DestroyEntity(self);
                return;
            }
            prim = &g_PrimBuf[allocIdx];
            self->primIndex = (s32) allocIdx;
            y = self->posY.i.hi;
            self->flags |= 0x800000;
            x = self->posX.i.hi;
            ext->baseY = y + (u16) g_Tilemap.scrollY.i.hi;
            xLeft = x - 0xE;
            xRight = x + 0xE;

            for (i = 0; prim != NULL; i++) {
                if (i & 1) {
                    // odd quad: dark shadow copy of the previous (even) quad
                    prim->u2 = prim->u0 = prev->u0;
                    prim->u3 = prim->u1 = prev->u1;
                    prim->v1 = prim->v0 = prev->v0;
                    prim->v3 = prim->v2 = prev->v2;
                    prim->y2 = prev->y2;
                    prim->y3 = prev->y3;
                    prim->x0 = prim->x2 = prev->x0;
                    prim->x1 = prim->x3 = prev->x1;
                } else if (i >= 2) {
                    // connector quad bridging quad0's stretched edge
                    s16 slope = D_us_80180D38[connectorSlopeSel];

                    if (angle < 0xF) {
                        prim->u2 = prim->u0 = prev->u1;
                        prim->x2 = prim->x0 = prev->x1;
                        prim->y2 = prev->y3;
                        prim->y3 = (slope != 0) ? (s16) (prev->y3 - (s32) (prim->x1 - prim->x0) / slope) : prev->y3;
                    } else {
                        prim->u3 = prim->u1 = prev->u0;
                        prim->x3 = prim->x1 = prev->x0;
                        prim->y3 = prev->y2;
                        prim->y2 = (slope != 0) ? (s16) (prev->y2 + (s32) (prim->x1 - prim->x0) / slope) : prev->y2;
                    }
                } else {
                    prim->u3 = prim->u1 = 0x20;
                    prim->v3 = prim->v2 = 0x7C;
                    prim->u2 = prim->u0 = 0;
                    prim->v1 = prim->v0 = 0x60;
                    prim->y3 = prim->y2 = y;
                    prim->x0 = prim->x2 = xLeft;
                    prim->x1 = prim->x3 = xRight;
                    if (angle != 0) {
                        s16 stretch = (s16) (((s32) angle << 5) / 28);

                        if (angle < 0xF) {
                            prim->u3 = prim->u1 = (u8) (prim->u0 + stretch);
                            prim->x3 = prim->x1 = (s16) (angle + prim->x0);
                        } else {
                            prim->u2 = prim->u0 = (u8) (prim->u2 + stretch);
                            prim->x2 = prim->x0 = (s16) (angle + prim->x2);
                        }
                    }
                    if (tiltSlopeSel != 0) {
                        s16 tiltSlope = D_us_80180D38[tiltSlopeSel];

                        if (tiltSlope < 0) {
                            if (x >= prim->x1) {
                                prim->y2 = (u16) prim->y2 + (prim->x1 - prim->x0) / tiltSlope;
                            } else {
                                prim->y3 = (u16) prim->y3 - (prim->x1 - x) / tiltSlope;
                                prim->y2 = (u16) prim->y2 + (x - prim->x0) / tiltSlope;
                            }
                        } else {
                            if (prim->x0 >= x) {
                                prim->y3 = (u16) prim->y3 - (prim->x1 - prim->x0) / tiltSlope;
                            } else {
                                prim->y2 = (u16) prim->y2 + (x - prim->x0) / tiltSlope;
                                prim->y3 = (u16) prim->y3 - (prim->x1 - x) / tiltSlope;
                            }
                        }
                    }
                }

                prim->b1 = prim->g1 = prim->r1 = 0xFF;
                prim->b0 = prim->g0 = prim->r0 = 0xFF;
                prim->b3 = prim->g3 = prim->r3 = 0x80;
                prim->b2 = prim->g2 = prim->r2 = 0x80;
                prim->clut = 0x162;
                prim->tpage = 0x1A;
                prim->drawMode = 0x77;
                prim->priority = (s16) (self->zPriority + 2);
                if (i & 1) {
                    prim->clut = 0x15F;
                    prim->b1 = prim->g1 = prim->r1 = 0;
                    prim->b0 = prim->g0 = prim->r0 = 0;
                    prim->b3 = prim->g3 = prim->r3 = 0x60;
                    prim->b2 = prim->g2 = prim->r2 = 0x60;
                    prim->drawMode = 0x37;
                    prim->priority += 2;
                }

                prev = prim;
                prim = prim->next;
            }

            pan = ((s16) self->posX.i.hi - 0x78) >> 4;
            if (pan < -8) {
                pan = -8;
            }
            if ((s16) pan >= 9) {
                pan = 8;
            }
            g_api_PlaySfxVolPan(0x7C2, 0x7F, pan);

            self->velocityY = D_us_80180C80[subtype].velocityY;
            ext->gravity = D_us_80180C80[subtype].gravity;
            {
                Entity* companion = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (companion != NULL) {
                    func_us_801A8D7C(0x1B, companion);
                    companion->velocityY = self->velocityY;
                }
            }
            break;
        }
        case 1:
            MoveEntity();
            self->velocityY += ext->gravity;
            if (self->velocityY > 0x28000) {
                self->step++;
            }
            break;
        case 2:
            MoveEntity();
            if ((u8) g_PrimBuf[self->primIndex].r0 < 9U) {
                DestroyEntity(self);
                return;
            }
            break;
        default:
            DestroyEntity(self);
            return;
    }

    // shared per-frame tail: drift with the water level, fade toward black
    // (note: unlike func_us_801A1120, this sibling does NOT re-add
    // D_us_801B2870 to y2/y3 here -- matches the .s exactly, only y0/y1 are
    // rederived from the already-updated y2/y3 each frame)
    deltaY = (ext->baseY - (u16) self->posY.i.hi) - (u16) g_Tilemap.scrollY.i.hi;
    prim = &g_PrimBuf[self->primIndex];
    for (i = 0; prim != NULL; i++) {
        prim->y0 = (u16) prim->y2 - deltaY;
        prim->y1 = (u16) prim->y3 - deltaY;

        if (i & 1) {
            if ((u8) prim->b3 >= 4U) {
                prim->b3 += 0xFC;
            }
            prim->g3 = prim->r3 = prim->b2 = prim->g2 = prim->r2 = prim->b3;
        } else {
            if ((u8) prim->b3 >= 8U) {
                prim->b3 += 0xFC;
            }
            prim->g3 = prim->r3 = prim->b2 = prim->g2 = prim->r2 = prim->b3;
            if ((u8) prim->b3 < 9U) {
                if ((u8) prim->b1 >= 8U) {
                    prim->b1 += 0xF8;
                }
                prim->g1 = prim->r1 = prim->b0 = prim->g0 = prim->r0 = prim->b1;
            }
        }
        prim = prim->next;
    }
}

extern s16 (*g_api_func_800EDB58)(s32 type, s32 count); // returns starting Primitive index for a chain of `count` prims (type 0x11), or -1

typedef struct {
    u16 counter; // ext+0x0: frames-remaining countdown; chain destroyed when it hits 0
} ET_801A27EC;

/*
 * Rising ember/spark chain particle effect -- same family/idiom as
 * func_us_8019F818 etc: allocates a chain of Primitives (here via
 * g_api_func_800EDB58 rather than g_api_AllocPrimitives), self->primIndex
 * holds the chain's starting index, self->flags bit 0x800000 marks
 * "chain allocated". Confidence: high on control flow/Entity fields
 * (fully cross-checked against func_us_801A27EC.s, including two delay-slot
 * subtleties: prim->unk26 is written unconditionally before the
 * null-next/terminator check in the init loop, and the a1=0x8000 constant
 * used in the update loop's velocity-cap compare is set in the branch delay
 * slot of the countdown check so it applies on both paths). Individual
 * Primitive scratch-field roles (unk8/unkA draw pos, unk12/unk16
 * scatter-target pos, unk1C velocity/unk24 lifetime) are inferred from the
 * established idiom shared with other functions in this file.
 */
void func_us_801A27EC(Entity* self)
{
    ET_801A27EC* ext = (ET_801A27EC*) &self->ext;
    u8* p;
    s16 posX;
    s16 posY;
    s32 primBase;

    posX = self->posX.i.hi;
    posY = self->posY.i.hi;

    switch (self->step) {
        case 0: {
            u16 zp2;
            u8* next;

            InitializeEntity(D_us_80180588);
            primBase = g_api_func_800EDB58(0x11, 0x21);
            if (primBase == -1) {
                DestroyEntity(self);
                return;
            }
            p = (u8*) &g_PrimBuf[primBase];
            ext->counter = 0x2F;
            self->primIndex = primBase;
            self->flags |= 0x800000;

            while (1) {
                *(u16*)(p + 0x32) = 0x73;
                zp2 = (u16)(self->zPriority + 2);
                next = *(u8**)(p + 0);
                *(u16*)(p + 0x26) = zp2;
                if (next == NULL) {
                    goto deactivate_tail;
                }
                *(u16*)(p + 0x14) = 0;
                *(u16*)(p + 0x10) = 0;
                *(s32*)(p + 0x1C) = (rand() << 3) + self->velocityY;
                *(s16*)(p + 0x16) = (s16)(posY + (rand() & 0xF));
                *(s16*)(p + 0x12) = (s16)(posX + (rand() & 0x1F) - 0x10);
                *(s16*)(p + 0x24) = (s16)((rand() & 0xF) + 0x20);
                *(u8*)(p + 4) = 0xFF;
                *(u8*)(p + 5) = 0xFF;
                *(u8*)(p + 6) = 0xFF;
                *(u8*)(p + 0xC) = 2;
                *(u8*)(p + 0xD) = 2;
                *(u16*)(p + 8) = *(u16*)(p + 0x12);
                *(u16*)(p + 0xA) = *(u16*)(p + 0x16);
                p = next;
            }
        }

        case 1: {
            s16 oldX, oldY, lifetime;
            s32 velX;

            if (--ext->counter == 0) {
                DestroyEntity(self);
                return;
            }
            primBase = self->primIndex;
            p = (u8*) &g_PrimBuf[primBase];

            while (1) {
                if (*(u8**)(p + 0) == NULL) {
                    goto deactivate_tail;
                }
                oldX = *(s16*)(p + 8);
                oldY = *(s16*)(p + 0xA);
                *(s16*)(p + 0x12) = oldX;
                lifetime = (s16)(*(u16*)(p + 0x24) - 1);
                *(u16*)(p + 0x24) = (u16) lifetime;
                *(s16*)(p + 0x16) = oldY;
                if (lifetime == 0) {
                    *(u16*)(p + 0x32) |= 8;
                }
                velX = *(s32*)(p + 0x1C);
                *(s32*)(p + 0x14) += velX;
                if (0x8000 < velX) {
                    *(u8*)(p + 4) -= 4;
                    *(u8*)(p + 6) -= 4;
                    *(u8*)(p + 5) -= 4;
                } else {
                    *(s32*)(p + 0x1C) = velX + 0x3800;
                }
                *(u16*)(p + 8) = *(u16*)(p + 0x12);
                *(u16*)(p + 0xA) = *(u16*)(p + 0x16);
                p = *(u8**)(p + 0);
            }
        }
    }
    return;

deactivate_tail:
    *(u8*)(p + 0xC) = 0;
    *(u16*)(p + 8) = 0;
    *(u16*)(p + 0xA) = 0;
    *(u16*)(p + 0x32) &= 0xFFF7;
}

/*
 * Confidence: medium (m2c output only, not independently .s-verified).
 * Generic decay-toward-threshold routine over 2 groups of 3 bytes each
 * (stride 0xC between groups, e.g. RGB triples for two light sources),
 * returns nonzero if any byte was still above the threshold this call.
 */
s32 func_us_801A2A90(u8* arg0, s32 threshold)
{
    u8* p = arg0 + 4;
    s32 changed = 0;
    s32 group, i;
    s32 v;

    for (group = 0; group < 2; group++) {
        for (i = 0; i < 3; i++) {
            v = *p - threshold;
            if (v <= 0) {
                v = 0;
            } else {
                changed = 1;
            }
            *p = (u8) v;
            p++;
        }
        p += 9;
    }
    return changed;
}

extern u16 D_us_801805A0[]; // InitializeEntity params (step 0)
extern u8 D_us_80180D4C[];  // anim table for AnimateEntity (steps 1/2)
extern u8 D_us_80180D58[];  // anim table for AnimateEntity (step 3, sub-step 0)
extern u8 D_us_80180D68[];  // anim table for AnimateEntity (step 3, sub-step 2)
extern u8 D_us_80180D74[];  // anim table for AnimateEntity (step 3, sub-step 3)
extern u8 D_us_80180D8C[];  // anim table for AnimateEntity (step 4, sub-step 0)
extern u8 D_us_80180D9C[];  // anim table for AnimateEntity (step 4, sub-step 2)
extern u8 D_us_80180DA8[];  // anim table for AnimateEntity (step 4, sub-step 3)
extern u8 D_us_80180DB4[];  // anim table for AnimateEntity (step 5)
extern u8 D_us_80180DC8[];  // anim table for AnimateEntity (step 6, sub-step 0/1)
extern char D_us_8019CA28[]; // FntPrint format string (debug frame-stepper display)
extern Entity g_Entities_64[];
extern u16 g_pads_1_pressed;
extern s32 AnimateEntity(u8* anim, Entity* self); // return-value variant needed here
extern void SetSubStep(s32 step_s);
extern s32 D_us_80180D48; // shared boss-wide flag/bitmask (see other users in this file)

typedef struct {
    u8 pad0[4];   // ext+0x0-3 (unused by this function)
    u16 unk80;     // ext+0x4 (abs 0x80): shared countdown timer, re-armed with
                    // a different value at the start of most step-6 sub-states
    u8 pad6[4];      // ext+0x6-8 (unused by this function)
    u8 unk85;         // ext+0x9 (abs 0x85): set 0 at the start of step 3's debris
                       // burst, set 1 once step 3 finishes (not read elsewhere
                       // in this function -- likely consumed by a sibling)
    u8 unk86;           // ext+0xA (abs 0x86): 0/1 toggle flag (step 2 blink-alternate)
} ET_801A2AEC;

/*
 * Scylla's multi-stage death cutscene / room-exit sequence (E_UNK_31). self
 * is Scylla's own root entity (the same one func_us_801A5338 sets up), and
 * this function additionally manages THREE more fixed "virtual slots" in
 * the same contiguous g_Entities_224-style array: self+2, self+15, self+28
 * (spawned as type 0x3A explosion markers in step 1).
 *
 * Top of every call: if D_us_80180D48 bit 2 is set, destroy self immediately.
 * Otherwise, once self->flags bit 0x100 latches (same shared-flag convention
 * as func_us_801A5338/801A4988), promotes to step 6.
 *
 * step 0: one-time setup (hitbox disabled, self+1 chain link), spawns a
 *   type-0x33 child at self+1, SetStep(1) (falls through to step 1's body
 *   the same frame).
 * step 1: waits for the player to be close (or hitFlags already set), marks
 *   the Time Attack "visited" flag, spawns 3 type-0x3A explosion markers at
 *   self+2/self+15/self+28, SetStep(2).
 * step 2: alternates a corpse-flash animation and toggles `unk86` every
 *   ~0x50 frames, alternating between step 3 and 4.
 * step 3 (sub-steps 0-3): plays a sequence of animations, bursts 6 type-0x35
 *   debris entities once, then once a packed pose/poseTimer word reads
 *   exactly 4, plays SFX 0x806.
 * step 4 (sub-steps 0-3): mirrors step 3 with a different animation set,
 *   periodically spawning a type-0x36 splash entity, then SetStep(2).
 * step 5: plays SFX 0x807 once, then an animation, looping back to step 2
 *   once done (step_s reset to 1, a quirk of the shared step_s field).
 * step 6 (sub-steps 0-8, dispatched via jtbl_us_8019CA38 -- resolved
 *   directly from the retail ROM's rodata rather than guessed): the actual
 *   "explosion montage" -- Time Attack record + castle flag + fanfare,
 *   final animation, then three back-to-back debris-spawn bursts (4, 2,
 *   then >=1-retry entities of type 0x3D) gated by re-armed countdowns,
 *   each phase also flipping bits in the shared D_us_80180D48 mask, ending
 *   by spawning a type-0x40 entity and destroying self.
 * step 0xFF: debug frame-stepper (FntPrint + pad-driven animCurFrame
 *   inc/dec), gated behind a two-button combo -- almost certainly a
 *   developer-only tool, never reachable in normal play.
 *
 * Confidence: high on control flow (every step/sub-step boundary
 * cross-checked against the .s, and the step-6 jump table resolved from
 * the actual BO3.BIN rodata rather than inferred -- jtbl_us_8019CA38 at
 * VRAM 0x8019CA38 / file offset 0x1CA38 in disks/us/BOSS/BO3/BO3.BIN);
 * medium on the exact gameplay meaning of the various D_us_80180D48/
 * D_us_80180C66 bit flips and g_CastleFlags[0xC3] (shared globals used
 * elsewhere in this file by inference only), and on self->unk50's packed
 * pose/poseTimer read in step 3 sub-step 3.
 */
void func_us_801A2AEC(Entity* self)
{
    ET_801A2AEC* ext = (ET_801A2AEC*) &self->ext;

    if (D_us_80180D48 & 4) {
        DestroyEntity(self);
        return;
    }

    if (self->flags & 0x100) {
        if (self->step != 6) {
            SetStep(6);
        }
    }

    switch (self->step) {
        case 0: {
            Entity* slot1 = self + 1;

            InitializeEntity(D_us_801805A0);
            self->zPriority = 0x88;
            self->hitboxOffX = 6;
            self->hitboxOffY = -0x30;
            self->hitboxWidth = 4;
            self->hitboxHeight = 0x1C;
            self->nextPart = slot1;
            self->parent = NULL;
            func_us_801A8DF0(0x33, self, slot1);
            slot1->nextPart = self;
            slot1->parent = self;
            ext->unk80 = self->zPriority + 1;
            SetStep(1);
        }
        /* fallthrough */
        case 1: {
            Entity* slot2 = self + 2;
            Entity* slot15 = self + 15;
            Entity* slot28 = self + 28;
            s32 dist;

            AnimateEntity(D_us_80180D4C, self);
            dist = GetDistanceToPlayerX();
            if (dist >= 0xB0 && self->hitFlags == 0) {
                return;
            }

            g_api_TimeAttackController(TIMEATTACK_EVENT_SCYLLA_DEFEAT, TIMEATTACK_SET_VISITED);
            D_us_80180C66 |= 0x30;

            func_us_801A8DF0(0x3A, self, slot2);
            slot2->zPriority = self->zPriority;
            slot2->posX.i.hi = (s16)(0x48 - g_Tilemap.scrollX.i.hi);
            slot2->params = 1;
            slot2->posY.i.hi = (s16)(0x90 - g_Tilemap.scrollY.i.hi);

            func_us_801A8DF0(0x3A, self, slot15);
            slot15->zPriority = self->zPriority - 2;
            slot15->posX.i.hi = (s16)(0x48 - g_Tilemap.scrollX.i.hi);
            slot15->params = 2;
            slot15->posY.i.hi = (s16)(0x90 - g_Tilemap.scrollY.i.hi);

            func_us_801A8DF0(0x3A, self, slot28);
            slot28->zPriority = self->zPriority - 4;
            slot28->posX.i.hi = (s16)(0x48 - g_Tilemap.scrollX.i.hi);
            slot28->params = 3;
            slot28->posY.i.hi = (s16)(0x90 - g_Tilemap.scrollY.i.hi);
            D_us_80180D48 |= 1;

            SetStep(2);
            return;
        }
        case 2:
            if (self->step_s == 0) {
                ext->unk80 = 0x50;
                self->step_s++;
            }
            AnimateEntity(D_us_80180D4C, self);
            if (self->hitFlags & 3) {
                SetStep(5);
            }
            ext->unk80--;
            if (ext->unk80 != 0) {
                return;
            }
            SetStep((ext->unk86 == 0) ? 3 : 4);
            ext->unk86 ^= 1;
            return;
        case 3:
            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180D58, self) != 0) {
                        return;
                    }
                    SetSubStep(1);
                    return;
                case 1: {
                    Entity* newEntity;
                    Entity* pool2;
                    s32 i;

                    newEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                    if (newEntity != NULL) {
                        func_us_801A8DF0(0x34, self, newEntity);
                        newEntity->posX.i.hi += 0x14;
                        newEntity->posY.i.hi -= 0x34;
                        *(Entity**) ((u8*) newEntity + 0x9C) = self; // ext field, not otherwise named
                        newEntity->zPriority -= 1;
                    }

                    pool2 = (Entity*) ((u8*) g_Entities_160 + 0x2F00);
                    for (i = 0; i < 6; i++) {
                        Entity* debris = AllocEntity(pool2, (Entity*) ((u8*) pool2 + 0x1780));
                        if (debris != NULL) {
                            func_us_801A8DF0(0x35, self, debris);
                            debris->posY.i.hi = (s16)(0xD8 - g_Tilemap.scrollY.i.hi);
                            debris->params = i;
                            debris->posX.i.hi = (s16)(self->posX.i.hi + (Random() & 0x7F) + 0x30);
                        }
                    }

                    ext->unk85 = 0;
                    PlaySfxPositional(0x805);
                    ext->unk80 = 0x50;
                    self->step_s++;
                }
                /* fallthrough */
                case 2:
                    AnimateEntity(D_us_80180D68, self);
                    ext->unk80--;
                    if (ext->unk80 != 0) {
                        return;
                    }
                    SetSubStep(3);
                    return;
                case 3:
                    if (AnimateEntity(D_us_80180D74, self) == 0) {
                        SetStep(2);
                    }
                    if (*(s32*) ((u8*) self + 0x50) != 4) {
                        return;
                    }
                    PlaySfxPositional(0x806);
                    ext->unk85 = 1;
                    return;
            }
            return;
        case 4:
            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180D8C, self) != 0) {
                        return;
                    }
                    SetSubStep(1);
                    return;
                case 1:
                    ext->unk80 = 0x40;
                    self->step_s++;
                    /* fallthrough */
                case 2:
                    AnimateEntity(D_us_80180D9C, self);
                    if ((ext->unk80 & 7) == 0) {
                        Entity* newEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (newEntity != NULL) {
                            func_us_801A8DF0(0x36, self, newEntity);
                            newEntity->posY.i.hi = (s16)(0xBC - g_Tilemap.scrollY.i.hi);
                            newEntity->posX.i.hi = (s16)(newEntity->posX.i.hi + 0x30 + (Random() & 0x7F));
                        }
                    }
                    ext->unk80--;
                    if (ext->unk80 != 0) {
                        return;
                    }
                    SetSubStep(3);
                    return;
                case 3:
                    if (AnimateEntity(D_us_80180DA8, self) != 0) {
                        return;
                    }
                    SetStep(2);
                    return;
            }
            return;
        case 5:
            if (self->step_s == 0) {
                PlaySfxPositional(0x807);
                self->step_s++;
            }
            if (AnimateEntity(D_us_80180DB4, self) != 0) {
                return;
            }
            SetStep(2);
            self->step_s = 1;
            return;
        case 6:
            if ((u32) self->step_s >= 9) {
                return;
            }
            switch (self->step_s) {
                case 0:
                    g_api_TimeAttackController(TIMEATTACK_EVENT_SCYLLA_DEFEAT, TIMEATTACK_SET_RECORD);
                    D_us_80180D48 |= 2;
                    g_CastleFlags[0xC3] = 1;
                    PlaySfxPositional(0x808);
                    self->hitboxState = 0;
                    self->step_s++;
                    /* fallthrough */
                case 1:
                    if (AnimateEntity(D_us_80180DC8, self) != 0) {
                        return;
                    }
                    SetSubStep(2);
                    return;
                case 2:
                    ext->unk80 = 0x80;
                    PlaySfxPositional(0x7C5);
                    self->step_s++;
                    return;
                case 3: {
                    s32 count = 0;

                    if ((g_Timer & 0x1F) == 0) {
                        g_api_PlaySfx(0x660);
                    }
                    do {
                        Entity* debris = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                        if (debris != NULL) {
                            s32 rnd;

                            count++;
                            func_us_801A8DF0(0x3D, self, debris);
                            debris->zPriority = 0x98;
                            debris->params = 1;
                            rnd = Random();
                            debris->posY.i.hi = 0xD8;
                            if (rnd < 0x40) {
                                debris->posX.i.hi = (s16)(debris->posX.i.hi - 0x40 + rnd);
                            } else {
                                debris->posY.i.hi = 0xB0;
                            }
                        }
                    } while (count < 4);
                    ext->unk80--;
                    if (ext->unk80 != 0) {
                        return;
                    }
                    D_us_80180D48 |= 0x10;
                    self->step_s++;
                    return;
                }
                case 4:
                    D_us_80180D48 |= 0x20;
                    ext->unk80 = 0x50;
                    PlaySfxPositional(0x7C5);
                    self->step_s++;
                    /* fallthrough */
                case 5: {
                    s32 count = 0;

                    if ((g_Timer & 0x1F) == 0) {
                        g_api_PlaySfx(0x661);
                    }
                    do {
                        Entity* debris = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                        if (debris != NULL) {
                            s32 rnd;

                            count++;
                            func_us_801A8DF0(0x3D, self, debris);
                            debris->zPriority = 0x98;
                            rnd = Random();
                            debris->params = 2;
                            debris->posY.i.hi = 0xC0;
                            debris->posX.i.hi = (s16)(debris->posX.i.hi - 0x20 + (rnd & 0x3F));
                        }
                    } while (count < 2);
                    ext->unk80--;
                    if (ext->unk80 != 0) {
                        return;
                    }
                    D_us_80180D48 |= 8;
                    self->step_s++;
                    return;
                }
                case 6:
                    PlaySfxPositional(0x7C5);
                    ext->unk80 = 0x60;
                    self->step_s++;
                    /* fallthrough */
                case 7: {
                    s32 count = 0;

                    if ((g_Timer & 0x1F) == 0) {
                        g_api_PlaySfx(0x662);
                    }
                    do {
                        Entity* debris = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                        if (debris != NULL) {
                            s32 rnd;

                            count++;
                            func_us_801A8DF0(0x3D, self, debris);
                            debris->zPriority = 0x98;
                            rnd = Random();
                            debris->params = 3;
                            debris->posY.i.hi = 0xB0;
                            debris->posX.i.hi = (s16)(debris->posX.i.hi - 0x10 + (rnd & 0x1F));
                        }
                    } while (count <= 0);
                    ext->unk80--;
                    if (ext->unk80 != 0) {
                        return;
                    }
                    self->step_s++;
                    return;
                }
                case 8: {
                    Entity* newEntity;

                    D_us_80180C66 &= 0xFFCF;
                    D_us_80180D48 |= 4;
                    newEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                    if (newEntity == NULL) {
                        return;
                    }
                    func_us_801A8DF0(0x40, self, newEntity);
                    newEntity->posX.i.hi = (s16)(0xC0 - g_Tilemap.scrollX.i.hi);
                    newEntity->params = 3;
                    newEntity->posY.i.hi = (s16)(0x80 - g_Tilemap.scrollY.i.hi);
                    DestroyEntity(self);
                    return;
                }
            }
            return;
        case 0xFF: {
            s16 frame = self->animCurFrame;

            FntPrint(D_us_8019CA28, frame);

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

            self->posX.i.hi = 0x80;
            self->posY.i.hi = 0x80;
            return;
        }
    }
}

extern s32 AnimateEntity(u8* anim, Entity* self); // return-value variant needed here
extern void SetStep(s32 step);
extern void SetSubStep(s32 step_s);
extern void PlaySfxPositional(s32 sfxId);
extern s32 D_us_80180D48; // shared boss-wide flag/bitmask (see other users in this file)
extern u16 D_us_801805AC[]; // InitializeEntity params (step 0)
extern u8 D_us_80180DD4[]; // anim table for AnimateEntity (steps 1/2)
extern u8 D_us_80180DE0[]; // anim table for AnimateEntity (steps 3/5 sub-step 1, step 6 sub-steps 1/3)
extern u8 D_us_80180DE8[]; // anim table for AnimateEntity (step 5 sub-step 2)
extern u8 D_us_80180E04[]; // anim table for AnimateEntity (step 3 sub-step 2)
extern u8 D_us_80180E18[]; // anim table for AnimateEntity (step 3 sub-step 4)
extern u8 D_us_80180E5C[]; // anim table for AnimateEntity (step 4 sub-step 0)
extern u8 D_us_80180E6C[]; // anim table for AnimateEntity (step 4 sub-step 1)
extern u8 D_us_80180E80[]; // anim table for AnimateEntity (step 4 sub-step 2)
extern u8 D_us_80180E94[]; // anim table for AnimateEntity (step 4 sub-step 3)

typedef struct {
    u8 pad0[4];  // ext+0x0-3 (unused by this function)
    u16 unk80;   // ext+0x4 (abs 0x80): shared countdown timer, re-armed with
                  // a different value across step 2/3/5's sub-states
    u8 pad6[4];  // ext+0x6-9 (unused by this function)
    u8 unk86;    // ext+0xA (abs 0x86): 0/1 toggle flag (step 2 alternation,
                  // same role as func_us_801A2AEC's ext->unk86)
} ET_801A365C;

/*
 * Scylla's post-cutscene "sink into the water" epilogue -- runs after
 * func_us_801A2AEC's explosion montage, on the same root entity, and
 * synchronizes with it purely through the shared D_us_80180D48 bitmask
 * (no direct entity pointers between the two). Promotes to step 6 via the
 * same self->flags bit 0x100 convention used throughout this file.
 *
 * step 0: one-time hitbox setup (small, mostly disabled footprint),
 *   SetStep(1) (falls through to step 1's body the same frame).
 * step 1: plays a floating/idle animation every frame until
 *   D_us_80180D48 bit 0 latches (set by func_us_801A2AEC's step 1 once its
 *   3 explosion markers spawn), then SetStep(2).
 * step 2 (sub-steps 0/1): re-arms a 0x60-frame countdown, plays the same
 *   idle animation with a 1-in-128 chance per frame to cut the wait short,
 *   then alternates SetStep(3)/SetStep(4) once the countdown expires
 *   (mirrors func_us_801A2AEC's step 2 alternation tail exactly).
 * step 3 (sub-steps 0-4, jtbl_us_8019CA80 target -- resolved directly from
 *   the retail ROM's rodata): a countdown-gated animation, a type-0x37
 *   companion spawn, then a final animation before looping back to step 2.
 * step 4 (sub-steps 0-3): 3 animations gated on self->unk50 (packed
 *   pose/poseTimer word, same field used in func_us_801A2AEC's step 3),
 *   each conditionally firing SFX 0x780 once, converging on a shared
 *   8-entity type-0x38 debris burst out of g_Entities_160 once the final
 *   sub-step's animation finishes; sub-step 3 alone never reaches the
 *   burst (loops back to step 2 instead).
 * step 5 (sub-steps 0-2, jtbl_us_8019CA60 target): a near-identical
 *   wind-down/companion-spawn pass, sharing its sub-step 0/1 tail with
 *   step 3's at the assembly level (same PlaySfxPositional(0x783) call,
 *   different re-armed countdown value).
 * step 6 (sub-steps 0-3): final fade -- idle animation until
 *   D_us_80180D48 bit 0x20 latches, then shrinks self->scaleX/scaleY down
 *   to nothing, destroying self once D_us_80180D48 bit 0x8 is set
 *   (presumably by func_us_801A2AEC's step 6 sub-step 8 finale).
 *
 * Confidence: high on control flow (every step/sub-step boundary
 * cross-checked against the .s, both jump tables resolved from the actual
 * BO3.BIN rodata rather than inferred -- jtbl_us_8019CA60 at VRAM
 * 0x8019CA60 / file offset 0x1CA60, jtbl_us_8019CA80 at VRAM 0x8019CA80 /
 * file offset 0x1CA80); medium on the exact gameplay meaning of the shared
 * D_us_80180D48 bit flips (inferred from func_us_801A2AEC's usage) and
 * self->unk50's packed pose/poseTimer read in step 4.
 */
void func_us_801A365C(Entity* self)
{
    ET_801A365C* ext = (ET_801A365C*) &self->ext;

    if (self->flags & 0x100) {
        if (self->step != 6) {
            SetStep(6);
        }
    }

    if (self->step < 7) {
        switch (self->step) {
            case 0:
                InitializeEntity(D_us_801805AC);
                self->hitboxOffX = 0x14;
                self->hitboxOffY = -0xE;
                self->hitboxWidth = 0x18;
                self->hitboxHeight = 0x14;
                SetStep(1);
                /* fallthrough */
            case 1:
                AnimateEntity(D_us_80180DD4, self);
                if (D_us_80180D48 & 1) {
                    SetStep(2);
                }
                return;
            case 2:
                switch (self->step_s) {
                    case 0:
                        ext->unk80 = 0x60;
                        self->step_s++;
                        /* fallthrough */
                    case 1:
                        AnimateEntity(D_us_80180DD4, self);
                        if ((Random() & 0x7F) == 0) {
                            SetSubStep(2);
                        }
                        ext->unk80--;
                        if (ext->unk80 != 0) {
                            return;
                        }
                        SetStep((ext->unk86 == 0) ? 3 : 4);
                        ext->unk86 ^= 1;
                        return;
                }
                return;
            case 3:
                switch (self->step_s) {
                    case 0:
                        PlaySfxPositional(0x783);
                        ext->unk80 = 0x28;
                        self->step_s++;
                        /* fallthrough */
                    case 1:
                        AnimateEntity(D_us_80180DE0, self);
                        ext->unk80--;
                        if (ext->unk80 != 0) {
                            return;
                        }
                        SetSubStep(2);
                        return;
                    case 2:
                        if (AnimateEntity(D_us_80180E04, self) == 0) {
                            SetSubStep(3);
                        }
                        return;
                    case 3: {
                        Entity* newEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (newEntity != NULL) {
                            func_us_801A8DF0(0x37, self, newEntity);
                            newEntity->posX.i.hi += 0x30;
                            newEntity->posY.i.hi -= 8;
                            newEntity->zPriority = self->zPriority + 1;
                        }
                        self->step_s++;
                    }
                    /* fallthrough */
                    case 4:
                        if (AnimateEntity(D_us_80180E18, self) == 0) {
                            SetStep(2);
                        }
                        return;
                }
                return;
            case 4: {
                s32 animating;

                switch (self->step_s) {
                    case 0:
                        animating = AnimateEntity(D_us_80180E5C, self);
                        if (*(s32*) ((u8*) self + 0x50) == 1) {
                            g_api_PlaySfx(0x780);
                        }
                        if (animating != 0) {
                            return;
                        }
                        SetSubStep(1);
                        break;
                    case 1:
                        animating = AnimateEntity(D_us_80180E6C, self);
                        if (*(s32*) ((u8*) self + 0x50) == 1) {
                            g_api_PlaySfx(0x780);
                        }
                        if (animating != 0) {
                            return;
                        }
                        SetSubStep(2);
                        break;
                    case 2:
                        animating = AnimateEntity(D_us_80180E80, self);
                        if (*(s32*) ((u8*) self + 0x50) == 1) {
                            g_api_PlaySfx(0x780);
                        }
                        if (animating != 0) {
                            return;
                        }
                        SetSubStep(3);
                        break;
                    case 3:
                        if (AnimateEntity(D_us_80180E94, self) == 0) {
                            SetStep(2);
                        }
                        return;
                }

                g_api_PlaySfx(0x80A);
                g_api_func_80102CD8(1);
                {
                    s32 count = 0;
                    do {
                        Entity* newEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        count++;
                        if (newEntity != NULL) {
                            func_us_801A8DF0(0x38, self, newEntity);
                            newEntity->posY.i.hi = 0;
                            newEntity->posX.i.hi = self->posX.i.hi + (Random() & 0x7F);
                            newEntity->zPriority = self->zPriority + 2;
                        }
                    } while (count < 8);
                }
                return;
            }
            case 5:
                switch (self->step_s) {
                    case 0:
                        PlaySfxPositional(0x783);
                        ext->unk80 = 0x20;
                        self->step_s++;
                        /* fallthrough */
                    case 1:
                        AnimateEntity(D_us_80180DE0, self);
                        ext->unk80--;
                        if (ext->unk80 != 0) {
                            return;
                        }
                        SetSubStep(2);
                        return;
                    case 2:
                        if (AnimateEntity(D_us_80180DE8, self) == 0) {
                            SetStep(2);
                        }
                        return;
                }
                return;
            case 6:
                switch (self->step_s) {
                    case 0:
                        PlaySfxPositional(0x783);
                        self->hitboxState = 0;
                        self->step_s++;
                        /* fallthrough */
                    case 1:
                        AnimateEntity(D_us_80180DE0, self);
                        if (D_us_80180D48 & 0x20) {
                            self->step_s++;
                        }
                        break;
                    case 2:
                        self->scaleY = 0x100;
                        self->scaleX = 0x100;
                        self->drawFlags |= 3;
                        self->step_s++;
                        /* fallthrough */
                    case 3:
                        AnimateEntity(D_us_80180DE0, self);
                        if (self->scaleX < 0x81) {
                            self->scaleY--;
                            self->scaleX = self->scaleY;
                        }
                        break;
                }
                if (D_us_80180D48 & 8) {
                    DestroyEntity(self);
                }
                return;
        }
    }
}

extern void AnimateEntity(u8* anim, Entity* self);
extern void InitializeEntity(u16 params[]);
extern void DestroyEntity(Entity* entity);
extern u16 D_us_801805B8[]; // InitializeEntity params (step 0, func_us_801A3CD8)
extern s32 D_us_80180D48;   // shared boss-wide flag/bitmask (also gates func_us_801A4C0C)
extern Tilemap g_Tilemap;

typedef struct {
    u8* anim;      // +0x0: animation data pointer, passed directly to AnimateEntity
    u16 zPriority; // +0x4
    s16 offsetX;   // +0x6: X position offset, world-space (posX.i.hi = offsetX - scrollX.i.hi)
    s16 offsetY;   // +0x8: Y position offset, world-space (posY.i.hi = offsetY - scrollY.i.hi)
    // stride confirmed 0xC via params-indexed array access (2 bytes padding after offsetY)
} D_us_80180F4C_Entry;
extern D_us_80180F4C_Entry D_us_80180F4C[];

// Chain-segment entity, position/anim driven entirely by a per-params table
// (D_us_80180F4C): step 0 positions self at a fixed world offset (relative to
// current camera scroll) and sets zPriority from the table, then falls
// through into step 1's per-frame AnimateEntity call; step 1 advances the
// segment's animation each frame and, once D_us_80180D48's bit 2 is set
// (a shared "boss-wide" signal, name/purpose not otherwise confirmed),
// advances to step 2. Step 2 is a scale-based shrink effect: step_s==0
// waits for D_us_80180D48's bit 0x40 before decrementing; step_s==1 resets
// scaleX/scaleY to 0x100 (1.0) and sets drawFlags bits 0-1, then decrements
// immediately this same frame; step_s==2 decrements every frame;
// step_s>=3 stops decrementing entirely -- then re-runs AnimateEntity every
// frame regardless of step_s.
// Verified directly against the .s file (not m2c's raw output, which was
// unreliable here): both AnimateEntity calls take exactly 2 arguments
// (anim, self) -- m2c's "DestroyEntity(D_us_80180D48)" and a spurious 3rd
// AnimateEntity argument are both stale-register artifacts (confirmed: $a0
// at those call sites still holds the original `self` pointer / nothing
// extra is loaded into $a2 before the calls).
// Confidence: high on control flow and field offsets (all confirmed against
// include/game.h and the raw .s); D_us_80180D48's individual bit meanings
// (0x10/0x2/0x40) are inferred from usage only, not named.
void func_us_801A3CD8(Entity* self)
{
    D_us_80180F4C_Entry* entry;

    if (D_us_80180D48 & 0x10) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801805B8);
            entry = &D_us_80180F4C[self->params];
            self->zPriority = entry->zPriority;
            self->posX.i.hi = (s16)(entry->offsetX - g_Tilemap.scrollX.i.hi);
            self->posY.i.hi = (s16)(entry->offsetY - g_Tilemap.scrollY.i.hi);
            /* fallthrough */
        case 1:
            entry = &D_us_80180F4C[self->params];
            AnimateEntity(entry->anim, self);
            if (D_us_80180D48 & 2) {
                self->step++;
            }
            return;
        case 2:
            switch (self->step_s) {
                case 1:
                    self->scaleY = 0x100;
                    self->scaleX = 0x100;
                    self->drawFlags |= 3;
                    self->step_s++;
                    goto decrement_scale;
                case 0:
                    if (D_us_80180D48 & 0x40) {
                        self->step_s++;
                        goto decrement_scale;
                    }
                    break;
                default:
                    if (self->step_s != 2) {
                        break;
                    }
                decrement_scale:
                    if (self->scaleX >= 0x81) {
                        self->scaleY--;
                        self->scaleX = self->scaleY;
                    }
                    break;
            }
            entry = &D_us_80180F4C[self->params];
            AnimateEntity(entry->anim, self);
            break;
    }
}

extern u16 D_us_801805C4[];   // InitializeEntity params (step 0)
extern u8 D_us_80180ED8[];    // anim table for AnimateEntity
extern SVECTOR D_us_8019CA94; // fixed (zeroed) rotation-axis constant
extern SVECTOR D_us_80180FA0; // rotating-disc corner vectors for RotAverage4
extern SVECTOR D_us_80180FA8;
extern SVECTOR D_us_80180FB0;
extern SVECTOR D_us_80180FB8;
extern s32 AnimateEntity(u8* anim, Entity* self);
extern void SetStep(s32 step);
extern void MoveEntity(void);
extern void UnkPolyFunc2(Primitive* prim);
extern void PlaySfxPositional(s32 sfxId);

typedef struct {
    Primitive* prim; // ext+0x0: base of the 2-quad GT4 chain allocated at step 2
    u16 introTimer;   // ext+0x4: one-shot delay counter, set to 0x20 at step 0
    u8 pad6[0x1C];     // ext+0x6-0x1F (unused by this function)
    Entity* parentBoss; // ext+0x20 (abs 0x9C): set by whoever spawns this
                          // entity (not in this function) -- read-only here
} ET_801A3EE0;

// Scylla's spinning heraldic-disc projectile: aborts immediately if
// D_us_80180D48 bit 1 is set (a shared "boss defeated/interrupt" flag also
// checked by func_us_801A16E4/func_us_801A6568/func_us_801A6370). step 0/1
// is an intro: step 0 seeds a 0x20-frame delay and shrinks scaleX/Y to 0;
// step 1 waits out that delay, then animates and grows scaleX/Y back up by 8
// each frame until it reaches 0x100 (full size), then watches
// parentBoss->ext+0x9 (a flag on the boss that spawned this) and advances to
// step 2 once set (i.e. once Scylla signals the throw). step 2 aims the disc
// at the player, allocates 2 GT4 quads for a properly-lit rotating sprite,
// and falls straight into step 3's per-frame body: spins the disc (GTE
// rotation matrix around Z driven by self->rotate, fixed X-tilt, projected
// via RotAverage4 against 4 fixed corner vectors), plays a periodic
// whoosh/spin sfx, and self-destroys once it flies off either edge of the
// screen (posX.i.hi wraps past 0x100 moving right, or goes negative moving
// left).
// Confidence: high on control flow and GTE calls (already fully typed by
// m2c against psxsdk/libgte.h); parentBoss->ext+0x9's exact meaning is
// inferred from usage only (gates the intro-to-throw transition), not
// independently confirmed against a named source.
void func_us_801A3EE0(Entity* self)
{
    SVECTOR rotAxis;
    MATRIX m;

    rotAxis = D_us_8019CA94;

    if (D_us_80180D48 & 2) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801805C4);
            ((ET_801A3EE0*) &self->ext)->introTimer = 0x20;
            self->drawFlags = 3;
            self->scaleY = 0;
            self->scaleX = 0;
            /* fallthrough */
        case 1: {
            ET_801A3EE0* ext = (ET_801A3EE0*) &self->ext;

            if (ext->introTimer != 0) {
                ext->introTimer--;
                return;
            }
            AnimateEntity(D_us_80180ED8, self);
            if (self->scaleX < 0x100) {
                s16 scale = (u16) self->scaleY + 8;
                self->scaleY = scale;
                self->scaleX = scale;
            }
            if (*((u8*) ext->parentBoss + 0x9) != 0) {
                SetStep(2);
                return;
            }
            break;
        }
        case 2: {
            ET_801A3EE0* ext = (ET_801A3EE0*) &self->ext;
            s16 allocIdx;
            s16 angle = (s16) ratan2(PLAYER_posY_i_hi - self->posY.i.hi, PLAYER_posX_i_hi - self->posX.i.hi);

            self->animCurFrame = 0;
            self->zPriority = 0x8C;
            self->velocityX = rcos((s32) angle) * 0x48;
            self->velocityY = rsin((s32) angle) * 0x48;
            allocIdx = g_api_AllocPrimitives(PRIM_GT4, 2);
            if (allocIdx == -1) {
                DestroyEntity(self);
                return;
            }
            {
                Primitive* prim = &g_PrimBuf[allocIdx];

                self->primIndex = (s32) allocIdx;
                ext->prim = prim;
                self->flags |= 0x800000;
                UnkPolyFunc2(prim);
                prim->tpage = 0x12;
                prim->clut = 0x203;
                prim->u2 = prim->u0 = 0x60;
                prim->u3 = prim->u1 = 0x7F;
                prim->v1 = prim->v0 = 0xE0;
                prim->v3 = prim->v2 = 0xFF;
                prim->drawMode = 2;
                prim->priority = self->zPriority;
            }
            self->step++;
            /* fallthrough */
        }
        case 3: {
            ET_801A3EE0* ext = (ET_801A3EE0*) &self->ext;
            Primitive* prim = ext->prim;
            s32 transOffset[3];
            long unusedP, unusedFlag;

            MoveEntity();
            self->rotate += 0x280;
            SetGeomScreen(0x400);
            SetGeomOffset((s32) self->posX.i.hi, (s32) self->posY.i.hi);
            RotMatrix(&rotAxis, &m);
            RotMatrixZ((s32) (s16) self->rotate, &m);
            RotMatrixX(0x2C0, &m);
            SetRotMatrix(&m);
            transOffset[0] = 0;
            transOffset[1] = 0;
            transOffset[2] = 0x400;
            TransMatrix(&m, (VECTOR*) transOffset);
            SetTransMatrix(&m);
            // each RotAverage4 output writes a packed 32-bit {x,y} pair in one
            // shot -- x0/y0, x1/y1, x2/y2, x3/y3 are adjacent fields in
            // Primitive, matching the GTE's packed sxy write exactly (not a
            // stray cast: this is how the real library call is meant to be used)
            RotAverage4(&D_us_80180FA0, &D_us_80180FA8, &D_us_80180FB0, &D_us_80180FB8,
                        (long*) &prim->x0, (long*) &prim->x1, (long*) &prim->x2, (long*) &prim->x3,
                        &unusedP, &unusedFlag);
            if (!((g_Timer + 2) & 7)) {
                PlaySfxPositional(0x6C7);
            }
            if (self->velocityX > 0) {
                if (self->posX.i.hi < 0x101) {
                    return;
                }
            } else if (self->posX.i.hi >= 0) {
                break;
            }
            DestroyEntity(self);
            return;
        }
    }
}

typedef struct {
    Primitive* prim;      // ext+0x0: head of the whole trail-segment chain
    u16 timer;             // ext+0x4 (abs 0x80): multi-purpose countdown --
                             // launch delay (steps 0/1), then time-to-impact
                             // (step 2), then segment-spacing counter
    u8 pad6[0x1E];          // ext+0x6-0x23 (unused by this function)
    Primitive* curSegment;  // ext+0x24 (abs 0xA0): trail segment currently
                             // being stretched/dragged behind the entity
} ET_801A42A8;

// Scylla's arcing acid/ink-spit trail attack: aborts immediately if
// D_us_80180D48 bit 1 is set (same shared boss-interrupt flag as
// func_us_801A16E4/801A3EE0/etc). step 0 allocates a whole chain of prims
// via g_api_func_800EDB58 (not g_api_AllocPrimitives -- same idiom as
// func_us_801A27EC), paints them all a dark blue-ish ink color and hides
// them (drawMode 8), then seeds a random launch delay scaled by params.
// step 1 waits out that delay, then launches on a ballistic arc toward a
// fixed X target (0x5C, screen-relative) and starts the first trail segment
// at the current position. step 2 is a small sub-state machine (step_s):
// free-fall under gravity until velocityY turns positive, then one-shot
// recomputes velocityY so the entity lands exactly on a fixed Y target
// (0x54) by the time the timer expires, then just falls. Every frame in
// step 2, the CURRENT segment's leading edge is dragged to the entity's
// live position (stretching it); every 4th frame a fresh segment is started
// where the previous one left off (chaining), and once the timer expires,
// advances to step 3. step 3 walks the whole chain fading each visible
// segment's color via the already-existing func_us_801A2A90(prim, 1)
// helper, hides any that finish fading, and self-destroys once the entire
// chain has faded out.
// Confidence: high on control flow and ext offsets (re-verified 0x80/0xA0
// directly against the .s); func_us_801A2A90's role confirmed by reading
// its own already-decompiled body earlier in this file rather than guessing.
// Dropped a phantom `Random(0x98)`-style argument in step 0's delay calc,
// same pattern as func_us_801A5F0C/801A6xxx -- delay slot before the `jal
// Random` is a plain nop, so it takes no real argument here either.
void func_us_801A42A8(Entity* self)
{
    ET_801A42A8* ext = (ET_801A42A8*) &self->ext;

    if (D_us_80180D48 & 2) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0: {
            s16 allocIdx = g_api_func_800EDB58(0x12, 0x12);

            InitializeEntity(D_us_80180564);
            if (allocIdx != -1) {
                Primitive* prim = &g_PrimBuf[allocIdx];

                self->primIndex = (s32) allocIdx;
                ext->prim = prim;
                self->flags |= 0x800000;
                while (prim != NULL) {
                    prim->r0 = 0x20;
                    prim->g0 = 0x28;
                    prim->b0 = 0xC0;
                    prim->r1 = 0x10;
                    prim->g1 = 0x10;
                    prim->b1 = 0x60;
                    prim->priority = 0x98;
                    prim->drawMode = 8;
                    prim = prim->next;
                }
                ext->timer = (Random() & 7) * self->params;
            }
            /* fallthrough */
        }
        case 1: {
            if (ext->timer != 0) {
                ext->timer--;
                return;
            }
            {
                s32 dx = (0x5C - g_Tilemap.scrollX.i.hi) - self->posX.i.hi;
                s32 rounded = dx << 0x10;
                Primitive* prim = ext->prim;
                s16 x = self->posX.i.hi;
                s16 y = self->posY.i.hi;

                if (dx & 0x8000) {
                    rounded += 0x1F;
                }
                self->velocityX = rounded >> 5;
                self->velocityY = -0x84000;
                ext->curSegment = prim;
                prim->x1 = prim->x0 = x;
                prim->drawMode = 0x33;
                prim->y1 = prim->y0 = y;
                ext->timer = 0x20;
                self->step++;
            }
            /* fallthrough */
        }
        case 2: {
            Primitive* segment;

            switch (self->step_s) {
                case 0:
                    MoveEntity();
                    self->velocityY += 0x4000;
                    if (self->velocityY > 0) {
                        self->step_s++;
                    }
                    break;
                case 1:
                    self->step_s++;
                    self->velocityY = (s32) (((0x54 - g_Tilemap.scrollY.i.hi) - (s16) self->posY.i.hi) << 0x10) / (s16) ext->timer;
                    /* fallthrough */
                case 2:
                    MoveEntity();
                    break;
            }

            segment = ext->curSegment;
            segment->x0 = (s16) (u16) self->posX.i.hi;
            segment->drawMode = 0x33;
            segment->y0 = (s16) self->posY.i.hi;
            ext->timer--;
            if (ext->timer == 0) {
                self->step++;
            } else if (!(ext->timer & 3)) {
                Primitive* next = segment->next;
                s16 x = self->posX.i.hi;
                s16 y = self->posY.i.hi;

                ext->curSegment = next;
                next->x1 = next->x0 = x;
                next->drawMode = 0x33;
                next->y1 = next->y0 = y;
            }
            /* fallthrough */
        }
        case 3: {
            Primitive* prim = ext->prim;
            s32 allFaded = 1;

            if (prim->next != NULL) {
                do {
                    if (prim->drawMode != 8) {
                        if (func_us_801A2A90((u8*) prim, 1) == 0) {
                            prim->drawMode = 8;
                        } else {
                            allFaded = 0;
                        }
                    }
                    prim = prim->next;
                } while (prim->next != NULL);
            }
            prim->y1 = 0;
            prim->y0 = 0;
            prim->x1 = 0;
            prim->x0 = 0;
            prim->drawMode = 0x33;
            if (allFaded) {
                DestroyEntity(self);
            }
            break;
        }
    }
}

extern void PlaySfxPositional(s32 sfxId);
extern void SetStep(s32 step);
extern s32 AnimateEntity(u8* anim, Entity* self);
extern s16 PLAYER_posX_i_hi;
extern s16 PLAYER_posY_i_hi;
extern u16 D_us_801805D0[]; // InitializeEntity params (step 0)
extern u8 D_us_80180F0C[];  // anim table for AnimateEntity (step 1)
extern u8 D_us_80180F18[];  // anim table for AnimateEntity (steps 2/3)
extern u8 D_us_80180F30[];  // anim table for AnimateEntity (step 4)
extern u8 D_us_80180F40[];  // anim table for AnimateEntity (steps 5/6)

typedef struct {
    u8 pad0[4];  // ext+0x0-0x3 (unused by this function)
    u16 timer;   // ext+0x4 (0x80): countdown reused across steps 2/3
} ET_801A4680;

// Scylla eye-projectile: if D_us_80180D48 bit 1 is set (shared trigger, same mask
// as the water-level/music functions), or once already flagged (self->flags &
// 0x100), shatters into 5 companion entities (type 0x39) and destroys self --
// checked every call, ahead of the normal step machine, so it can interrupt any
// state. Otherwise runs a 7-state sequence: 0/1 telegraph animation, then once
// it finishes, launches upward and starts a `timer` countdown (step 2); 2/3 an
// arcing rise-then-fall (velocity damped each frame, then a sine wave once the
// timer expires) with the timer reused as a fixed-length phase gate; 4 waits out
// a third animation; 5 aims at and launches toward the player (velocityX/Y from
// rcos/rsin of the angle to PLAYER_pos), then 6 just animates/moves while
// travelling. self->pose==3 during steps 0/1 triggers an extra one-shot sfx
// (behavior/purpose of that specific pose value not independently confirmed).
// Confidence: high on control flow and Entity fields (self->step/pose/flags/
// hitboxState/velocityX/velocityY/rotate all confirmed via game.h); medium on
// D_us_80180D48's exact bit semantics (inferred from usage, shared with other
// already-decompiled functions in this file) and the ext `timer` field's role
// across steps 2/3 (behavior-inferred only).
void func_us_801A4680(Entity* self)
{
    ET_801A4680* ext = (ET_801A4680*) &self->ext;

    if (D_us_80180D48 & 2) {
        self->flags |= 0x100;
    }

    if (self->flags & 0x100) {
        s32 i;
        for (i = 0; i < 5; i++) {
            Entity* piece = AllocEntity(g_Entities_224, (Entity*)((u8*) g_Entities_224 + 0x1780));
            if (piece != NULL) {
                func_us_801A8DF0(0x39, self, piece);
                piece->params = 1;
                piece->zPriority = self->zPriority;
            }
        }
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801805D0);
            self->zPriority = 0x8C;
            self->hitboxState = 0;
            /* fallthrough */
        case 1:
            if (AnimateEntity(D_us_80180F0C, self) == 0) {
                self->velocityY = -0x40000;
                ext->timer = 0x50;
                self->hitboxState = 2;
                SetStep(2);
            }
            if (self->pose == 3) {
                PlaySfxPositional(0x66E);
            }
            return;

        case 2: {
            s32 velocityY = self->velocityY;
            s32 damp = velocityY >> 4;

            AnimateEntity(D_us_80180F18, self);
            MoveEntity();
            self->velocityY = velocityY - damp;
            if (--ext->timer == 0) {
                ext->timer = 0x80;
                self->rotate = (s16)(Random(damp) * 0x10);
                self->step++;
                return;
            }
            break;
        }

        case 3:
            AnimateEntity(D_us_80180F18, self);
            MoveEntity();
            self->rotate += 0x20;
            self->velocityY = rsin(self->rotate) * 8;
            if (--ext->timer == 0) {
                SetStep(4);
                return;
            }
            break;

        case 4:
            if (AnimateEntity(D_us_80180F30, self) == 0) {
                SetStep(5);
                return;
            }
            break;

        case 5: {
            s16 angle = (s16) ratan2(PLAYER_posY_i_hi - self->posY.i.hi, PLAYER_posX_i_hi - self->posX.i.hi);

            self->velocityX = rcos(angle) * 0x70;
            self->velocityY = rsin(angle) * 0x70;
            self->facingLeft = (self->velocityX > 0) ? 0 : 1;
            self->hitboxState = 3;
            PlaySfxPositional(0x625);
            self->step++;
            /* fallthrough */
        }
        case 6:
            AnimateEntity(D_us_80180F40, self);
            MoveEntity();
            break;
    }
}

extern s32 D_us_80180D48; // shared boss-wide flag/bitmask (also gates func_us_801A2A90/801A3CD8)
extern Entity g_Entities_224[];
extern void* AllocEntity(Entity* pool, Entity* poolEnd);
extern void func_us_801A8DF0(s32 type, Entity* parent, Entity* newEntity); // boss/bo3/create_entity.c, still a stub
extern void PlaySfxPositional(s32 sfxId);
extern void DestroyEntity(Entity* entity);
extern void InitializeEntity(u16 params[]);
extern s32 AnimateEntity(u8* anim, Entity* self);
extern void SetStep(s32 step);
extern void MoveEntity(void);
extern s32 GetAngleBetweenEntities(Entity* from, Entity* to);
extern s16 LimitAngleChange(s16 maxDelta, s16 current, s16 target);
extern s16 PLAYER_posX_i_hi;
extern s16 PLAYER_posY_i_hi;
extern u16 D_us_801805DC[];
extern u8 D_us_80180EF4[];
extern u8 D_us_80180EE4[];

typedef struct {
    u8 pad[0x10];    // ext+0x0-0xF (0x7C-0x8B), unused by this function
    s16 targetAngle; // ext+0x10 (0x8C): homing-projectile aim angle
} ET_801A4988;

/*
 * Scylla ranged "spit" projectile. If D_us_80180D48 bit 0x2 is set (a
 * shared boss-wide flag, name/purpose not otherwise confirmed -- also gates
 * func_us_801A2A90/801A3CD8 elsewhere in this file), or once already
 * flagged, the projectile bursts into 16 debris entities (type 0x39, from
 * the shared g_Entities_224 pool) and destroys itself, playing SFX 0x6E3.
 * Otherwise runs its own small step machine: step 0/1 play a "flight" anim
 * until it finishes, then step 2 aims at the player (angle clamped to
 * [-0x180, 0x200]) and sets velocity via rcos/rsin*0x50, then step 3 plays
 * a second anim, moves, and re-aims each frame toward g_Entities[0] (the
 * boss' own root/anchor entity, not the player -- confirmed directly by the
 * call argument) via LimitAngleChange, easing the stored angle (ext+0x10)
 * by at most 8 units/frame.
 * Confidence: high on control flow and arithmetic (both directly confirmed
 * against the asm); medium on D_us_80180D48's exact meaning and on why
 * step 3 homes on g_Entities[0] rather than the player.
 */
void func_us_801A4988(Entity* self)
{
    ET_801A4988* ext = (ET_801A4988*)&self->ext;
    s32 i;
    Entity* newEntity;
    s32 angle;

    if (D_us_80180D48 & 2) {
        self->flags |= 0x100;
    }

    if (self->flags & 0x100) {
        for (i = 0; i < 0x10; i++) {
            newEntity = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (newEntity != NULL) {
                func_us_801A8DF0(0x39, self, newEntity);
                newEntity->zPriority = self->zPriority;
            }
        }
        PlaySfxPositional(0x6E3);
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801805DC);
            self->blendMode = 0x30;
            self->hitPoints = 0;
            // fallthrough
        case 1:
            if (!AnimateEntity(D_us_80180EF4, self)) {
                SetStep(2);
            }
            return;

        case 2:
            angle = ratan2(PLAYER_posY_i_hi - self->posY.i.hi, PLAYER_posX_i_hi - self->posX.i.hi);
            if ((s16)angle >= 0x201) {
                angle = 0x200;
            }
            if ((s16)angle < -0x180) {
                angle = -0x180;
            }
            self->velocityX = rcos(angle) * 0x50;
            self->velocityY = rsin(angle) * 0x50;
            ext->targetAngle = (s16)angle;
            PlaySfxPositional(0x655);
            self->step++;
            // fallthrough
        case 3: {
            s16 newAngle;

            AnimateEntity(D_us_80180EE4, self);
            MoveEntity();
            newAngle = LimitAngleChange(8, ext->targetAngle, GetAngleBetweenEntities(self, g_Entities));
            self->velocityX = rcos(newAngle) * 0x50;
            self->velocityY = rsin(newAngle) * 0x50;
            ext->targetAngle = newAngle;
            break;
        }
    }
}

extern void* AllocEntity(Entity* pool, Entity* poolEnd);
extern s32 UnkCollisionFunc3(s16 list[]);
extern void PlaySfxPositional(s32 sfxId);
extern void func_us_801A8DF0(s32 type, Entity* parent, Entity* newEntity); // boss/bo3/create_entity.c, still a stub
extern Entity g_Entities_224[];

extern u16 D_us_801805E8[]; // InitializeEntity params (step 0, func_us_801A4C0C)
extern s16 D_us_80180F88[]; // per-params Y offset added to posY.i.hi+scrollY.i.hi ground check (indexed by self->params)
extern s16 D_us_80180F90[]; // collision probe list passed to UnkCollisionFunc3 (terminator-based, see UnkCollisionFunc3.s)
extern char D_us_8019CABC[]; // debug string printed via FntPrint if the splash-entity alloc fails

/*
 * unkA4 (ext+0x28): "ground splash already spawned" one-shot flag, confirmed
 * via .s: read (lbu) before the ground-check block, set to 1 right after,
 * gating a second func_us_801A8DF0(0x17, ...) splash spawn so it only fires
 * once per instance.
 */
typedef struct {
    u8 unk0[0x28];
    u8 splashSpawned; // absolute 0xA4
} ET_801A4C0C;

/*
 * Confidence: HIGH. Fully cross-checked against func_us_801A4C0C.s.
 * Two m2c artifacts corrected:
 *  - MoveEntity() takes NO arguments (confirmed via MoveEntity.s: it reads
 *    g_CurrentEntity and updates its own posX/posY from velocityX/velocityY
 *    -- $a0 in the disassembly just happens to still hold self->posY.i.hi
 *    from an earlier comparison and is never used by the callee).
 *  - UnkCollisionFunc3 takes the array itself, not its address (confirmed
 *    via e_fake_ralph.c's `UnkCollisionFunc3(D_us_80180958)` where
 *    D_us_80180958 is a plain `static s16[]`); m2c's `&D_us_80180F90` is
 *    the address-of-array-decays-to-same-pointer artifact.
 * The .Lus_801A4C74/.Lus_801A4DE8 labels are reused as a shared tail (both
 * the "already flagged for despawn" path and the "collision probe hit"
 * path jump into the same func_us_801A8DF0(2, self, new)/DestroyEntity
 * sequence), reproduced here with goto to match the real control flow.
 */
void func_us_801A4C0C(Entity* self)
{
    Entity* newEntity;
    s16 posY;

    if (D_us_80180D48 & 2) {
        self->flags |= 0x100;
    }
    if (self->flags & 0x100) {
        newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
        if (newEntity == NULL) {
            goto destroy;
        }
        goto spawn_and_destroy;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801805E8);
            self->drawFlags = 4;
            self->params = Random() & 3;
            self->animCurFrame = (s16) (self->params + 0x6F);
            self->rotate = (u16) (rand() & 0xFFF);
            PlaySfxPositional(0x644);
            /* fallthrough */
        case 1:
            posY = self->posY.i.hi;
            self->rotate += 0x80;
            if (posY < 0x80) {
                MoveEntity();
                self->velocityY += 0x2000;
                return;
            }
            if (posY + D_us_80180F88[self->params] + g_Tilemap.scrollY.i.hi >= 0xD9) {
                if (((ET_801A4C0C*) &self->ext)->splashSpawned == 0) {
                    ((ET_801A4C0C*) &self->ext)->splashSpawned = 1;
                    newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                    if (newEntity != NULL) {
                        func_us_801A8DF0(0x17, self, newEntity);
                        newEntity->params = 0;
                        newEntity->posY.i.hi = (s16) (0xD8 - g_Tilemap.scrollY.i.hi);
                    }
                }
            }
            if (UnkCollisionFunc3(D_us_80180F90) & 1) {
                newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (newEntity == NULL) {
                    FntPrint(D_us_8019CABC);
                    goto destroy;
                }
            spawn_and_destroy:
                func_us_801A8DF0(2, self, newEntity);
                newEntity->params = 0x11;
            destroy:
                DestroyEntity(self);
                return;
            }
            self->velocityY -= 0x2000;
            return;
    }
}

extern u16 D_us_801805F4[]; // InitializeEntity params
extern u8 D_us_80180F24[]; // anim table for AnimateEntity

typedef struct {
    u8 pad0[4];   // ext+0x0-3 (unused by this function)
    s16 lifetime; // ext+0x4
} ET_801A4E24;

/*
 * Confidence: medium (m2c output only, not independently .s-verified).
 * Debris/spark particle: step 0 randomizes a scale-shrink lifetime
 * (scaleX=scaleY, range depends on params), a random facing, and an
 * initial velocity via rcos/rsin of a random angle (halved if
 * params!=0), applying one frame of that velocity immediately (posX +=
 * velocityX*4 etc, a "warm start" so the particle doesn't appear
 * stationary on its spawn frame). Falls through into step 1, which
 * animates + moves every frame and grows velocityY toward a cap
 * (0x2FFFF) by 0x1400/frame (gravity).
 */
void func_us_801A4E24(Entity* self)
{
    ET_801A4E24* ext = (ET_801A4E24*) &self->ext;
    s16 size;

    switch (self->step) {
        case 0: {
            s32 speed;
            s16 angle;

            InitializeEntity(D_us_801805F4);
            self->drawFlags = 3;
            if (self->params == 0) {
                size = (s16)((Random() & 0x7F) + 0x40);
            } else {
                size = (s16)((Random() & 0x3F) + 0x20);
            }
            self->scaleY = size;
            self->scaleX = size;
            self->blendMode = 0x10;
            self->facingLeft = (u16)(Random() & 1);

            speed = (Random() & 0x1F) + 0x10;
            angle = (s16)(Random() * 6 + 0x900);
            self->velocityX = (speed * rcos(angle)) >> 1;
            self->velocityY = speed * rsin(angle);
            if (self->params != 0) {
                self->velocityX >>= 1;
                self->velocityY >>= 1;
            }
            self->posX.val += self->velocityX * 4;
            self->posY.val += self->velocityY * 4;
            ext->lifetime = (s16)((Random() & 0x1F) + 0x10);
            /* fallthrough */
        }
        case 1:
            AnimateEntity(D_us_80180F24, self);
            MoveEntity();
            if (self->velocityY <= 0x2FFFF) {
                self->velocityY += 0x1400;
            }
            return;
    }
}

typedef struct {
    u8* firstPrim; // ext+0x0: head of this entity's Primitive chain
} ET_801A4FB8;

/*
 * Chain-link quad position propagator. Walks 12 consecutive Entity slots
 * starting at g_CurrentEntity+1 (treated as an array of chain-segment
 * entities laid out contiguously -- confirmed via the two walking pointers
 * advancing by sizeof(Entity)==0xBC each iteration, NOT a self->nextPart
 * linked list) in lockstep with the Primitive chain stored raw at
 * g_CurrentEntity->ext+0 (same "ext holds a Primitive* chain head"
 * convention as func_us_8019F818/func_us_801A27EC). Operates purely on
 * g_CurrentEntity (no entity parameter), like MoveEntity()/FallEntity().
 * For each pair of consecutive entities it computes the angle between them
 * (ratan2), offsets it by +/-0x400 (90 degrees, sign per
 * g_CurrentEntity->params) to get a perpendicular direction, and writes two
 * edge points (midpoint +/- cos/sin) into the current Primitive's
 * unk8/0xA/0x20/0x22 fields, then advances to the next Primitive and writes
 * the SAME midpoint into its unk14/0x16/0x2C/0x2E fields so consecutive
 * quads share an edge (a rope/chain rendered as a strip of quads).
 * Confidence: high on control flow (cross-checked against
 * func_us_801A4FB8.s throughout, including the a1=0x8000-style constant
 * folding pattern -- here the round-toward-zero right-shift-by-17 midpoint
 * divide is expressed as plain integer division by 0x20000, matching this
 * codebase's established convention of trusting C's rounding for such
 * cases). unkE (CLUT/texture-page selector, taken from
 * g_CurrentEntity->palette when flags&0xF, else a fixed 0x205) is inferred
 * from context, not independently verified.
 */
void func_us_801A4FB8(void)
{
    Entity* next = g_CurrentEntity + 1;
    u8* prim = ((ET_801A4FB8*) &g_CurrentEntity->ext)->firstPrim;
    s32 x0 = g_CurrentEntity->posX.val;
    s32 y0 = g_CurrentEntity->posY.val;
    s32 i;

    *(s32*)(prim + 0x2C) = x0;
    *(s32*)(prim + 0x14) = x0;
    *(s32*)(prim + 0x2E) = y0;
    *(s32*)(prim + 0x16) = y0;
    *(u16*)(prim + 0x32) = 8;

    for (i = 0; i < 0xC; i++) {
        s32 angle = ratan2(next->posY.val - y0, next->posX.val - x0);
        s16 perpAngle;
        s32 midX, midY, cosV, sinV;
        u16 unkE;

        if (g_CurrentEntity->params != 0) {
            perpAngle = (s16)(angle - 0x400);
        } else {
            perpAngle = (s16)(angle + 0x400);
        }

        midX = (x0 + next->posX.val) / 0x20000;
        midY = (y0 + next->posY.val) / 0x20000;
        cosV = (rcos(perpAngle) << 4) >> 12;
        sinV = (rsin(perpAngle) << 4) >> 12;

        *(s16*)(prim + 8) = (s16)(midX + cosV);
        *(s16*)(prim + 0xA) = (s16)(midY + sinV);
        *(s16*)(prim + 0x20) = (s16)(midX - cosV);
        *(s16*)(prim + 0x22) = (s16)(midY - sinV);

        if (g_CurrentEntity->flags & 0xF) {
            unkE = (u16)(g_CurrentEntity->palette & 0xFFF);
        } else {
            unkE = 0x205;
        }
        *(u16*)(prim + 0xE) = unkE;

        prim = *(u8**)(prim + 0);
        *(s16*)(prim + 0x14) = (s16)(midX + cosV);
        *(s16*)(prim + 0x16) = (s16)(midY + sinV);
        *(s16*)(prim + 0x2C) = (s16)(midX - cosV);
        *(s16*)(prim + 0x2E) = (s16)(midY - sinV);

        y0 = next->posY.val;
        x0 = next->posX.val;
        next++;
    }

    *(u16*)(prim + 0x32) = 8;
}

/*
 * Chain relaxation/smoothing pass, companion to func_us_801A4FB8's quad
 * renderer: walks 11 consecutive Entity slots starting at
 * g_CurrentEntity+1 (same contiguous-array convention as 801A4FB8, NOT a
 * nextPart list). Each segment's position eases toward an extrapolation
 * of the average of a running position and the position of the entity two
 * slots ahead. Confidence: high, cross-checked against
 * func_us_801A516C.s throughout.
 */
void func_us_801A516C(void)
{
    Entity* seg = g_CurrentEntity + 1;
    s32 runX = g_CurrentEntity->posX.val;
    s32 runY = g_CurrentEntity->posY.val;
    s32 i;

    for (i = 0; i < 0xB; i++) {
        s32 smoothX = (runX + (seg + 1)->posX.val) >> 1;
        s32 smoothY = (runY + (seg + 1)->posY.val) >> 1;

        runX = smoothX + ((smoothX - seg->posX.val) >> 2);
        runY = smoothY + ((smoothY - seg->posY.val) >> 2);
        seg->posX.val = runX;
        seg->posY.val = runY;
        seg++;
    }
}

extern s32 SquareRoot0(s32 value);

typedef struct {
    u8 pad0[6];  // ext+0x0-0x5 (unused by this function)
    u16 phase;   // ext+0x6 (self->unk82)
} ET_801A51E4;

/*
 * Chain wave/whip motion pass, companion to func_us_801A4FB8/801A516C:
 * walks 11 consecutive Entity slots starting at g_CurrentEntity+1 (same
 * contiguous-array convention). For each segment it computes the angle
 * from a running position to that segment's actual position (ratan2),
 * adds a per-frame oscillating wobble (rcos of a phase accumulator that
 * advances +0x180/frame, stored in g_CurrentEntity->ext+6), clamps the
 * distance to the segment to at most 0x20, and advances the running
 * position by (distance, angle) in polar form -- writing the new running
 * position into the segment's own posX/posY and the computed angle into
 * its rotate field. Confidence: high, cross-checked against
 * func_us_801A51E4.s throughout (this resolves m2c's "unk-0x1A" artifact:
 * it's simply the segment's posY, read/written via a second walking
 * pointer that the compiler happened to base near `rotate`).
 */
void func_us_801A51E4(void)
{
    Entity* seg = g_CurrentEntity + 1;
    ET_801A51E4* ext = (ET_801A51E4*) &g_CurrentEntity->ext;
    s32 runX = g_CurrentEntity->posX.val;
    s32 runY = g_CurrentEntity->posY.val;
    u16 phase = ext->phase;
    s32 i;

    for (i = 0; i < 0xB; i++) {
        s32 dx = seg->posX.val - runX;
        s32 dy = seg->posY.val - runY;
        s32 angle = ratan2(dy, dx);
        s32 cosWobble = (rcos((s16) phase) << 8) >> 12;
        s16 angleAdj = (s16)(angle + cosWobble);
        s32 dxInt = dx >> 16;
        s32 dyInt = dy >> 16;
        s32 dist = SquareRoot0(dxInt * dxInt + dyInt * dyInt);

        if (dist >= 0x21) {
            dist = 0x20;
        }

        runX += (dist * rcos(angleAdj)) << 4;
        seg->posX.val = runX;
        seg->rotate = (u16) angleAdj;
        runY += (dist * rsin(angleAdj)) << 4;
        seg->posY.val = runY;

        phase += 0x180;
        seg++;
    }
}

extern void SetStep(s32 step);
extern s32 GetDistanceToPlayerX(void);
extern u16 D_us_80180600[]; // InitializeEntity params (step 0, func_us_801A57A4), shared here (step 0)

typedef struct {
    Primitive* prim; // ext+0x0: head of the 13-quad Primitive chain (body texture strip)
    u16 unk80;        // ext+0x4: shared countdown timer (step-1 approach/arm wait, step-3 re-arm timer)
    u16 phase;         // ext+0x6: chain wave-motion phase accumulator (see func_us_801A51E4)
    u8 unk84;           // ext+0x8: body-segment walk counter, used in the step-3 death sequence
    u8 unk85;            // ext+0x9: NOT used on self; written on a sibling segment's own ext (see
                         // func_us_801A57A4's identically-offset "unk85" arm/advance flag)
    u8 unk86;             // ext+0xA: "waiting" flag toggled on/off in step 1
} ET_801A5338;

/*
 * Scylla body-chain head: spawns 13 render quads plus 11 body-segment
 * entities (type 0x3B, func_us_801A57A4) and 1 tail entity (type 0x3C,
 * func_us_801A5948) at self+1..self+12 (contiguous g_Entities_224 slots,
 * same convention as func_us_801A4FB8/801A516C/801A51E4, which this
 * function calls every frame in steps 2-3 to render/relax/animate the
 * chain). step 0: one-time setup (falls through into step 1's body,
 * self->step is never written here so it stays 0 across the fallthrough).
 * step 1: waits either for the player to approach (params==0,
 * GetDistanceToPlayerX) or for a fixed countdown (params!=0), then
 * SetStep(2). steps 2/3 both drive the chain passes each frame; step 3
 * additionally advances a step_s sub-state machine (0: arm death sequence
 * + castle-flag bookkeeping, 1: countdown then arm one body segment's
 * ext.unk85 flag per expiry (walking tail-to-head as unk84 counts down),
 * 2: destroys all 12 chain entities plus self). A separate one-time check
 * at the top of every call promotes step 2 to step 3 once a shared flag
 * (D_us_80180D48 bit 1) latches self->flags bit 0x100.
 * Confidence: high on control flow, cross-checked instruction-by-instruction
 * against func_us_801A5338.s; medium on the exact gameplay meaning of
 * D_us_80180D48/D_us_80180C66/g_CastleFlags[0xC5] (same shared globals used
 * elsewhere in this file, meaning inferred from usage only).
 */
void func_us_801A5338(Entity* self)
{
    ET_801A5338* ext = (ET_801A5338*) &self->ext;

    if ((self->params != 0) && (D_us_80180D48 & 2)) {
        self->flags |= 0x100;
    }
    if (self->flags & 0x100) {
        if (self->step == 2) {
            SetStep(3);
        }
    }

    switch (self->step) {
        case 0: {
            Primitive* p;
            Entity* seg;
            s32 uBase;
            s32 i;
            s32 primIdx;

            InitializeEntity(D_us_80180600);
            primIdx = g_api_AllocPrimitives(4, 0xD);
            if (primIdx == -1) {
                DestroyEntity(self);
                return;
            }
            p = &g_PrimBuf[primIdx];
            self->primIndex = primIdx;
            self->ext.prim = p;
            self->flags |= 0x800000;

            uBase = -1;
            for (i = 0; i < 0xD; i++) {
                p->u0 = p->u2 = (u8) uBase;
                p->u1 = p->u3 = (u8)(uBase - 0xE);
                p->v0 = p->v1 = 0x60;
                p->v2 = p->v3 = 0x7F;
                p->tpage = 0x13;
                p->clut = 0x205;
                p->priority = 0x88;
                p->drawMode = 2;
                uBase -= 0xE;
                p = p->next;
            }

            if (self->params == 0 && g_CastleFlags[0xC5] != 0) {
                DestroyEntity(self);
                return;
            }

            self->nextPart = self + 1;
            self->parent = NULL;

            seg = self + 1;
            for (i = 0; i < 0xB; i++) {
                func_us_801A8DF0(0x3B, self, seg);
                seg->parent = self;
                seg->nextPart = seg + 1;
                seg->params = i & 3;
                seg = seg + 1;
            }

            func_us_801A8DF0(0x3C, self, self + 12);
            (self + 12)->params = self->params;
            (self + 12)->parent = self;
            (self + 12)->nextPart = self;
            (self + 12)->zPriority = self->zPriority + 1;
            ext->unk80 = self->params * 3 * 0x10;
        }
        /* fallthrough */
        case 1: {
            s32 dist;

            if (self->params == 0) {
                ext->unk86 = 1;
                dist = GetDistanceToPlayerX();
                if (dist < 0xC0) {
                    ext->unk86 = 0;
                    D_us_80180C66 |= 0xE;
                    SetStep(2);
                }
            } else {
                ext->unk86 = 1;
                ext->unk80--;
                if (ext->unk80 == 0) {
                    ext->unk86 = 0;
                    SetStep(2);
                }
            }
            return;
        }
        case 2:
            func_us_801A516C();
            func_us_801A51E4();
            func_us_801A4FB8();
            ext->phase += 0x40;
            return;
        case 3: {
            Primitive* p;
            Entity* seg;
            s32 i;

            func_us_801A516C();
            func_us_801A51E4();
            func_us_801A4FB8();
            ext->phase += 0x80;

            if (self->step_s != 1) {
                if (self->step_s >= 2) {
                    if (self->step_s == 2) {
                        seg = self + 1;
                        for (i = 0; i < 0xC; i++) {
                            DestroyEntity(seg);
                            seg = seg + 1;
                        }
                        DestroyEntity(self);
                    }
                    return;
                }

                ext->unk84 = 0xB;
                ext->unk80 = 1;
                if (self->params == 0) {
                    g_CastleFlags[0xC5] |= 1;
                    D_us_80180C66 &= 0xFFF1;
                }
                self->step_s++;
            }

            ext->unk80--;
            if (ext->unk80 != 0) {
                return;
            }

            seg = self + 1;
            p = self->ext.prim;
            if (ext->unk84 > 0) {
                ext->unk80 = 0x10;
                for (i = 0; i < ext->unk84; i++) {
                    p = p->next;
                    seg = seg + 1;
                }
            }
            p = p->next;
            p->drawMode = 8;
            ((ET_801A5338*) &seg->ext)->unk85 = 1;

            ext->unk84--;
            if (ext->unk84 == 0) {
                self->step_s++;
            }
            return;
        }
    }
}

typedef struct {
    u8 pad0[4];   // ext+0x0-0x3 (unused by this function)
    u16 unk80;    // ext+0x4: countdown of hits-remaining before step_s advances
    u8 pad1[3];   // ext+0x6-0x8 (unused by this function)
    u8 unk85;     // ext+0x9: "done"/advance-to-step-2 flag, checked in step 1
} ET_801A57A4;

/*
 * Confidence: high, fully cross-checked against func_us_801A57A4.s.
 * step 0: one-time init (animSet=2, animCurFrame=0, unk5A=0, palette=0),
 * falls through into step 1's body.
 * step 1: hitboxState toggles on/off in sync with (g_Timer&3)==params (a
 * blinking/phased-hitbox pattern), and once ext->unk85 is set elsewhere,
 * advances to step 2 via SetStep(2).
 * step 2: step_s==0 arms a 0x10-hit countdown (hitboxState=0, unk80=0x10,
 * step_s=1) and falls into the step_s==1 body; step_s>=2 is a no-op return.
 * step_s==1 body: plays a periodic sfx (0x662, every 16 frames per
 * g_Timer&0xF), and every 4th frame (unk80&3==0) spawns a projectile/spark
 * (type 0x3D) offset -0x400 (90 degrees) from self->rotate; unk80 counts
 * down to 0, at which point step_s advances to 2 (stopping the attack).
 */
void func_us_801A57A4(Entity* self)
{
    ET_801A57A4* ext = (ET_801A57A4*) &self->ext;
    Entity* newEntity;

    if (self->step == 0) {
        InitializeEntity(D_us_80180600);
        self->animSet = 2;
        self->animCurFrame = 0;
        self->unk5A = 0;
        self->palette = 0;
    }

    if (self->step <= 1) {
        if ((g_Timer & 3) == self->params) {
            self->hitboxState = 3;
        } else {
            self->hitboxState = 0;
        }
        if (ext->unk85 != 0) {
            SetStep(2);
        }
        return;
    }

    if (self->step != 2) {
        return;
    }

    if (self->step_s >= 2) {
        return;
    }
    if (self->step_s == 0) {
        self->hitboxState = 0;
        ext->unk80 = 0x10;
        self->step_s = 1;
    }

    if ((g_Timer & 0xF) == 0) {
        PlaySfxPositional(0x662);
    }
    if ((ext->unk80 & 3) == 0) {
        newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
        if (newEntity != NULL) {
            func_us_801A8DF0(0x3D, self, newEntity);
            newEntity->params = 0;
            newEntity->zPriority = 0x89;
            newEntity->rotate = (u16)(self->rotate - 0x400);
        }
    }
    ext->unk80--;
    if (ext->unk80 == 0) {
        self->step_s++;
    }
}

extern s32 AnimateEntity(u8* anim, Entity* self); // return-value variant needed here
extern s32 PLAYER_posY_val;
extern u8 D_us_80180FCC[]; // anim table for AnimateEntity (step 1/2, "aim" pose)
extern u8 D_us_80180FD8[]; // anim table for AnimateEntity (step 3, close-range)
extern u8 D_us_80180FE0[]; // anim table for AnimateEntity (step 4)

typedef struct {
    u8 pad0[4]; // ext+0x0-3 (unused by this function)
    u16 unk80;  // ext+0x4 (abs 0x80): countdown timer, armed to 0x40 once
                // the aim/attack sequence starts (steps 1/2 -> 3)
    u16 phase;  // ext+0x6 (abs 0x82): wave-wobble phase accumulator (step 5,
                // same role as func_us_801A51E4's chain phase)
} ET_801A5948;

/*
 * Scylla's tail entity (type 0x3C, spawned by func_us_801A5338 at
 * self+12, i.e. root = self - 12; the last body segment is self - 1,
 * same "contiguous g_Entities_224 slots" convention as the rest of the
 * chain). This is Scylla's ranged head-lunge/spit attack, launched from
 * the tail end of the chain. Promotes to step 6 via the usual
 * self->flags bit 0x100 convention once (self->params != 0 &&
 * D_us_80180D48 bit 1) latches it.
 *
 * step 0: one-time setup (animCurFrame=0x49, drawFlags=4, facingLeft=1
 *   only for the params==0 variant), falls through into step 1.
 * step 1: waits for root->ext.unk86 ("waiting" flag, read via raw
 *   self-relative offset arithmetic rather than a second pointer --
 *   confirmed to land exactly on root's ext+0xA field, same ET_801A5338
 *   struct used by func_us_801A5338/801A57A4) to clear, then jumps
 *   self->step to 2 directly (a raw field write, not SetStep) before
 *   falling into step 2's own body the same frame.
 * step 2 (also the direct jtbl_us_8019CAC8 target, entered mid-way
 *   through step 1's own code -- a compiler tail-merge, same idiom as
 *   func_us_801A365C): plays an aim animation; once finished, aims at a
 *   mixed target (player's Y, but Scylla's own head's X -- preserved
 *   exactly as read, not "corrected" to a consistent target) and fires
 *   with a fixed launch speed, then SetStep(3).
 * step 3: on step_s==0, plays a one-time SFX (0x80A, params==0 variant
 *   only) then advances step_s; every frame it moves, and once close to
 *   the player plays a close-range animation + a conditional SFX (0x809)
 *   gated on the packed pose/poseTimer word reading exactly 2; a
 *   countdown (armed to 0x40 in step 2) drives SetStep(4) on expiry;
 *   the params==0 variant additionally spawns a periodic type-0x3F
 *   entity every 8 frames (g_Timer & 7).
 * step 4: plays a landing animation, SetStep(5) once finished.
 * step 5: re-aims at root every frame (ratan2 + a wobble term from
 *   ext.phase, same wobble idiom as func_us_801A51E4), moves, and once
 *   close enough to root (integer pixel distance < 0x40) SetStep(2)
 *   (loops back to re-aim/relaunch).
 * step 6 (sub-steps 0/1): spawns a type-0x2 entity once, re-aims
 *   straight at root (no wobble) and launches at a different fixed
 *   speed, then on sub-step 1 moves every frame until within a very
 *   tight raw fixed-point distance of root (0x40 raw units, NOT pixels
 *   -- preserved exactly as read, distinct from step 5's pixel-shifted
 *   check), at which point velocityX/Y are zeroed.
 *
 * Every path (including early "still waiting"/"still animating" exits)
 * converges on a shared per-frame tail that re-aims self->rotate at the
 * previous chain segment (self - 1): rotate = angle, further flipped to
 * (0x800 - angle) for the params==0 variant only.
 *
 * Confidence: high on control flow (every step/sub-step boundary and the
 * self-relative root/prev offset arithmetic cross-checked against the
 * .s, jump table resolved from the actual BO3.BIN rodata -- jtbl at VRAM
 * 0x8019CAC8 / file offset 0x1CAC8); medium on the exact gameplay intent
 * of the mixed-target aim in step 2 and the two different distance-check
 * scales in steps 5/6 (both preserved literally rather than reconciled).
 */
void func_us_801A5948(Entity* self)
{
    Entity* root = self - 12;
    Entity* prev = self - 1;
    ET_801A5948* ext = (ET_801A5948*) &self->ext;
    s32 angle;

    if ((self->params != 0) && (D_us_80180D48 & 2)) {
        self->flags |= 0x100;
    }
    if (self->flags & 0x100) {
        if (self->step < 6) {
            SetStep(6);
        }
    }

    if (self->step < 7) {
        switch (self->step) {
            case 0:
                InitializeEntity(D_us_80180600);
                self->animCurFrame = 0x49;
                self->drawFlags = 4;
                if (self->params == 0) {
                    self->facingLeft = 1;
                }
                /* fallthrough */
            case 1:
                if (((ET_801A5338*) &root->ext)->unk86 != 0) {
                    break;
                }
                self->step = 2;
                /* fallthrough */
            case 2:
                if (AnimateEntity(D_us_80180FCC, self) != 0) {
                    break;
                }
                angle = ratan2(PLAYER_posY_val - self->posY.val, g_Entities[0].posX.val - self->posX.val);
                self->velocityX = rcos((s16) angle) << 5;
                self->velocityY = rsin((s16) angle) << 5;
                ext->unk80 = 0x40;
                self->animCurFrame = 0x49;
                SetStep(3);
                break;
            case 3:
                if (self->step_s == 0) {
                    if (self->params == 0) {
                        g_api_PlaySfx(0x80A);
                    }
                    self->step_s++;
                }
                MoveEntity();
                if (GetDistanceToPlayerX() < 0x38) {
                    AnimateEntity(D_us_80180FD8, self);
                    if (*(s32*) ((u8*) self + 0x50) == 2) {
                        PlaySfxPositional(0x809);
                    }
                }
                ext->unk80--;
                if (ext->unk80 == 0) {
                    SetStep(4);
                }
                if (self->params != 0) {
                    break;
                }
                if ((g_Timer & 7) != 0) {
                    break;
                }
                {
                    Entity* newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                    if (newEntity == NULL) {
                        break;
                    }
                    func_us_801A8D7C(0x3F, newEntity);
                    newEntity->posX.i.hi = 0xE0;
                    newEntity->posY.i.hi = 0x24;
                    {
                        s32 spreadAngle = ((Random() & 0x3F) << 4) | 0x400;
                        newEntity->velocityX = rcos(spreadAngle) << 4;
                        newEntity->velocityY = rsin(spreadAngle) << 4;
                    }
                }
                break;
            case 4:
                if (AnimateEntity(D_us_80180FE0, self) == 0) {
                    SetStep(5);
                }
                break;
            case 5: {
                s16 angleAdj;
                s32 dx = root->posX.val - self->posX.val;
                s32 distInt;

                angle = ratan2(root->posY.val - self->posY.val, dx);
                angleAdj = (s16) (angle + ((rsin((s16) ext->phase) << 9) >> 12));
                self->velocityX = rcos(angleAdj) << 5;
                self->velocityY = rsin(angleAdj) << 5;
                MoveEntity();
                self->animCurFrame = 0x49;
                ext->phase += 0x80;
                if (self->velocityY > 0x8000) {
                    self->animCurFrame = 0x4A;
                }
                if (self->velocityY > 0x7FFF) {
                    self->animCurFrame = 0x4B;
                }
                distInt = dx >> 16;
                if (distInt < 0) {
                    distInt = -distInt;
                }
                if (distInt < 0x40) {
                    SetStep(2);
                }
                break;
            }
            case 6:
                switch (self->step_s) {
                    case 0: {
                        Entity* newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));

                        if (newEntity != NULL) {
                            func_us_801A8DF0(2, self, newEntity);
                            newEntity->params = 3;
                            newEntity->zPriority = self->zPriority + 1;
                        }

                        self->animCurFrame = 0;
                        self->hitboxState = 0;
                        angle = ratan2(root->posY.val - self->posY.val, root->posX.val - self->posX.val);
                        self->velocityX = rcos((s16) angle) * 12;
                        self->velocityY = rsin((s16) angle) * 12;
                        self->step_s++;
                    }
                    /* fallthrough */
                    case 1: {
                        s32 dx;

                        MoveEntity();
                        dx = root->posX.val - self->posX.val;
                        if (dx < 0) {
                            dx = -dx;
                        }
                        if (dx < 0x40) {
                            self->velocityX = 0;
                            self->velocityY = 0;
                        }
                        break;
                    }
                }
                break;
        }
    }

    angle = ratan2(self->posY.val - prev->posY.val, self->posX.val - prev->posX.val);
    self->rotate = (u16) angle;
    if (self->params == 0) {
        self->rotate = (u16) (0x800 - angle);
    }
}

typedef struct {
    u8 pad0[8];      // ext+0x0-7
    s16 angleOffset; // ext+0x8
    u8 pad1[2];      // ext+0xA-B
    s8 variant;      // ext+0xC
    s8 countdown;    // ext+0xD
} ET_801A5E4C;

/*
 * Confidence: medium (m2c output only, not independently .s-verified).
 * Spawns a ring of 6 projectile/debris entities (type 0x3D via
 * func_us_801A8DF0) around g_CurrentEntity, sharing one random angle
 * offset and one random "variant" (0-3) across the whole ring, with each
 * spawned entity's countdown field (ext+0xD) set to 6,5,4,3,2,1 in spawn
 * order.
 */
void func_us_801A5E4C(void)
{
    s8 variant = (s8)(Random() & 3);
    s16 angleOffset = (s16)(((Random() & 0xF) << 8) - 0x800);
    s32 i;

    for (i = 0; i < 6; i++) {
        Entity* newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
        if (newEntity != NULL) {
            ET_801A5E4C* ext = (ET_801A5E4C*) &newEntity->ext;
            func_us_801A8DF0(0x3D, g_CurrentEntity, newEntity);
            newEntity->params = 2;
            ext->countdown = (s8)(6 - i);
            ext->angleOffset = angleOffset;
            ext->variant = variant;
        }
    }
}

typedef struct {
    u8 pad0[4];      // ext+0x0-0x3 (unused by this function)
    u8* animData;    // ext+0x4 (abs 0x80): anim frame data pointer for AnimateEntity
    u8 pad8[8];       // ext+0x8-0xF (unused by this function)
    s32 unk10;        // ext+0x10 (abs 0x8C): per-frame velocityY delta while in
                       // the debris/shatter sub-states (step_s-driven, steps 2-4)
} ET_801A5F0C;

extern void InitializeEntity(u16 params[]);
extern void MoveEntity(void);
extern void DestroyEntity(Entity* entity);
extern s32 AnimateEntity(u8* anim, Entity* self);
// Note: D_us_8018054C is already declared `extern s32 D_us_8018054C;` earlier
// in this file (func_us_8019E0D0); reusing that declaration here rather than
// introducing a second, incompatible one -- matches this file's existing,
// tolerated int/array inconsistency for this symbol.

typedef struct {
    s16 animSet;    // +0x0
    s16 unk5A;      // +0x2
    u16 paletteBase; // +0x4 (palette = this + 0x2E0)
    u8 blendMode;    // +0x6
    u8 pad7;         // +0x7
    u8* animData;    // +0x8
} D_us_8018105C_entry;
extern D_us_8018105C_entry D_us_8018105C[];

// Scylla debris/shard entity spawned on death: step 0 picks a per-subtype
// (params & 0xF) look/anim from D_us_8018105C and advances step to
// subtype+1 immediately; params bits 0xFF00 optionally override zPriority,
// and params bits 0xF0 force a special "flash" look (palette 0x819F,
// blendMode 0x10, facingLeft) that overrides step's own field before
// returning (both paths return -- the switch's "default" merge point in the
// raw dump is this shared return, not an unhandled-case fallthrough).
// step 1 (self->step==1) is a small rotate/fly-off sub-state machine keyed
// off step_s: step_s==0 sets velocity from rotate via rcos/rsin and falls
// into step_s==1's body (MoveEntity + anim-driven destroy); step_s==1 does
// just that body; step_s>=2 does nothing at all and returns immediately --
// this last branch is a real 3-way "if/else if/else" in the asm
// (.Lus_801A6000) but m2c's raw dump silently dropped the "else: do
// nothing" arm, merging it away. Caught by re-tracing .Lus_801A6000's three
// branches directly against the asm rather than trusting the dump.
// steps 2/3/4 are three near-identical debris/explosion variants (only the
// opacity/velocity constants differ): step_s==0 seeds drawFlags, opacity,
// facingLeft, velocityX/Y and a per-frame velocityY delta (ext->unk10) then
// advances step_s; every call then does MoveEntity, applies the velocityY
// delta, decrements opacity (wraps via +0xFF), and falls into the same
// anim-driven-destroy tail as step 1. The raw dump attributed a bogus
// `Random(0xFFFF4000)`-style argument to the delta's Random() call in each
// of these three cases; that hex constant is actually a leftover value in
// $a0 already consumed by the *previous* instruction (storing the constant
// into velocityY), not a real argument -- Random() takes no real parameter
// here, confirmed by re-tracing the delay-slot ordering in the .s. Dropped
// the phantom argument in all three cases.
// Confidence: high on control flow (re-verified against the .s directly for
// the step_s>=2 branch and the Random() argument issue above); D_us_8018105C
// field semantics inferred from usage/offsets only.
void func_us_801A5F0C(Entity* self)
{
    ET_801A5F0C* ext = (ET_801A5F0C*) &self->ext;

    switch (self->step) {
        case 0: {
            u16 subtype = self->params & 0xF;
            D_us_8018105C_entry* entry = &D_us_8018105C[subtype];

            self->palette = entry->paletteBase + 0x2E0;
            self->blendMode = entry->blendMode;
            self->animSet = entry->animSet;
            self->unk5A = entry->unk5A;
            self->step = subtype + 1;
            ext->animData = entry->animData;
            if (self->params & 0xFF00) {
                self->zPriority = (u16)((self->params & 0xFF00) >> 8);
            }
            if (self->params & 0xF0) {
                self->palette = 0x819F;
                self->blendMode = 0x10;
                self->facingLeft = 1;
            }
            return;
        }
        case 1: {
            if (self->step_s == 0) {
                s16 rotate = self->rotate;
                self->drawFlags = 4;
                self->velocityX = rcos(rotate) << 4;
                self->velocityY = rsin(rotate) << 4;
                self->step_s++;
            } else if (self->step_s != 1) {
                break; // step_s >= 2: do nothing this frame
            }
            MoveEntity();
            if (AnimateEntity(ext->animData, self) == 0) {
                DestroyEntity(self);
            }
            break;
        }
        case 2:
            if (self->step_s == 0) {
                self->drawFlags = 8;
                self->opacity = 0xC0;
                self->facingLeft = (u16)(Random() & 1);
                self->velocityX = (Random() << 8) - 0x8000;
                self->velocityY = -0xC000;
                ext->unk10 = -(Random() << 4) - 0x4000;
                self->step_s++;
            }
            MoveEntity();
            self->velocityY += ext->unk10;
            self->opacity += 0xFF;
            if (AnimateEntity(ext->animData, self) == 0) {
                DestroyEntity(self);
            }
            break;
        case 3:
            if (self->step_s == 0) {
                self->drawFlags = 8;
                self->opacity = 0x80;
                self->facingLeft = (u16)(Random() & 1);
                self->velocityX = (Random() << 0xA) - 0x20000;
                self->velocityY = -0x24000;
                ext->unk10 = -(Random() << 4) - 0x2000;
                self->step_s++;
            }
            MoveEntity();
            self->velocityY += ext->unk10;
            self->opacity += 0xFF;
            if (AnimateEntity(ext->animData, self) == 0) {
                DestroyEntity(self);
            }
            break;
        case 4:
            if (self->step_s == 0) {
                self->drawFlags = 8;
                self->opacity = 0x80;
                self->facingLeft = (u16)(Random() & 1);
                self->velocityX = (Random() << 9) - 0x10000;
                self->velocityY = -0x28000;
                ext->unk10 = -(Random() << 4) - 0x1000;
                self->step_s++;
            }
            MoveEntity();
            self->velocityY += ext->unk10;
            self->opacity += 0xFF;
            if (AnimateEntity(ext->animData, self) == 0) {
                DestroyEntity(self);
            }
            break;
    }
}

extern void (*g_api_CheckCollision)(s32 x, s32 y, Collider* res, s32 unk);

/*
 * Falling debris chunk: step 0 does one-shot init (drawFlags=4, animSet
 * sentinel -0x7FFF forcing a fresh anim, palette=0x800B constant -- NOT
 * Random(), confirmed via delay-slot ordering in the .s -- animCurFrame
 * randomly 0x26 or 0x27, facingLeft=1 if moving left), then falls straight
 * into step 1's body (shared via the asm's fallthrough, no separate case).
 * step 1: rotate advances every frame (+0x40 if params!=0, else +0x20),
 * gravity accelerates velocityY, then checks ground collision 6px below
 * self; on hit, adjusts posY.i.hi by the collision's tile-top edge (a
 * narrow 16-bit read of Collider's normally-s32 unk18 field -- confirmed
 * via the .s using lhu at exactly that offset, kept as a raw offset read
 * here rather than the named s32 field so the codegen shape stays
 * faithful) and either bounces (velocityY = -velocityY/2, still falling
 * fast: >0xFFFF) or spawns a splash (type 6) and self-destroys (near rest).
 * Confidence: high, fully cross-checked against func_us_801A61F0.s.
 */
void func_us_801A61F0(Entity* self)
{
    Collider col;
    Entity* newEntity;
    s32 velocityY;

    if (self->step == 0) {
        InitializeEntity(D_us_8018054C);
        self->drawFlags = 4;
        self->animSet = -0x7FFF;
        self->palette = 0x800B;
        self->animCurFrame = (Random() & 1) ? 0x26 : 0x27;
        if (self->velocityX < 0) {
            self->facingLeft = 1;
        }
    } else if (self->step != 1) {
        return;
    }

    MoveEntity();
    self->rotate += (u16)(self->params != 0 ? 0x40 : 0x20);

    velocityY = self->velocityY + 0x2000;
    self->velocityY = velocityY;
    g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 6, &col, 0);
    if ((col.unk10 & 1) == 0) {
        return;
    }

    self->posY.i.hi += *(u16*)((u8*)&col + 0x18);
    if (velocityY > 0xFFFF) {
        self->velocityY = -velocityY / 2;
        return;
    }

    newEntity = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
    if (newEntity != NULL) {
        func_us_801A8DF0(6, self, newEntity);
        newEntity->params = 0x10;
    }
    DestroyEntity(self);
}

// Boss-defeat music sequencer, sibling of func_us_801A6568 (same step-machine
// idiom, different trigger bits on the shared D_us_80180D48 mask instead of
// D_us_80180C66, and a 6th extra state). step 0: destroys self immediately if
// this Time Attack event was already recorded, else plays a fanfare (0x90)
// and falls into step 1, which waits for D_us_80180D48 bit 0 before
// requesting the victory music (0x31D) to stop. Step 2 waits for it to
// actually stop then resumes it; step 3 optionally restarts the stop
// sequence with a fanfare + different track (0x91/0x30B) on bit 1; step 4
// does the same on bit 2 (no fanfare); step 5 is the same "wait for stop,
// resume" pattern as step 2.
// Confidence: high on control flow and Entity/global fields; D_us_80180D48's
// individual bit meanings and g_api_TimeAttackController's exact semantics
// are inferred from usage/naming only.
void func_us_801A6370(Entity* self)
{
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180564);
            if (g_api_TimeAttackController(TIMEATTACK_EVENT_SCYLLA_DEFEAT, TIMEATTACK_GET_RECORD) != 0) {
                DestroyEntity(self);
                return;
            }
            g_api_PlaySfx(0x90);
            /* fallthrough */
        case 1:
            if (D_us_80180D48 & 1) {
                stopMusicFlag = 0;
                currentMusicId = 0x31D;
                self->step++;
            }
            return;

        case 2:
            if (g_api_func_80131F68() == 0) {
                stopMusicFlag = 0;
                g_api_PlaySfx(currentMusicId);
                self->step++;
            }
            /* fallthrough */
        case 3:
            if (D_us_80180D48 & 2) {
                g_api_PlaySfx(0x91);
                currentMusicId = 0x30B;
                stopMusicFlag = 0;
                self->step++;
                return;
            }
            break;

        case 4:
            if (D_us_80180D48 & 4) {
                stopMusicFlag = 1;
                currentMusicId = 0x30B;
                self->step++;
                return;
            }
            break;

        case 5:
            if (g_api_func_80131F68() == 0) {
                stopMusicFlag = 0;
                g_api_PlaySfx(currentMusicId);
                self->step++;
                return;
            }
            break;
    }
}

// Post-battle music sequencer: step 0 destroys self immediately if
// g_CastleFlags[0xC5] (a "sequence already played" flag) is set, otherwise
// falls into step 1, which waits for D_us_80180C66 bit 1 (a shared signal,
// same mask as func_us_801A07CC/801A1BE8) before requesting the victory
// music (0x31D) to stop. Step 2 waits for the current music to actually stop
// (g_api_func_80131F68), then plays it back (resume). Step 3 optionally
// restarts the stop sequence with a different track (0x30B) if the flag is
// set again; step 4 is the same "wait for stop, resume" pattern as step 2.
// Confidence: high on control flow and Entity/global fields (self->step,
// g_CastleFlags, stopMusicFlag/currentMusicId/g_api_func_80131F68 all
// confirmed via game.h); D_us_80180C66/g_CastleFlags[0xC5]'s precise
// gameplay meaning inferred from usage only.
void func_us_801A6568(Entity* self)
{
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180564);
            if (g_CastleFlags[0xC5] != 0) {
                DestroyEntity(self);
                return;
            }
            /* fallthrough */
        case 1:
            if (D_us_80180C66 & 2) {
                stopMusicFlag = 1;
                currentMusicId = 0x31D;
                self->step++;
            }
            return;

        case 2:
            if (g_api_func_80131F68() == 0) {
                stopMusicFlag = 0;
                g_api_PlaySfx(currentMusicId);
                self->step++;
            }
            /* fallthrough */
        case 3:
            if (g_CastleFlags[0xC5] != 0) {
                g_api_PlaySfx(0x90);
                stopMusicFlag = 1;
                currentMusicId = 0x30B;
                self->step = 4;
                return;
            }
            break;

        case 4:
            if (g_api_func_80131F68() == 0) {
                stopMusicFlag = 0;
                g_api_PlaySfx(currentMusicId);
                self->step++;
                return;
            }
            break;
    }
}
