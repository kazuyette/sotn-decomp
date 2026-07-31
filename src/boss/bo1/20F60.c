#include "stage.h"

extern void (*g_api_PlaySfx)(s32 sfxId);
extern void ReplaceBreakableWithItemDrop(Entity* entity);
extern s16 (*g_api_func_800EDB58)(s32, s32); // TODO: likely "alloc a chain of N prims of type T"; args (0x11, 8) not yet understood

extern u8* D_us_80180C40[];  // per-attack animation frame table
extern u16 D_us_80180A8C[];  // InitializeEntity params (step 0, func_us_801A0F60)
extern u16 D_us_80180AA4[];  // InitializeEntity params (step 0, func_us_801A10A8 AND func_us_801A518C -- same address, confirming both are "debris effect" entities)
extern u8 D_us_80180C50[];   // hitboxHeight table
extern u8 D_us_80180C54[];   // drop params table
extern u16 D_us_80180C58[];  // animSet table
extern u8 D_us_80180C60[];   // blendMode table
extern u16 D_us_80180C64[];  // zPriority table
extern s32 D_us_80180C74[];  // debris initial "velocityX" (unk1C) table, TODO: verify packed layout
extern s32 D_us_80180C78[];  // debris initial "velocityY" (unk20) table, TODO: verify packed layout
extern Entity g_Entities_224[];
extern Entity g_Entities_192[]; // sub-slice of g_Entities used by func_us_801A2774's step-3 explosion
extern Entity g_Entities_64[];  // sub-slice of g_Entities used elsewhere in this file (bo1)

extern u16 D_us_80180B34[];  // InitializeEntity params (step 0, func_us_801A493C)
extern u16 D_us_80180B40[];  // InitializeEntity params (step 0, func_us_801A2774)
extern u16 D_us_80180B4C[];  // InitializeEntity params (step 0, func_us_801A2BC4 / 2CC4 / 2D90 -- shared projectile init)
extern u16 D_us_80180B58[];  // InitializeEntity params (step 0, func_us_801A4394 / 45D0)
extern u16 D_us_80180D00[];  // per-attack-pattern angle table, indexed by self->params (or params & 0xF)
extern s32 D_us_80180CD8;    // shared per-pattern "attack done" bitmask (bit N == pattern N finished)
extern s32 D_us_80180CE0;    // shared attack-completion counter
extern s32 D_us_80180CE4;    // shared status flags: bit0 seen broadly ("attack ending"), bits 2/3 also referenced (func_us_801A2F2C)
extern u8 D_us_80180DE0[];   // animation frame table (func_us_801A45D0's crawl anim)
extern u8 D_us_80180CE8[];   // TODO: passed to UnkCollisionFunc2 -- presumably a boss-specific hitbox/Collider definition, layout unresolved
extern u16 D_us_80180B1C[];  // InitializeEntity params (step 0, func_us_801A2F2C / 3480)
extern u16 D_us_80180DD0[];  // per-params opacity byte table (stride 2, cast to u8 -- same pattern as other tables here)
extern u8 D_us_80180DF4[];   // animation frame table (func_us_801A2F2C)
extern u8 D_us_80180E10[];   // animation frame table (func_us_801A3480 step 2 sub-state 0)
extern u8 D_us_80180E18[];   // animation frame table (func_us_801A3480 step 2 sub-state 3)

typedef struct { s16 x; s16 y; } D_us_80180D90_Entry;
extern D_us_80180D90_Entry D_us_80180D90[]; // per-segment position offset from root (x/y interleaved -- confirmed
                                             // by address math: m2c's separately-named D_us_80180D92 is this
                                             // table + 2 bytes, matching the 4-byte stride exactly)

extern s32 GetSideToPlayer(void); // bo1-local (0x801A8D28); confirmed 0-arg by usage in src/boss/rbo0/e_fake_grant.c
extern void SetStep(s32 step);    // bo1-local (0x801A95F8)
extern void SetSubStep(s32 step_s); // bo1-local (0x801A9618)
extern s32 UnkCollisionFunc2(void* collider); // bo1-local (0x801A9040)
extern s32 GetAngleBetweenEntities(Entity* from, Entity* to); // bo1-local (0x801A9518)
extern s32 GetDistanceToPlayerX(void); // bo1-local
extern u16 D_8003C506;      // shared (non-boss) global -- some kind of background/screen effect index, TODO: exact purpose unresolved
extern u16 D_8003C508[];    // shared (non-boss) lookup table for the above, indexed by an animation counter
extern s16 PLAYER_posX_i_hi;
extern s16 PLAYER_posY_i_hi;
extern s32 currentMusicId;
extern Entity g_Entities_160[]; // sub-slice of g_Entities (used for a cutscene entity spawn in func_us_801A1878's step 8)
extern void (*g_api_func_80102CD8)(s32); // TODO: unresolved, called with arg 1 alongside a periodic SFX in func_us_801A1878
extern u16 g_pads_1_pressed;
extern s32 stopMusicFlag;
extern u16 D_us_80180B10[]; // InitializeEntity params (step 0, func_us_801A1878)
extern u32 D_us_80180D14[8]; // player-angle-sector -> attack pattern index table (8 sectors, see func_us_801A1878)
extern s32 D_us_80180CDC;    // shared "chain link finished flying to position" flag, set by chain-segment logic elsewhere
extern u16 D_us_80180B28[]; // InitializeEntity params (step 0, func_us_801A38EC / 4AF4 -- shared)
extern s16 LimitAngleChange(s16 maxDelta, s16 current, s16 target); // shared trig helper, eases an angle toward a target capped at maxDelta/frame
extern u8 D_us_80180F58[]; // animation frame table (func_us_801A518C patterns 0/1/4)
extern u8 D_us_80180F74[]; // animation frame table (func_us_801A518C patterns 5/6)
extern u8 D_us_80180FAC[]; // animation frame table (func_us_801A518C patterns 2/3)

/*
 * Per-attack-pattern behavior table for func_us_801A518C, indexed by
 * self->params & 0xF. Recovered directly from m2c's rodata reconstruction
 * (not guessed) -- these are the actual initializer values from the ROM.
 */
typedef struct {
    u16 animSet;
    u16 unk5A;         // -> self->unk5A (already a named-but-generic Entity field)
    u16 paletteOffset; // -> self->palette = paletteOffset + 0x2E0
    u8 blendMode;
    u8 pad;
    u8* animTable;     // -> self->ext+4, consumed by AnimateEntity
} D_us_80180FCC_Entry; // size 0xC
static const D_us_80180FCC_Entry D_us_80180FCC[7] = {
    { 0x000E, 0x79, 0x0,  0x30, 0, D_us_80180F58 },
    { 0x000E, 0x79, 0x0,  0x70, 0, D_us_80180F58 },
    { 0x8004, 0x58, 0xF,  0x10, 0, D_us_80180FAC },
    { 0x8004, 0x58, 0x4,  0x30, 0, D_us_80180FAC },
    { 0x000E, 0x79, 0x4,  0x30, 0, D_us_80180F58 },
    { 0x8005, 0x59, 0x6,  0x30, 0, D_us_80180F74 },
    { 0x8005, 0x59, 0x6,  0x30, 0, D_us_80180F74 },
};
extern u8 D_us_80180E20[];  // animation frame table (func_us_801A38EC step 32 sub-state 0)
extern u8 D_us_80180E2C[];  // animation frame table (func_us_801A38EC step 0/1)
extern Entity g_Entities_128[]; // sub-slice of g_Entities (func_us_801A38EC step 16)

/*
 * Shared per-attack-pattern table, indexed by self->params, stride 0xA (5 x
 * u16 fields). Field boundaries confirmed by cross-referencing 3 different
 * consumer functions that each read a different subset of offsets:
 *   - func_us_801A2774 case 0 reads offsets 0x0 and 0x2 directly.
 *   - func_us_801A2BC4 case 0 reads offsets 0x4 and 0x6 (as alternate
 *     animCurFrame values depending on params & 0x100).
 *   - func_us_801A2CC4 case 0 reads offset 0x8 (called D_us_80180D3C by m2c,
 *     which is just this same table + 8 bytes -- confirmed by address math:
 *     0x80180D3C - 0x80180D34 == 8, matching the 0xA stride exactly).
 */
typedef struct {
    /* 0x0 */ u16 animCurFrame;      // func_us_801A2774
    /* 0x2 */ u16 zPriority;         // func_us_801A2774
    /* 0x4 */ u16 altAnimCurFrame0;  // func_us_801A2BC4, used when !(params & 0x100)
    /* 0x6 */ u16 altAnimCurFrame1;  // func_us_801A2BC4, used when params & 0x100
    /* 0x8 */ u16 variantFrameId;    // nonzero => func_us_801A2774 spawns an escort
                                     // entity (type 0x1E); also read directly as an
                                     // animCurFrame in func_us_801A2CC4
} D_us_80180D34_Entry; // size 0xA
extern D_us_80180D34_Entry D_us_80180D34[];

void func_us_801A0F60(Entity* self) {
    u16 attackId = self->params >> 0xC;

    if (self->step != 0) {
        AnimateEntity(D_us_80180C40[attackId], self);
        if (self->hitParams != 0) {
            Entity* item;
            g_api_PlaySfx(0x634);

            item = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (item != NULL) {
                CreateEntityFromCurrentEntity(2, item);
                item->params = D_us_80180C54[attackId];
            }
            ReplaceBreakableWithItemDrop(self);
        }
    } else {
        InitializeEntity(D_us_80180A8C);
        self->zPriority = g_unkGraphicsStruct.BottomCornerTextTimer - 0x14; // dead store, overwritten below (matches original asm); real field at this offset per include/game.h
        self->zPriority = D_us_80180C64[attackId];
        self->blendMode = D_us_80180C60[attackId];
        self->hitboxHeight = D_us_80180C50[attackId];
        self->animSet = D_us_80180C58[attackId];
    }
}

/*
 * Granfaloon "shatter into debris" effect handler.
 *
 * self->step == 0: first frame. Allocates a chain of Primitive slots via the
 * still-unnamed g_api_func_800EDB58 and reuses their normally-unused color
 * bytes (0x10-0x13 and 0x14-0x17) as a scratch 32-bit fixed-point (posX, posY)
 * accumulator per debris piece -- confirmed by cross-referencing the same
 * union trick used for Entity.posX/posY (include/types.h: f32 is a
 * {s16 lo, s16 hi} union, .i.hi is the integer part, matching the existing
 * convention in src/dra/4B758.c: `entity->posX.i.hi + g_backbufferX`).
 * self->flags |= FLAG_HAS_PRIMS (0x800000, include/game.h:411) tells
 * DestroyEntity to free this prim chain later -- already a named/documented
 * flag, not a guess.
 *
 * self->step == 1 (or falls through from step 0 on the same frame): advances
 * the physics for each debris piece one frame -- posX/posY accumulate a
 * constant unk1C (velocityX) and an incrementing unk20 (velocityY += 0x1000,
 * i.e. gravity), then each piece is drawn at (entity base position + local
 * debris offset). self->ext timer (unk80, initialized to 0x40 = 64 frames)
 * counts down once per frame; at 0 the entity self-destructs.
 *
 * Confidence: high on control flow and the physics/position math (directly
 * confirmed against named Entity fields and existing conventions elsewhere
 * in the repo). Lower confidence on: the exact semantics of D_us_80180C74/78
 * (assumed velocityX/velocityY, 8-byte stride suggests 2 values per debris
 * piece but the table's true element type isn't verified), and the two
 * unnamed g_api_func_800EDB58 arguments. Not yet verified against a real
 * compile/asm-differ pass (no PS1 toolchain in this sandbox) -- do that in
 * WSL before trusting this as byte-exact.
 */
typedef struct {
    /* 0x7C */ Primitive* prim;
    /* 0x80 */ s16 timer;
} ET_801A10A8;

void func_us_801A10A8(Entity* self) {
    Primitive* prim;
    s32 debrisIdx;
    s32 velocityX;
    s32 primIdx;

    if (self->step == 0) {
        InitializeEntity(D_us_80180AA4);
        ((ET_801A10A8*) &self->ext)->timer = 0x40;

        primIdx = g_api_func_800EDB58(0x11, 8);
        prim = &g_PrimBuf[primIdx];
        self->primIndex = primIdx;
        ((ET_801A10A8*) &self->ext)->prim = prim;
        self->flags |= FLAG_HAS_PRIMS;

        if (prim->next != NULL) {
            debrisIdx = 0;
            do {
                prim->u0 = 2;
                prim->v0 = 2;
                prim->x0 = self->posX.i.hi;
                prim->r0 = 0x60;
                prim->g0 = 0x40;
                prim->b0 = 0x60;
                prim->y0 = self->posY.i.hi;
                *(s32*) &prim->r2 = D_us_80180C74[debrisIdx]; // unk1C, reused as velocityX
                velocityX = D_us_80180C78[debrisIdx];          // unk20, reused as velocityY
                *(s32*) &prim->x2 = velocityX;
                if (self->params & 0x10) {
                    s32 tmp = *(s32*) &prim->r2;
                    *(s32*) &prim->r2 = velocityX;
                    *(s32*) &prim->x2 = -tmp;
                }
                debrisIdx += 8;
                if (self->params & 1) {
                    *(s32*) &prim->r2 = -*(s32*) &prim->r2;
                }
                *(s32*) &prim->r1 = 0;  // posX accumulator reset (unk10)
                *(s32*) &prim->x1 = 0;  // posY accumulator reset (unk14)
                prim->drawMode = 0;
                prim->priority = self->zPriority;
                prim = prim->next;
            } while (prim->next != NULL);
        }
        prim->v0 = 0;
        prim->u0 = 0;
        prim->y0 = 0;
        prim->x0 = 0;
        prim->drawMode = 2;
        prim->priority = self->zPriority;
    }
    if (self->step == 0 || self->step == 1) {
        prim = ((ET_801A10A8*) &self->ext)->prim;
        if (prim->next != NULL) {
            do {
                s32 posX = *(s32*) &prim->r1;
                s32 posY = *(s32*) &prim->x1;
                s32 velY = *(s32*) &prim->x2;

                posX += *(s32*) &prim->r2;             // += velocityX
                *(s32*) &prim->x2 = velY + 0x1000;      // velocityY += gravity
                posY += velY;

                *(s32*) &prim->r1 = posX;
                *(s32*) &prim->x1 = posY;

                prim->x0 = self->posX.i.hi + (s16) (posX >> 16);
                prim->y0 = self->posY.i.hi + (s16) (posY >> 16);
                prim = prim->next;
            } while (prim->next != NULL);
        }
        prim->v0 = 0;
        prim->u0 = 0;
        prim->y0 = 0;
        prim->x0 = 0;
        prim->drawMode = 2;
        prim->priority = self->zPriority;

        if (--((ET_801A10A8*) &self->ext)->timer == 0) {
            DestroyEntity(self);
        }
    }
}

/*
 * Granfaloon main body state machine (7 states, self->step 0-6). Handles
 * the boss falling from the ceiling, landing on the ground, and settling --
 * driven by the jump table at vram 0x801A0AA0 (jtbl_us_801A0AA0, recovered
 * from asm/us/boss/bo1/data/0.data.s and fed back into m2c by hand, since
 * splat put it in a separate `data` split from the function itself).
 *
 * step 0: first frame. InitializeEntity(D_us_80180AF8), sets initial screen
 * position from g_Tilemap camera offsets depending on self->params (room
 * orientation), writes a 4-tile pattern (D_us_80180CCC) into g_Tilemap.
 * If D_us_801B2378 is set, the entity self-destructs immediately -- looks
 * like a global skip/debug flag, not yet named anywhere else in the repo.
 * Falls through into step 1's body on the same frame.
 *
 * step 1 (and step 4, same shape): gate -- only proceeds to the shared
 * "advance step" tail (goto block_40) if D_us_80180CD4 (a local "impact
 * already handled" flag) is still 0; otherwise returns immediately so step 2
 * (or step 2's continuation) keeps running every frame until landing.
 *
 * step 2: real fall physics. self->posY += self->velocityY; velocityY +=
 * 0x8000 (gravity). Calls g_api_CheckCollision(posX, posY+0x20, ...) to test
 * for ground contact. On contact: corrects posY, reflects+dampens velocityY
 * (negate, then >>2), plays the impact sound once (PlaySfxPositional(0x655),
 * gated by D_us_80180CD4 & 0x10 so it doesn't repeat every frame), and spawns
 * two debris entities (entity type 6 and type 0x17) positioned near the
 * impact point, mirrored left/right based on self->params. Once velocityY
 * has settled (>= -0x1FFF) advances to step 3. Separately -- independent of
 * whether contact happened this frame -- the first time velocityY has been
 * above 0x40000 (self->ext byte flag at absolute offset 0x84 latches this),
 * a third debris entity (type 0x17) is spawned.
 *
 * step 3, 5: write two more 4-tile patterns into g_Tilemap (D_us_80180CC4,
 * D_us_80180CCC respectively) using the same D_us_80180CB4 position table,
 * then fall into the shared "advance step" tail.
 *
 * step 6: sets flags bit 0x80000000 (top bit -- not yet named/cross-
 * referenced elsewhere, unlike FLAG_HAS_PRIMS) and decrements posY.i.hi by
 * 1/frame -- final state, likely the boss settling/idling in place after
 * landing.
 *
 * Confidence: high on control flow, the gravity/collision physics, and
 * entity spawning (all match confirmed Entity fields and named functions/
 * flags). Lower confidence on: the exact semantics of D_us_80180CB4 (typed
 * here as a [2][4] s16 tile-position table indexed directly by self->params,
 * which only works cleanly if params is guaranteed 0 or 1 in this context --
 * elsewhere in bo1, params is used as a multi-bit field, e.g.
 * `params & 0x10`/`& 1`/`>> 0xC`, so this assumption isn't verified);
 * D_us_801B2378's real meaning is a guess (global kill-switch flag); the
 * 0x80000000 flag bit in step 6 isn't named/cross-referenced. Not verified
 * against a real compile/asm-differ pass -- do that in WSL.
 */
typedef struct {
    /* 0x84 */ u8 hasSpawnedThirdDebris;
} ET_801A1334;

extern u32 D_us_80180AF8[6];      // InitializeEntity params (step 0)
extern s16 D_us_80180CB4[2][4];   // tile position table, indexed by self->params (see caveat above)
extern u16 D_us_80180CC4[4];      // tile values written in step 3
extern u16 D_us_80180CCC[4];      // tile values written in steps 0 and 5
extern s32 D_us_80180CD4;         // local "impact sound already played" flag (bit 0x10) + gate (nonzero)
extern s32 D_us_801B2378;         // TODO: guessed meaning -- "skip/debug kill switch"

void func_us_801A1334(Entity* self) {
    s16 tileBase;
    Collider col;
    s32 i;
    Entity* debris;

    switch (self->step) {
        case 0:
            InitializeEntity((u16*) D_us_80180AF8);
            self->animCurFrame = self->params + 0x10;

            if (self->params != 0) {
                self->posX.i.hi = 7 - g_Tilemap.scrollX.i.hi;
                tileBase = 0x141;
            } else {
                self->posX.i.hi = 0x1F7 - g_Tilemap.scrollX.i.hi;
                tileBase = 0x41;
            }
            self->posY.i.hi = tileBase - g_Tilemap.scrollY.i.hi;

            for (i = 0; i < 4; i++) {
                g_Tilemap.fg[D_us_80180CB4[self->params][i]] = D_us_80180CCC[i];
            }

            if (D_us_801B2378 != 0) {
                DestroyEntity(self);
                return;
            }
            D_us_80180CD4 = 0;
            self->zPriority = 0x5F;
            // fallthrough to step 1

        case 1:
        case 4:
            if (D_us_80180CD4 != 0) {
                return;
            }
            goto block_40;

        case 2:
            self->posY.val += self->velocityY;
            self->velocityY += 0x8000;
            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 0x20, &col, 0);

            if (col.unk10 != 0) {
                self->posY.i.hi += 0; // TODO: correction value from Collider result, offset not resolved (real signature confirmed via include/game.h: Collider.unk10 is the branch condition here, per asm/us/boss/bo1/nonmatchings/20F60/func_us_801A1334.s)
                self->velocityY = -self->velocityY;
                if (self->velocityY < 0) {
                    self->velocityY += 3;
                }
                self->velocityY >>= 2;

                if (!(D_us_80180CD4 & 0x10)) {
                    PlaySfxPositional(0x655, self->velocityY);
                    D_us_80180CD4 |= 0x10;
                }

                debris = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (debris != NULL) {
                    CreateEntityFromEntity(6, self, debris);
                    debris->params = 0x10;
                    debris->posX.i.hi += (Random() & 7) * 2 - 8;
                    debris->posY.i.hi += 0x20;
                }
                debris = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (debris != NULL) {
                    CreateEntityFromEntity(0x17, self, debris);
                    debris->posY.i.hi += 0x20;
                    debris->posX.i.hi += self->params != 0 ? 8 : -8;
                    debris->params = self->params;
                }
                if (self->velocityY >= -0x1FFF) {
                    self->step++;
                }
            }

            if (!((ET_801A1334*) &self->ext)->hasSpawnedThirdDebris && self->velocityY > 0x40000) {
                ((ET_801A1334*) &self->ext)->hasSpawnedThirdDebris = 1;
                debris = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (debris != NULL) {
                    CreateEntityFromEntity(0x17, self, debris);
                    debris->posY.i.hi += 0x10;
                    debris->posX.i.hi += self->params != 0 ? 8 : -8;
                    debris->params = self->params + 0x10;
                    return;
                }
            }
            break;

        case 3:
            for (i = 0; i < 4; i++) {
                g_Tilemap.fg[D_us_80180CB4[self->params][i]] = D_us_80180CC4[i];
            }
            goto block_40;

        case 5:
            for (i = 0; i < 4; i++) {
                g_Tilemap.fg[D_us_80180CB4[self->params][i]] = D_us_80180CCC[i];
            }
block_40:
            self->step++;
            break;

        case 6:
            self->flags |= 0x80000000; // TODO: unnamed flag bit
            self->posY.i.hi--;
            break;
    }
}

/*
 * Moves *value one step (of magnitude `step`) toward `target`, snapping
 * directly to `target` once within `step` of it. Returns 1 the call it
 * snaps (reached target), 0 otherwise. Conceptually the same idea as the
 * shared AdjustValueWithinThreshold (src/saturn/game.c:1763) but a distinct,
 * boss-specific variant: operates in-place through a pointer and returns a
 * "reached" flag instead of returning the new value. Confirmed via m2c
 * (matches hand analysis exactly, no ambiguity in this one -- pure integer
 * arithmetic, no unnamed globals or unclear structs involved).
 */
s32 func_us_801A17F8(s16* value, s16 target, s16 step) {
    s16 cur = *value;
    s32 diff = cur - target;

    if (diff < 0) {
        diff = -diff;
    }
    if (diff < step) {
        *value = target;
        return 1;
    }
    if (target < cur) {
        *value = cur - step;
    }
    if (*value < target) {
        *value += step;
    }
    return 0;
}

/*
 * Granfaloon's ROOT/director entity. This resolves the "root" ambiguity
 * flagged in func_us_801A2774/2F2C/3480: step 0 here spawns exactly 8
 * type-0x19 entities starting at self+10 slots (stride 5 slots each) --
 * precisely matching func_us_801A2774/2F2C's `root = self - (params*5+10)`
 * formula -- and 9 type-0x1C entities starting at self+1 (stride 1 slot).
 * This is also why ext+8 (patternActiveMask) being a per-pattern BITMASK
 * (not a single bool) makes sense: this entity sets one bit per active
 * chain pattern (`self->patternActiveMask = 1 << self->currentPattern`),
 * and each chain segment reads back its own bit.
 *
 * step 0: full boss init -- spawns the two entity families above, resets
 * the shared globals (D_us_80180CE4/CD8/CE0), positions itself, then falls
 * to step 2.
 * step 2: waits until the player is within 0x80 (GetDistanceToPlayerX) of
 * the boss's X, then advances to step 3.
 * step 3: intro "dive" sequence -- camera/music setup (TimeAttackController,
 * currentMusicId), falls toward the ground while firing item-drop-style
 * projectiles (entity type 2, params 0x11) with randomized spread, until
 * low enough on screen to move to step 4.
 * step 4: "circling" phase -- orbits a fixed screen point (angleAccum),
 * aiming velocity toward that orbit position; once its countdown expires,
 * if enough attacks have been used (D_us_80180CE0 >= 3) it picks a target
 * chain pattern by player-angle sector (D_us_80180D14 table) and, if that
 * pattern isn't already active, jumps straight to the "activate pattern"
 * tail (step_s machine below, step 4); otherwise falls through to the
 * generic gate (step_s machine, default target step 4). Also globally
 * forces step 7 once D_us_80180CE0 >= 9 (an "enrage"/final-phase trigger).
 * step 5: a short 3-state pause/telegraph (SFX 0x80E then a random
 * confirmation SFX) before returning to step 4.
 * step 7: "revenge volley" -- every 32 frames fires a ring of 4 type-0x24
 * projectiles aimed via GetAngleBetweenEntities(self, player), while still
 * circling; once its own countdown expires, picks the next pattern
 * (D_us_80180D14 again) and moves to step 6.
 * step 1 / step 6 / anything else not explicitly cased (this matches the
 * raw decompilation's nested default/default structure): the pattern-
 * activation sub-machine (step_s 0-3) -- sets patternActiveMask's bit for
 * the chosen pattern, waits for D_us_80180CDC (chain segment reports back
 * "in position"), then eases velocity toward the chain link's angle
 * (D_us_80180D00 table) before decaying it, eventually returning to
 * step 4 (or 7, via the D_us_80180CE0 >= 9 check on every path through this
 * block).
 * step 8 (forced once D_us_80180CE4 bit 0x100 is set, i.e. boss defeated):
 * death cutscene -- camera pan, music swap, screen-edge flee, spawns type
 * 0x22 fade-out puffs, spawns a type-0x25 entity (likely the stage-clear/
 * relic-drop trigger), waits for music playback to finish.
 * step 0xFF: an otherwise-unreachable hidden DEBUG menu -- reads controller
 * input directly (g_pads_1_pressed) to page through animCurFrame values and
 * prints them via FntPrint ("charal %x" / "y %x"), then re-applies the
 * shared per-frame collider table (D_us_80180E34/ED8, same table already
 * used in func_us_801A2774) for whichever frame is selected. Left in the
 * retail code but not reachable through normal step transitions.
 *
 * The shared per-frame tail (skipped during the step-8 death cutscene):
 * advances a 4-frame animation counter on a timer mask, and clamps the
 * boss's position to stay on screen based on current velocity direction.
 * NOTE: the else-branch of the Y clamp writes to posX instead of posY --
 * preserved exactly as decompiled; this looks like a genuine original-game
 * bug/typo, not a decompilation error.
 *
 * Confidence: medium-high on overall control flow and named Entity fields
 * (all confirmed). Lower confidence on: D_8003C506/D_8003C508's real
 * purpose, D_us_80180D14's exact element type (declared as u32[8], stride
 * inferred from the 0x1C bitmask seen in the raw pointer arithmetic), the
 * g_api_func_80102CD8/80131F68 function pointers, and ext+0xA (unk86)'s
 * role beyond clearly being part of the same animation-timer trio as
 * animCounter/animCounterMask. Not yet verified against a real compile.
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 counter;
    /* 0x82 (ext+6) */ u16 angleAccum;
    /* 0x84 (ext+8) */ s8 patternActiveMask;
    /* 0x85 (ext+9) */ u8 currentPattern;
    /* 0x86 (ext+0xA) */ u8 unk86; // TODO: part of the animCounter/animCounterMask trio, exact role unclear
    /* 0x98 (ext+0x1C) */ u8 animCounter;
    /* 0x99 (ext+0x1D) */ u8 animCounterMask;
} ET_801A1878;

// Forward declarations: shared with func_us_801A2774 (defined later in this
// file), but also needed here since func_us_801A1878 reads the same target
// pointer / per-frame collider table in its own step-0xFF debug menu.
typedef struct {
    /* 0x9C (ext+0x20) */ Entity* target;
} ET_801A2774;
extern u8 D_us_80180ED8[];  // animCurFrame -> collider-table index
extern s32 D_us_80180E34[]; // packed per-frame collider table (see comment at func_us_801A2774)

void func_us_801A1878(Entity* self) {
    Entity* segment;
    Entity* projectile;
    Entity* cutsceneEntity;
    s16 diveTargetX;
    s16 diveTargetY;
    s16 orbitAngle;
    s16 orbitX;
    s16 aimAngle;
    s16 fireAngle;
    s32 pattern;
    s32 i;
    s32 nextStep;
    ET_801A1878* ext;

    ext = (ET_801A1878*) &self->ext;

    if (D_us_80180CD8 & 0x100) {
        self->hitboxState = 3;
    }
    if ((self->flags & 0x100) && self->step < 8) {
        SetStep(8);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B10);
            self->zPriority = 0x40;
            self->animCurFrame = 1;
            self->hitboxState = 0;
            D_us_80180CE4 = 0;
            D_us_80180CD8 = 0;
            D_us_80180CE0 = 0;
            self->posX.i.hi = 0x100 - g_Tilemap.scrollX.i.hi;
            self->posY.i.hi = 0x240 - g_Tilemap.scrollY.i.hi;

            segment = self + 10;
            for (i = 0; i < 8; i++) {
                CreateEntityFromEntity(0x19, self, segment);
                segment->params = i;
                segment->zPriority = self->zPriority + 1; // confirmed: m2c's "var_s1->unk-C" is (segment+0x30)-0xC = segment->zPriority (0x24)
                segment += 5;
            }
            segment = self + 1;
            for (i = 0; i < 9; i++) {
                CreateEntityFromEntity(0x1C, self, segment);
                segment->params = i;
                ((ET_801A2774*) &segment->ext)->target = self; // confirmed: m2c's "var_s1_2->unk6C" is (segment+0x30)+0x6C = segment's ext+0x20 "target" pointer -- same convention as entity types 0x1E/0x1B
                segment += 1;
            }

            ext->animCounterMask = 7;
            ext->unk86 = 0;
            SetStep(2);
            // fallthrough
        case 2:
            if (GetDistanceToPlayerX() < 0x80) {
                SetStep(3);
            }
            break;

        case 3:
            switch (self->step_s) {
                case 0:
                    g_api_TimeAttackController(3, 2);
                    D_us_80180CD4 = 1;
                    currentMusicId = 0x334;
                    stopMusicFlag = 0;
                    D_us_80180CE4 |= 0x10;
                    g_api_PlaySfx(0x334);
                    self->velocityY = -0x10000;
                    self->step_s++;
                    // fallthrough
                case 1:
                    MoveEntity();
                    if (!(g_Timer & 0xF)) {
                        PlaySfxPositional(0x7CD);
                        g_api_func_80102CD8(1);
                    }
                    diveTargetY = self->posY.i.hi + g_Tilemap.scrollY.i.hi;
                    FntPrint("y %x\n", diveTargetY);
                    {
                        s32 stillDiving = diveTargetY < 0x1C0;
                        if (diveTargetY < 0x100) {
                            SetStep(4);
                            stillDiving = diveTargetY < 0x1C0;
                        }
                        if (stillDiving) {
                            ext->unk86 = 0x7F;
                        }
                    }
                    {
                        s32 spreadX = (rcos(ratan2(diveTargetY - 0x140, 0x70)) * 0x70) >> 0xC;
                        s32 baseX = self->posX.i.hi - spreadX;
                        s32 jitter = (spreadX * 2 * Random()) >> 8;
                        s16 dropY = 0x1B0 - g_Tilemap.scrollY.i.hi;

                        projectile = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (projectile != NULL) {
                            CreateEntityFromEntity(2, self, projectile);
                            projectile->zPriority = 0x70;
                            projectile->posX.i.hi = baseX + jitter;
                            projectile->posY.i.hi = dropY;
                            projectile->params = 0x11;
                        }
                    }
                    break;
            }
            break;

        case 4:
            if (self->step_s == 0) {
                ext->animCounterMask = 0xF;
                ext->unk86 = 0xFF;
                ext->counter = 0x180;
                ext->patternActiveMask = 0;
                self->step_s++;
            }
            MoveEntity();
            orbitX = 0x100 - g_Tilemap.scrollX.i.hi;
            {
                s16 orbitY = 0x120 - g_Tilemap.scrollY.i.hi;
                orbitAngle = (ext->angleAccum + 0x10) & 0xFFF;
                ext->angleAccum += 0x10;
                {
                    s32 targetX = orbitX + ((rcos(orbitAngle) * 0x50) >> 0xC);
                    s32 targetY = orbitY + ((rsin(orbitAngle) * 0x50) >> 0xC);
                    aimAngle = ratan2(targetY - self->posY.i.hi, targetX - self->posX.i.hi);
                    self->velocityX = (rcos(aimAngle) * 0x6000) >> 0xC;
                    self->velocityY = (rsin(aimAngle) * 0x6000) >> 0xC;
                }
            }
            if (--ext->counter == 0) {
                SetStep(5);
                if (D_us_80180CE0 >= 3) {
                    fireAngle = ratan2(PLAYER_posY_i_hi - self->posY.i.hi, PLAYER_posX_i_hi - self->posX.i.hi);
                    pattern = D_us_80180D14[((fireAngle + 0x100) >> 7) & 7];
                    nextStep = 6;
                    if ((D_us_80180CD8 >> pattern) & 1) {
                        ext->currentPattern = (u8) pattern;
                        goto do_set_step;
                    }
                }
            }
            goto step_tail_check;

        case 5:
            switch (self->step_s) {
                case 0:
                    ext->counter = 0x40;
                    ext->animCounterMask = 3;
                    ext->unk86 = 0;
                    g_api_PlaySfx(0x80E);
                    self->step_s++;
                    // fallthrough
                case 1:
                    if (--ext->counter == 0) {
                        ext->counter = 0x100;
                        ext->animCounterMask = 3;
                        ext->unk86 = 0x1F;
                        self->step_s++;
                        g_api_PlaySfx((Random() & 1) ? 0x8C0 : 0x8C1);
                    }
                    break;
                case 2:
                    if (--ext->counter == 0) {
                        SetStep(4);
                    }
                    break;
            }
            break;

        case 7:
            if (self->step_s == 0) {
                ext->counter = 0x80;
                ext->patternActiveMask = 0;
                ext->angleAccum = Random() * 0x10;
                self->step_s++;
            }
            if (!(ext->counter & 0x1F)) {
                PlaySfxPositional(0x61E);
                fireAngle = GetAngleBetweenEntities(self, g_Entities) - 0xB00;
                for (i = 0; i < 4; i++) {
                    projectile = AllocEntity(g_Entities + 0x69C0 / sizeof(Entity), (Entity*) ((u8*) g_Entities + 0x69C0 + 0x2340));
                    if (projectile != NULL) {
                        CreateEntityFromEntity(0x24, self, projectile);
                        projectile->rotate = fireAngle;
                    }
                    fireAngle += 0x200;
                }
            }
            MoveEntity();
            orbitX = 0x100 - g_Tilemap.scrollX.i.hi;
            {
                s16 orbitY = 0x120 - g_Tilemap.scrollY.i.hi;
                orbitAngle = (ext->angleAccum + 0x20) & 0xFFF;
                ext->angleAccum += 0x20;
                {
                    s32 targetX = orbitX + ((rcos(orbitAngle) << 6) >> 0xC);
                    s32 targetY = orbitY + ((rsin(orbitAngle) << 6) >> 0xC);
                    aimAngle = ratan2(targetY - self->posY.i.hi, targetX - self->posX.i.hi);
                    self->velocityX = (rcos(aimAngle) << 0x10) >> 0xC;
                    self->velocityY = (rsin(aimAngle) << 0x10) >> 0xC;
                }
            }
            if (--ext->counter == 0) {
                fireAngle = ratan2(PLAYER_posY_i_hi - self->posY.i.hi, PLAYER_posX_i_hi - self->posX.i.hi);
                ext->currentPattern = (u8) D_us_80180D14[((fireAngle + 0x100) >> 7) & 7];
                SetStep(6);
            }
            break;

        default:
            /* step 1, step 6, and anything else fall through to here --
             * matches the raw decompilation's nested default/default
             * structure exactly. */
            if (self->step == 8) {
                switch (self->step_s) {
                    case 0:
                        g_api_TimeAttackController(3, 1);
                        D_us_80180CE4 |= 1;
                        g_api_PlaySfx(0x92);
                        currentMusicId = 0x307;
                        self->velocityY = -0xC000;
                        self->velocityX = 0;
                        self->step_s++;
                        // fallthrough
                    case 1:
                        MoveEntity();
                        self->velocityY += 0x400;
                        if (self->velocityY > 0) {
                            ext->counter = 0x80;
                            D_us_80180CE4 |= 8;
                            self->drawFlags = 4;
                            self->opacity = 0x80;
                            self->step_s++;
                            self->drawFlags |= 8;
                        }
                        break;
                    case 2:
                        MoveEntity();
                        self->velocityY = 0x8000;
                        self->rotate -= 0x40;
                        if (!(ext->counter & 3)) {
                            self->opacity += 0xFF; // TODO: literal translation of "+= 0xFF" (i.e. -1 mod 256) on a u8 field
                        }
                        if (!(ext->counter & 7)) {
                            PlaySfxPositional(0x661);
                        }
                        if (--ext->counter == 0) {
                            D_us_80180CE4 |= 4;
                            self->step_s++;
                        }
                        break;
                    case 3:
                        MoveEntity();
                        self->velocityY += 0x2000;
                        if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x1B1) {
                            self->scaleX = 0x100;
                            self->step_s++;
                            PlaySfxPositional(0x7C5);
                        }
                        break;
                    case 4:
                        self->animCurFrame = 0;
                        for (i = 0; i < 2; i++) {
                            projectile = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                            if (projectile != NULL) {
                                CreateEntityFromEntity(0x22, self, projectile);
                                projectile->params = 6;
                                projectile->scaleX = self->scaleX;
                                projectile->zPriority = self->zPriority;
                            }
                        }
                        ext->counter++;
                        self->scaleX--;
                        if (!(ext->counter & 0x1F)) {
                            PlaySfxPositional(0x7C5);
                        }
                        if (self->scaleX == 0) {
                            self->step_s++;
                        }
                        break;
                    case 5:
                        cutsceneEntity = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (cutsceneEntity != NULL) {
                            CreateEntityFromEntity(0x25, self, cutsceneEntity);
                            stopMusicFlag = 1;
                            D_us_80180CD4 = 0;
                            cutsceneEntity->posX.i.hi = 0x100 - g_Tilemap.scrollX.i.hi;
                            cutsceneEntity->params = 1;
                            cutsceneEntity->posY.i.hi = 0x150 - g_Tilemap.scrollY.i.hi;
                            currentMusicId = 0x307;
                            self->step_s++;
                        }
                        break;
                    case 6:
                        if (g_api_func_80131F68() == 0) {
                            stopMusicFlag = 0;
                            g_api_PlaySfx(currentMusicId);
                            self->step_s++;
                        }
                        break;
                }
                break;
            } else if (self->step == 0xFF) {
                /* Hidden debug menu -- unreachable via normal step
                 * transitions (no code anywhere sets step to 0xFF). Reads
                 * controller input to page through animCurFrame values. */
                FntPrint("charal %x\n", self->animCurFrame);
                if (g_pads_1_pressed & 0x80) {
                    if (self->params == 0) {
                        self->animCurFrame++;
                        self->params |= 1;
                        goto debug_check_down;
                    }
                } else {
                    self->params = 0;
                debug_check_down:
                    if (g_pads_1_pressed & 0x20) {
                        if (self->step_s == 0) {
                            self->animCurFrame--;
                            self->step_s |= 1;
                        }
                    } else {
                        self->step_s = 0;
                        {
                            u8* frameCollider = (u8*) &D_us_80180E34[D_us_80180ED8[self->animCurFrame]];
                            self->hitboxOffX = (s8) frameCollider[0];
                            self->hitboxOffY = (s8) frameCollider[1];
                            self->hitboxWidth = frameCollider[5];
                            self->hitboxHeight = frameCollider[9];
                        }
                    }
                }
                break;
            }

            switch (self->step_s) {
                case 0:
                    ext->animCounterMask = 7;
                    ext->unk86 = 0;
                    D_us_80180CDC = 0;
                    self->velocityX = 0;
                    self->velocityY = 0;
                    ext->patternActiveMask = (s8) (1 << ext->currentPattern);
                    self->step_s++;
                    // fallthrough
                case 1:
                    if (D_us_80180CDC != 0) {
                        self->step_s++;
                    }
                    break;
                case 2:
                    diveTargetX = D_us_80180D00[ext->currentPattern] + 0x800;
                    self->velocityX = (rcos(diveTargetX) << 0x11) >> 0xC;
                    self->velocityY = (rsin(diveTargetX) << 0x11) >> 0xC;
                    ext->animCounterMask = 0xFF;
                    ext->unk86 = 0;
                    self->step_s++;
                    // fallthrough
                case 3:
                    MoveEntity();
                    self->velocityX -= self->velocityX >> 4;
                    self->velocityY -= self->velocityY >> 4;
                    if (D_us_80180CDC == 0) {
                        nextStep = 4;
                        ext->patternActiveMask = 0;
                    do_set_step:
                        SetStep(nextStep);
                    step_tail_check:
                        if (D_us_80180CE0 >= 9) {
                            SetStep(7);
                        }
                    }
                    break;
            }
            break;
    }

    if (!(self->flags & 0x100)) {
        D_8003C506 = D_8003C508[ext->animCounter];
        if (!(ext->animCounterMask & g_Timer)) {
            ext->animCounter = (ext->animCounter + 1) & 3;
        }
        {
            s16 clampX;
            s32 clampedX;
            if (self->velocityX > 0) {
                clampX = 0x160 - g_Tilemap.scrollX.i.hi;
                clampedX = clampX < self->posX.i.hi;
            } else {
                clampX = 0xA0 - g_Tilemap.scrollX.i.hi;
                clampedX = self->posX.i.hi < clampX;
            }
            if (clampedX) {
                self->posX.i.hi = clampX;
            }
        }
        if (self->velocityY > 0) {
            s16 clampY = 0x180 - g_Tilemap.scrollY.i.hi;
            if (clampY < self->posY.i.hi) {
                self->posY.i.hi = clampY;
            }
        } else {
            s16 clampY = 0xC0 - g_Tilemap.scrollY.i.hi;
            if (self->posY.i.hi < clampY) {
                self->posX.i.hi = clampY; // NOTE: preserved exactly as decompiled -- writes posX, not posY; looks like an original bug
            }
        }
    }
}

/*
 * Granfaloon "warning shrapnel" attack-pattern entity, one instance per
 * params (0-8ish), controlled by the main body (func_us_801A1334) through 3
 * shared globals: D_us_80180CD4 (impact flag), D_us_80180CD8 (per-pattern
 * "done" bitmask), D_us_80180CE0 (completion counter).
 *
 * step 0: InitializeEntity, sets hitbox/zPriority/animCurFrame from the
 * shared D_us_80180D34 table (see typedef above), reads a per-frame collider
 * table (D_us_80180E34, indexed via D_us_80180ED8[animCurFrame]) to set
 * hitboxOffX/Y/Width/Height, and -- if the pattern's variantFrameId is
 * nonzero -- spawns an escort entity (type 0x1E). That escort's own ext+0x20
 * pointer (0x9C) is set to `self`, and func_us_801A2CC4 (which reads exactly
 * that pointer/offset to follow a target's position every frame) is
 * confirmed to be entity type 0x1E's update function.
 *
 * step 1: gate -- waits for D_us_80180CD4 (impact already happened) before
 * advancing to step 2.
 *
 * step 2: follows its escort-parent's position (same ext+0x9C pointer
 * pattern), then rolls a random chance (parent's ext+0xA byte, offset 0x86)
 * to fire a projectile (type 0x20 or 0x21) toward an angle-offset position.
 *
 * step 3: fires once (gated on step_s == 0) -- the "boss slam" shockwave:
 * marks its pattern bit done in D_us_80180CD8, spawns 2 marker entities
 * (type 0x1D) and 24 spark entities (type 0x1F) in a ring, plays SFX 0x7C7.
 *
 * Confidence: high on control flow and named-field spawns/positions
 * (all cross-referenced against confirmed Entity fields and the entity-type
 * 0x1E connection to func_us_801A2CC4). Lower confidence on: the exact byte
 * layout of the D_us_80180E34 collider table (kept as raw offset arithmetic
 * rather than a guessed struct -- see comment at the read site), and the
 * true meaning of the parent's ext+0xA "chance" byte (offset 0x86). Not yet
 * verified against a real compile/asm-differ pass.
 */
extern Entity D_8007CC98[]; // sub-slice of g_Entities (escort entity pool)
extern Entity D_80079D98[]; // sub-slice of g_Entities (projectile entity pool)

void func_us_801A2774(Entity* self) {
    Entity* escort;
    Entity* parent;
    Entity* marker;
    Entity* spark;
    Entity* sparkBase;
    Entity* projectile;
    s16 angle;
    s16 x;
    s16 y;
    s32 i;

    if (D_us_80180CE4 & 1) {
        self->flags |= 0x100; // TODO: unnamed bit, same as func_us_801A4394/45D0
    }
    if ((self->flags & 0x100) && self->step < 3) {
        SetStep(3);
    }

    switch (self->step) {
        case 0: {
            D_us_80180D34_Entry* entry = &D_us_80180D34[self->params];
            u8* frameCollider;

            InitializeEntity(D_us_80180B40);
            self->animCurFrame = entry->animCurFrame;
            self->zPriority = entry->zPriority;

            // TODO: D_us_80180E34's true element layout isn't resolved -- m2c
            // shows byte 1 of 3 *consecutive* 4-byte entries being read
            // (offsets +0, +1, +5, +9 from the indexed base), which doesn't
            // match a single clean struct. Kept as raw offsets rather than
            // guessing a wrong shape.
            frameCollider = (u8*) &D_us_80180E34[D_us_80180ED8[self->animCurFrame]];
            self->hitboxOffX = (s8) frameCollider[0];
            self->hitboxOffY = (s8) frameCollider[1];
            self->hitboxWidth = frameCollider[5];
            self->hitboxHeight = frameCollider[9];

            if (entry->variantFrameId != 0) {
                escort = AllocEntity(D_8007CC98, (Entity*) ((u8*) D_8007CC98 + 0x2340));
                if (escort != NULL) {
                    CreateEntityFromEntity(0x1E, self, escort);
                    ((ET_801A2774*) &escort->ext)->target = self;
                    escort->params = self->params;
                }
            }
            // fallthrough
        }
        case 1:
            self->hitboxState = 0;
            if (D_us_80180CD4 != 0) {
                self->hitboxState = 3;
                self->step++;
            }
            return;

        case 2:
            parent = ((ET_801A2774*) &self->ext)->target;
            self->posX.i.hi = parent->posX.i.hi;
            self->posY.i.hi = parent->posY.i.hi;

            // TODO: parent's ext+0xA byte (offset 0x86) -- guessed as a
            // "fireball chance" divisor; not cross-referenced elsewhere.
            if (*((u8*) parent + 0x86) != 0 && !(Random() % *((u8*) parent + 0x86))) {
                x = self->posX.i.hi;
                y = self->posY.i.hi;
                if (self->params != 8) {
                    angle = D_us_80180D00[self->params];
                    x += (rcos(angle) * 0x60) >> 0xC;
                    y += (rsin(angle) * 0x60) >> 0xC;
                }
                projectile = AllocEntity(D_80079D98, (Entity*) ((u8*) D_80079D98 + 0x2340));
                if (projectile != NULL) {
                    CreateEntityFromCurrentEntity((Random() & 1) ? 0x20 : 0x21, projectile);
                    projectile->zPriority = 0x4A;
                    projectile->posX.i.hi = x;
                    projectile->posY.i.hi = y;
                    projectile->params = self->params;
                    return;
                }
            }
            break;

        case 3:
            if (self->step_s == 0) {
                self->hitboxState = 0;
                self->animCurFrame = 0;
                D_us_80180CE0++;
                D_us_80180CD8 |= 1 << self->params;

                for (i = 0; i < 2; i++) {
                    marker = AllocEntity(g_Entities_192, (Entity*) ((u8*) g_Entities_192 + 0x2F00));
                    if (marker != NULL) {
                        CreateEntityFromEntity(0x1D, self, marker);
                        marker->zPriority = 0x48;
                        marker->params = self->params + (i << 8);
                    }
                }

                x = self->posX.i.hi;
                y = self->posY.i.hi;
                if (self->params != 8) {
                    angle = D_us_80180D00[self->params];
                    x += (rcos(angle) * 0x50) >> 0xC;
                    y += (rsin(angle) * 0x50) >> 0xC;
                }

                sparkBase = (Entity*) ((u8*) g_Entities_192 + 0xBC0);
                for (i = 0; i < 0x18; i++) {
                    spark = AllocEntity(sparkBase, (Entity*) ((u8*) sparkBase + 0x1780));
                    if (spark != NULL) {
                        CreateEntityFromCurrentEntity(0x1F, spark);
                        spark->zPriority = 0x4A;
                        spark->posX.i.hi = x;
                        spark->posY.i.hi = y;
                    }
                }
                PlaySfxPositional(0x7C7);
                self->step_s++;
            }
            break;
    }
}

/*
 * Companion projectile for func_us_801A2774's attack pattern: launches at an
 * angle derived from the shared D_us_80180D34 table (offset by params &
 * 0x100), applying gravity each frame. Confidence: high -- pure integer
 * arithmetic on confirmed fields, no unresolved globals besides the shared
 * D_us_80180D00/D_us_80180D34 tables already cross-validated elsewhere.
 */
void func_us_801A2BC4(Entity* self) {
    D_us_80180D34_Entry* entry;
    s16 angle;
    u16 idx;
    u16 params;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B4C);
            params = self->params;
            idx = params & 0xF;
            self->flags |= 0x80000000; // TODO: unnamed flag bit, also seen in func_us_801A1334/2D90
            angle = D_us_80180D00[idx];
            entry = &D_us_80180D34[idx];
            if (params & 0x100) {
                self->animCurFrame = entry->altAnimCurFrame1;
                angle -= 0x80;
            } else {
                self->animCurFrame = entry->altAnimCurFrame0;
                angle += 0x80;
            }
            self->velocityX = rsin(angle) * 0x10;
            self->velocityY = rcos(angle) * 0x10;
            break;

        case 1:
            MoveEntity();
            self->velocityY += 0x1000; // gravity
            break;
    }
}

/*
 * Entity type 0x1E's update function: an invisible "escort" that tracks a
 * parent entity's exact integer position every frame (ext+0x9C pointer set
 * by func_us_801A2774 at spawn time), then self-destructs once its pattern
 * bit is marked done in D_us_80180CD8. Confidence: high -- confirmed by
 * cross-referencing the spawn site in func_us_801A2774.
 */
typedef struct {
    /* 0x9C (ext+0x20) */ Entity* target;
} ET_801A2CC4;

void func_us_801A2CC4(Entity* self) {
    Entity* target;

    if (D_us_80180CE4 & 1) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B4C);
            self->zPriority = 0x3C;
            self->animCurFrame = D_us_80180D34[self->params].variantFrameId;
            // fallthrough
        case 1:
            target = ((ET_801A2CC4*) &self->ext)->target;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            if ((D_us_80180CD8 >> self->params) & 1) {
                DestroyEntity(self);
            }
            break;
    }
}

/*
 * Generic spinning debris/spark particle: launched at a random angle/speed,
 * halved if params != 0, tumbles (rotate += 0x40/frame) under gravity
 * (terminal velocity capped at 0x2FFFF), despawns once it scrolls off the
 * bottom of the screen. Confidence: high -- pure integer arithmetic on
 * confirmed fields; only the ext+4 "life" field's consumer is unresolved
 * (written here, not read anywhere in this file).
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 life; // TODO: written here, not read by any function in this file
} ET_801A2D90;

void func_us_801A2D90(Entity* self) {
    s16 angle;
    s32 speed;
    s32 velocityX;
    s32 velocityY;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B4C);
            self->flags |= 0x80000000; // TODO: unnamed flag bit, also seen in func_us_801A1334/2BC4
            self->animCurFrame = (Random() & 3) + 0x79;
            self->drawFlags = 4;
            self->facingLeft = Random() & 1;
            speed = (Random() & 0x1F) + 0x10;
            angle = (Random() * 6) + 0x900;
            velocityX = (speed * rcos(angle)) >> 1;
            velocityY = speed * rsin(angle);
            if (self->params != 0) {
                velocityX >>= 1;
                velocityY >>= 1;
            }
            self->velocityX = velocityX;
            self->velocityY = velocityY;
            self->posX.val += velocityX * 0x10;
            self->posY.val += velocityY * 0x10;
            ((ET_801A2D90*) &self->ext)->life = (Random() & 0x1F) + 0x10;
            // fallthrough
        case 1:
            MoveEntity();
            self->rotate += 0x40;
            if (self->velocityY <= 0x2FFFF) {
                self->velocityY += 0x1400; // gravity, capped terminal velocity
            }
            break;
    }

    if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x1C9) {
        DestroyEntity(self);
    }
}

/*
 * Granfaloon "eye" chain-segment: root = self - (params*5 + 10) (same
 * shared-allocation-block pattern as func_us_801A2774, confirmed by exact
 * offset math: sizeof(Entity) == 0xBC, and 5*0xBC + 10*0xBC == 0x3AC*params
 * + 0x758 as m2c reported it in raw pointer form).
 *
 * step 0: spawns a type-0x1A companion at self+1 (same "next slot" pattern
 * as func_us_801A3480's type-0x1B companion), initializing its zPriority
 * and params.
 *
 * step 1: idle -- follows root's position (D_us_80180D90 offset table,
 * shared with func_us_801A2774/3480), copies a per-segment "visible" bit
 * from root's ext+8 into its own ext+8 (TODO: this conflicts with
 * ET_801A1334.hasSpawnedThirdDebris being a single bool at the same offset
 * -- "root" here is almost certainly a different anchor entity, not
 * func_us_801A1334's main body). If the pattern's D_us_80180CD8 bit is set:
 * either idly swings self->rotate (root not "visible") or smoothly aims
 * toward the player via GetAngleBetweenEntities, clamped to +-0x80, using
 * func_us_801A17F8 as the smoothing/easing function in both cases (this is
 * the same generic "ease toward target" helper already decompiled).
 *
 * step 2 (forced via SetStep once D_us_80180CE4 bit 0 is set, i.e. the
 * boss's global "attack ending" signal used throughout this file): keeps
 * following root's position, drives a continuous oscillating rotate, and
 * runs its own step_s sub-machine that periodically fires homing type-0x22
 * projectiles using ROOT's rotate field as an aim reference (SFX 0x7C6 for
 * the periodic volley, 0x662 once, then a final volley using root's own
 * rotate + this segment's angle offset before presumably being cleaned up
 * by the boss's death sequence).
 *
 * Confidence: medium on control flow (faithful to the decompilation); low
 * on which physical entity "root" is (offset math is solid, cross-entity
 * relationship to func_us_801A1334 is not confirmed), and on
 * D_us_80180D00/D_us_80180D90's semantics beyond what's exercised here.
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 counter;
    /* 0x82 (ext+6) */ s16 orbitAngle;
    /* 0x84 (ext+8) */ u8 chainVisible; // copied from root's ext+8 -- see TODO above
} ET_801A2F2C;

void func_us_801A2F2C(Entity* self) {
    Entity* root;
    Entity* companion;
    Entity* projectile;
    s16 angle;
    s32 fireAngle;

    if ((D_us_80180CE4 & 1) && self->step < 2) {
        SetStep(2);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B1C);
            self->drawFlags = 8;
            self->rotate = 0;
            self->drawFlags |= 4;
            self->opacity = (u8) D_us_80180DD0[self->params];
            companion = self + 1;
            CreateEntityFromEntity(0x1A, self, companion);
            companion->zPriority = self->zPriority + 1;
            companion->params = self->params;
            // fallthrough
        case 1:
            root = self - (self->params * 5 + 10);
            self->posX.i.hi = root->posX.i.hi + D_us_80180D90[self->params].x;
            self->posY.i.hi = root->posY.i.hi + D_us_80180D90[self->params].y;
            ((ET_801A2F2C*) &self->ext)->chainVisible =
                *((u8*) &root->ext + 8) & (1 << self->params);
            AnimateEntity(D_us_80180DF4, self);
            self->animCurFrame += self->params * 7;

            if ((D_us_80180CD8 >> self->params) & 1) {
                if (((ET_801A2F2C*) &self->ext)->chainVisible == 0) {
                    self->drawFlags |= 4;
                    func_us_801A17F8(&self->rotate,
                        (rcos(((ET_801A2F2C*) &self->ext)->orbitAngle) << 0xB) >> 0x10, 0x10);
                    ((ET_801A2F2C*) &self->ext)->orbitAngle += 0x30;
                    return;
                }
                angle = GetAngleBetweenEntities(root, g_Entities) - D_us_80180D00[self->params];
                if (angle >= 0x81) {
                    angle = 0x80;
                } else if (angle < -0x80) {
                    angle = -0x80;
                }
                func_us_801A17F8(&self->rotate, angle, 2);
                return;
            }
            return;

        case 2:
            root = self - (self->params * 5 + 10);
            self->posX.i.hi = root->posX.i.hi + D_us_80180D90[self->params].x;
            self->posY.i.hi = root->posY.i.hi + D_us_80180D90[self->params].y;
            func_us_801A17F8(&self->rotate,
                (rcos(((ET_801A2F2C*) &self->ext)->orbitAngle) * 0xA00) >> 0x10, 0x40);
            ((ET_801A2F2C*) &self->ext)->orbitAngle += 0x70;

            switch (self->step_s) {
                case 0:
                    self->hitboxState = 0;
                    ((ET_801A2F2C*) &self->ext)->counter = self->params;
                    self->step_s++;
                    return;

                case 1:
                    ((ET_801A2F2C*) &self->ext)->counter++;
                    if (!(((ET_801A2F2C*) &self->ext)->counter & 0xF)) {
                        PlaySfxPositional(0x7C6);
                        projectile = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                        if (projectile != NULL) {
                            CreateEntityFromEntity(0x22, self, projectile);
                            projectile->params = 4;
                            projectile->zPriority = 0x70;
                            projectile->rotate = D_us_80180D00[self->params];
                        }
                    }
                    if (D_us_80180CE4 & 8) {
                        PlaySfxPositional(0x662);
                        self->step_s++;
                    }
                    break;

                default:
                    if (self->step_s != self->step) {
                        if (self->step_s != 3) {
                            return;
                        }
                        DestroyEntity(self);
                    } else {
                        root = self - (self->params * 5 + 10);
                        self->animCurFrame = 0;
                        ((ET_801A2F2C*) &self->ext)->counter++;
                        self->posX.i.hi = root->posX.i.hi;
                        self->posY.i.hi = root->posY.i.hi;
                        fireAngle = D_us_80180D00[self->params] + root->rotate;
                        self->posX.i.hi += (rcos(fireAngle) * 0x18) >> 0xC;
                        self->posY.i.hi += (rsin(fireAngle) * 0x18) >> 0xC;
                        if (!(((ET_801A2F2C*) &self->ext)->counter & 3)) {
                            projectile = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                            if (projectile != NULL) {
                                CreateEntityFromEntity(0x22, self, projectile);
                                projectile->params = 5;
                                projectile->rotate = fireAngle + 0x200;
                                projectile->zPriority = 0x70;
                            }
                        }
                        if (D_us_80180CE4 & 4) {
                            self->step_s++;
                        }
                    }
                    break;
            }
            break;
    }
}

/*
 * Granfaloon chain-tentacle segment: unlike func_us_801A2F2C (which follows
 * a fixed "root" computed from self->params), each instance of this
 * function follows the entity immediately BEFORE it in the array
 * (prev = self - 1, one Entity struct back) -- confirmed precisely: the
 * negative byte offsets m2c reported (-0x9E, -0xBA, -0xB6, -0x38) land
 * exactly on (self-1)'s rotate (0x1E), posX.i.hi (0x02), posY.i.hi (0x06),
 * and ext+8 byte respectively, since sizeof(Entity) (0xBC/188) minus each
 * magnitude gives exactly that known field offset. This makes the chain a
 * simple segment-follows-segment rope, distinct from func_us_801A2F2C's
 * fixed-root satellites.
 *
 * step 0: spawn init, initial rotate from D_us_80180D00[params].
 * step 1/2/3 all open with the same "follow the previous segment" block:
 * copy prev's ext+8 byte, compute an angle from it, offset position by
 * 0x28 along that angle, update rotate. Then branch on step_s:
 *   step 2 runs a 4-state sub-machine: waits for an animation
 *   (D_us_80180E10), spawns a type-0x1B visual companion at self+1 (whose
 *   ext+0x9C "target" pointer is set to `self` -- the same convention
 *   confirmed for entity type 0x1E in func_us_801A2774/2CC4, now also
 *   apparently used by type 0x1B), waits for that companion to still be
 *   type 0x1B, plays a second animation (D_us_80180E18), then loops back
 *   to step 1.
 *   step 3 runs a 3-state sub-machine: idle counter, periodic type-0x22
 *   projectile spawn (every 8 frames) while D_us_80180CE4 bit 3 is clear,
 *   then self-destructs once that bit is set.
 *
 * Confidence: high on control flow and the previous-segment-follow
 * mechanism (byte math is exact, not a guess). Lower confidence on the
 * D_us_80180E10/E18 animation tables' content and the shared ext+8 "chain
 * active" byte's real meaning beyond "nonzero enables it".
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 counter;
} ET_801A3480;

void func_us_801A3480(Entity* self) {
    Entity* prev;
    Entity* companion;
    Entity* projectile;
    s16 angle;
    u8 prevActive;

    if ((D_us_80180CE4 & 1) && self->step < 3) {
        SetStep(3);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B1C);
            self->animCurFrame = 0x3A;
            self->drawFlags = 0xC;
            self->opacity = (u8) D_us_80180DD0[self->params];
            self->rotate = D_us_80180D00[self->params] - 0x400;
            // fallthrough
        case 1:
            prev = self - 1;
            prevActive = *((u8*) &prev->ext + 8);
            self->posX.i.hi = prev->posX.i.hi;
            self->posY.i.hi = prev->posY.i.hi;
            angle = D_us_80180D00[self->params] + prevActive;
            self->rotate = angle - 0x400;
            self->posX.i.hi += (rcos(angle) * 0x28) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x28) >> 0xC;
            if (prevActive != 0) {
                SetStep(2);
            }
            return;

        case 2:
            prev = self - 1;
            prevActive = *((u8*) &prev->ext + 8);
            self->posX.i.hi = prev->posX.i.hi;
            self->posY.i.hi = prev->posY.i.hi;
            angle = D_us_80180D00[self->params] + prevActive;
            self->rotate = angle - 0x400;
            self->posX.i.hi += (rcos(angle) * 0x28) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x28) >> 0xC;

            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180E10, self) == 0) {
                        SetSubStep(1);
                        return;
                    }
                    break;

                case 1:
                    companion = self + 1;
                    CreateEntityFromEntity(0x1B, self, companion);
                    ((ET_801A3480*) &self->ext)->counter = 0x100;
                    ((ET_801A2774*) &companion->ext)->target = self; // ext+0x9C, same convention as entity 0x1E
                    companion->rotate = self->rotate;
                    companion->zPriority = self->zPriority + 1;
                    self->step_s++;
                    // fallthrough
                case 2:
                    companion = self + 1;
                    if (companion->entityId != 0x1B) {
                        self->step_s++;
                        return;
                    }
                    break;

                case 3:
                    if (AnimateEntity(D_us_80180E18, self) == 0) {
                        SetStep(1);
                        return;
                    }
                    break;
            }
            break;

        case 3:
            prev = self - 1;
            prevActive = *((u8*) &prev->ext + 8);
            self->posX.i.hi = prev->posX.i.hi;
            self->posY.i.hi = prev->posY.i.hi;
            angle = D_us_80180D00[self->params] + prevActive;
            self->rotate = angle - 0x400;
            self->posX.i.hi += (rcos(angle) * 0x28) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x28) >> 0xC;

            switch (self->step_s) {
                case 0:
                    self->hitboxState = 0;
                    ((ET_801A3480*) &self->ext)->counter = self->params;
                    self->step_s++;
                    return;

                case 1:
                    ((ET_801A3480*) &self->ext)->counter++;
                    if (!(((ET_801A3480*) &self->ext)->counter & 7)) {
                        projectile = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                        if (projectile != NULL) {
                            CreateEntityFromEntity(0x22, self, projectile);
                            projectile->params = 3;
                            projectile->zPriority = 0x70;
                        }
                    }
                    if (D_us_80180CE4 & 8) {
                        self->step_s++;
                        return;
                    }
                    break;

                case 2:
                    self->animCurFrame = 0;
                    DestroyEntity(self);
                    break;
            }
            break;
    }
}

/*
 * Granfaloon "eye" projectile/visual: renders a deformable diamond-shaped
 * Primitive (GT4-style quad, tpage 0x12, clut 0x200) stretched between
 * itself and a second tracked point ((self+1)'s posX/posY -- the type-0x1B
 * companion this function spawns in step 2, same "next slot" + ext+0x9C
 * "target" convention already confirmed in func_us_801A2774/2F2C/3480/
 * 1878). self->ext+0 holds the Primitive* directly (same convention as
 * func_us_801A10A8's ET_801A10A8.prim).
 *
 * step 0 (params == 0): spawns a type-0x1B tracker companion offset from
 * itself by rotate+0x400 (perpendicular offset), allocates its Primitive,
 * initializes the quad's fixed corner colors/UVs, then falls into step 1
 * which follows its OWN ext+0x9C target (a separate parent, set by
 * whoever spawned this entity) and waits for an intro animation
 * (D_us_80180E2C) to finish before moving to step 2.
 * step 0 (params & 0x100): skips all of the above and jumps straight to
 * step 0x20 (32) or 0x10 (16) depending on further param bits -- this
 * function clearly serves multiple visual roles selected at spawn time.
 * step 2/3: "expanding ring" phase -- follows the target, accumulates a
 * growing radius (ext+0x28) outward from the target's rotate direction,
 * updates the quad's corners using both self's position and the type-0x1B
 * companion's position (the two ends of the stretched diamond), checks for
 * ground collision once (latching a landing point, ext+0x24/0xC/0xE), then
 * repeatedly spawns type-0x22 splash puffs at that landing point. Self-
 * destructs once it drifts far enough off screen (|x|,|y| >= 0x221 with a
 * -0x20 margin).
 * step 4/5: same follow+quad-update logic, but shrinking (scaleX counts
 * down by 4/frame from 0x100) instead of growing a radius, until scaleX
 * hits 0, at which point D_us_80180CDC is cleared and the entity is
 * destroyed.
 * step 16 (0x10): follows target-1's rotate/position (one entity BEFORE
 * its target -- same "read the previous entity" idea as func_us_801A3480),
 * copies drawFlags/scaleX from its actual ext+0x9C target, and -- once that
 * target is confirmed still alive as entity type 0x1B and its scaleX has
 * reached 0 -- periodically spawns a type-0x23 entity aimed opposite the
 * current rotate.
 * step 32 (0x20): follows target, plays a 2-state intro animation
 * (D_us_80180E20) then a fixed-length "settle" countdown that flickers
 * animCurFrame based on g_Timer's low bit and shrinks/grows scaleX in a
 * small ripple, before falling through to destruction (goto -> DestroyEntity)
 * once the countdown ends.
 *
 * Confidence: medium on control flow (faithful to the raw decompilation,
 * including reusing already-confirmed conventions: FLAG_HAS_PRIMS, the
 * ext+0x9C target pointer, ext+0x24 "landed" latch from func_us_801A493C).
 * Low confidence on: the exact Primitive quad field semantics beyond
 * "diamond shape with these fixed UV/color constants", and which numeric
 * entity type this function actually corresponds to (not cross-referenced
 * from a spawn site in this file). Not yet verified against a real compile.
 */
typedef struct {
    /* 0x7C (ext+0) */ Primitive* prim;
    /* 0x80 (ext+4) */ u16 counter;
    /* 0x88 (ext+0xC) */ u16 landedX;
    /* 0x8A (ext+0xE) */ u16 landedY;
    /* 0xA0 (ext+0x24) */ u32 hasLanded;
    /* 0xA4 (ext+0x28) */ s32 radius;
} ET_801A38EC;

void func_us_801A38EC(Entity* self) {
    ET_801A38EC* ext = (ET_801A38EC*) &self->ext;
    Entity* companion;
    Entity* target;
    Entity* splash;
    Entity* spawned;
    Primitive* prim;
    Collider collider;
    s16 angle;
    s16 spreadX;
    s16 spreadY;
    s32 half;

    if (D_us_80180CE4 & 1) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B28);
            self->drawFlags = 4;
            self->hitboxHeight = 8;
            self->hitboxWidth = 8;
            if (self->params != 0) {
                self->hitboxState = 0;
                if (self->params & 0x100) {
                    self->step = 0x20;
                    self->hitboxState = 0;
                    return;
                }
                self->animCurFrame = 0x64;
                self->step = 0x10;
                return;
            }

            angle = self->rotate + 0x400;
            companion = AllocEntity(D_8007CC98, (Entity*) ((u8*) D_8007CC98 + 0x2340));
            if (companion != NULL) {
                CreateEntityFromEntity(0x1B, self, companion);
                companion->params = 0x100;
                companion->rotate = self->rotate;
                companion->posX.i.hi += (rcos(angle) * 0x18) >> 0xC;
                companion->posY.i.hi += (rsin(angle) * 0x18) >> 0xC;
                companion->zPriority = self->zPriority;
                ((ET_801A2774*) &companion->ext)->target = ((ET_801A2774*) &self->ext)->target;
            }
            self->posX.i.hi += (rcos(angle) * 0x24) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x24) >> 0xC;

            {
                s32 primIdx = g_api_AllocPrimitives(4, 1);
                if (primIdx != -1) {
                    prim = &g_PrimBuf[primIdx];
                    self->primIndex = primIdx;
                    ext->prim = prim;
                    self->flags |= FLAG_HAS_PRIMS;
                    prim->tpage = 0x12;
                    prim->clut = 0x200;
                    prim->u2 = 0xB8;
                    prim->u0 = 0xB8;
                    prim->u3 = 0xD0;
                    prim->u1 = 0xD0;
                    prim->v1 = 0x42;
                    prim->v0 = 0x42;
                    prim->v3 = 0x46;
                    prim->v2 = 0x46;
                    prim->drawMode = 8;
                    prim->priority = self->zPriority - 1;
                }
            }
            // fallthrough
        case 1:
            target = ((ET_801A2774*) &self->ext)->target;
            self->rotate = target->rotate;
            angle = target->rotate + 0x400;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            self->posX.i.hi += (rcos(angle) * 0x24) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x24) >> 0xC;
            if (AnimateEntity(D_us_80180E2C, self) == 0) {
                SetStep(2);
                return;
            }
            break;

        case 2:
            target = ((ET_801A2774*) &self->ext)->target;
            self->rotate = target->rotate;
            angle = target->rotate + 0x400;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            self->posX.i.hi += (rcos(angle) * 0x24) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x24) >> 0xC;

            companion = self + 1;
            CreateEntityFromEntity(0x1B, self, companion);
            companion->params = 1;
            ((ET_801A2774*) &companion->ext)->target = self;
            angle = self->rotate + 0x400;
            companion->rotate = self->rotate;
            self->posX.i.hi += (rcos(angle) * 0x28) >> 0xC;
            ext->radius = 0x4C0000;
            self->animCurFrame = 0x66;
            D_us_80180CDC = 1;
            self->posY.i.hi += (rsin(angle) * 0x28) >> 0xC;
            ext->prim->drawMode = 2;
            PlaySfxPositional(0x80B);
            self->step++;
            // fallthrough
        case 3:
            target = ((ET_801A2774*) &self->ext)->target;
            ext->radius += 0x80000;
            self->rotate = target->rotate;
            angle = target->rotate + 0x400;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            self->posX.val += (ext->radius >> 0xC) * rcos(angle);
            self->posY.val += (ext->radius >> 0xC) * rsin(angle);

            if (!ext->hasLanded) {
                g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi, &collider, 0);
                if (collider.effects & 1) {
                    ext->hasLanded = 1;
                    ext->landedX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
                    ext->landedY = self->posY.i.hi + g_Tilemap.scrollY.i.hi;
                    if (ext->landedY >= 0x1B1) {
                        ext->landedY = 0x1B0;
                    }
                }
            } else {
                splash = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (splash != NULL) {
                    CreateEntityFromCurrentEntity(0x22, splash);
                    splash->posX.i.hi = ext->landedX - g_Tilemap.scrollX.i.hi;
                    splash->posY.i.hi = ext->landedY - g_Tilemap.scrollY.i.hi;
                    splash->posX.i.hi += Random() & 0xF;
                    splash->zPriority = 0x70;
                    splash->params = 2;
                    splash->posY.i.hi += Random() & 0xF;
                }
            }

            {
                s16 screenX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
                s16 screenY = self->posY.i.hi + g_Tilemap.scrollY.i.hi;
                if (screenX < -0x20 || screenY < -0x20 || screenX >= 0x221 || screenY >= 0x221) {
                    self->step++;
                }
            }

            angle = self->rotate + 0x400;
            half = -((rsin(angle) * 0xC) >> 0xC);
            ext->prim->x0 = self->posX.i.hi + half;
            ext->prim->x1 = self->posX.i.hi - half;
            spreadX = (s16) half;
            {
                s32 half2 = (rcos(angle) * 0xC) >> 0xC;
                ext->prim->y0 = self->posY.i.hi + half2;
                ext->prim->y1 = self->posY.i.hi - half2;
                ext->prim->x2 = (self + 1)->posX.i.hi + spreadX;
                ext->prim->x3 = (self + 1)->posX.i.hi - spreadX;
                ext->prim->y2 = (self + 1)->posY.i.hi + half2;
                ext->prim->y3 = (self + 1)->posY.i.hi - half2;
            }
            return;

        case 4:
            self->scaleX = 0x100;
            self->drawFlags |= 1;
            self->step++;
            // fallthrough
        case 5:
            target = ((ET_801A2774*) &self->ext)->target;
            self->rotate = target->rotate;
            angle = target->rotate + 0x400;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            self->posX.val += (ext->radius >> 0xC) * rcos(angle);
            {
                s32 lo = (ext->radius >> 0xC) * rsin(angle);
                self->scaleX -= 4;
                self->posY.val += lo;
                if (self->scaleX == 0) {
                    D_us_80180CDC = 0;
                    DestroyEntity(self);
                    return;
                }
            }
            angle = self->rotate + 0x400;
            half = -((rsin(angle) * 0xC) >> 0xC);
            {
                s16 s = self->scaleX;
                half = (half * s) >> 8;
                ext->prim->x0 = self->posX.i.hi + half;
                ext->prim->x1 = self->posX.i.hi - half;
                spreadX = (s16) half;
                {
                    s32 half2 = ((rcos(angle) * 0xC) >> 0xC) * s;
                    half2 >>= 8;
                    ext->prim->y0 = self->posY.i.hi + half2;
                    ext->prim->y1 = self->posY.i.hi - half2;
                    ext->prim->x2 = (self + 1)->posX.i.hi + spreadX;
                    ext->prim->x3 = (self + 1)->posX.i.hi - spreadX;
                    ext->prim->y2 = (self + 1)->posY.i.hi + half2;
                    ext->prim->y3 = (self + 1)->posY.i.hi - half2;
                }
            }
            return;

        case 16:
            target = ((ET_801A2774*) &self->ext)->target;
            {
                Entity* beforeTarget = target - 1;
                self->rotate = beforeTarget->rotate;
                angle = beforeTarget->rotate + 0x400;
                self->posX.i.hi = beforeTarget->posX.i.hi;
                self->posY.i.hi = beforeTarget->posY.i.hi;
            }
            self->posX.i.hi += (rcos(angle) * 0x24) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x24) >> 0xC;
            self->drawFlags = target->drawFlags;
            self->scaleX = target->scaleX;
            if (target->entityId == 0x1B) {
                ext->counter++;
                if (self->scaleX == 0 && !(ext->counter & 3)) {
                    spawned = AllocEntity(g_Entities_128, (Entity*) ((u8*) g_Entities_128 + 0x2F00));
                    if (spawned != NULL) {
                        CreateEntityFromEntity(0x23, self, spawned);
                        spawned->rotate = self->rotate + 0x400;
                        return;
                    }
                }
            } else {
                DestroyEntity(self);
            }
            break;

        case 32:
            target = ((ET_801A2774*) &self->ext)->target;
            self->rotate = target->rotate;
            angle = target->rotate + 0x400;
            self->posX.i.hi = target->posX.i.hi;
            self->posY.i.hi = target->posY.i.hi;
            self->posX.i.hi += (rcos(angle) * 0x18) >> 0xC;
            self->posY.i.hi += (rsin(angle) * 0x18) >> 0xC;

            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180E20, self) == 0) {
                        ext->counter = 0x40;
                        self->scaleX = 0x100;
                        self->drawFlags |= 1;
                        self->step_s++;
                        return;
                    }
                    break;
                case 1:
                    self->animCurFrame = (g_Timer & 1) ? 0x60 : 0;
                    self->scaleX = ((ext->counter & 7) * 3) + 0x100;
                    if (--ext->counter == 0) {
                        DestroyEntity(self);
                    }
                    break;
            }
            break;

        default:
            break;
    }
}

/*
 * Falling spark that impacts and leaves a trail of splash entities (type
 * 0x22): step 0-1 fly/fall under gravity; once D_us_80180CE4 bit 0x100 gets
 * set on self (attack ending), it "lands" (zeroes velocity/hitboxState,
 * plays impact SFX, advances to step 2); step 2 spawns a splash entity every
 * 8 frames via an ext+4 countdown, then destroys itself when the countdown
 * (and the shared off-screen check) run out. Confidence: high on control
 * flow/fields; the shared off-screen despawn check (posY.i.hi +
 * g_Tilemap.scrollY.i.hi >= 0x1C9) matches func_us_801A2D90/493C exactly.
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 counter; // splash-spawn countdown (steps 1 and 2)
} ET_801A4394;

void func_us_801A4394(Entity* self) {
    Entity* splash;
    s16 angle;
    s32 dir;

    if (D_us_80180CE4 & 1) {
        self->flags |= 0x100; // TODO: unnamed bit, same as func_us_801A2774/45D0
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B58);
            if (self->params != 8) {
                angle = D_us_80180D00[self->params] - 0x80;
                angle += Random();
                self->velocityX = (rcos(angle) << 0xF) >> 0xC;
                self->velocityY = (rsin(angle) << 0xF) >> 0xC;
            }
            dir = Random() & 3;
            if (dir == 3) {
                dir = 0;
            }
            self->facingLeft = Random() & 1;
            self->drawFlags = 4;
            self->animCurFrame = (dir * 2) + 0x73;
            // fallthrough
        case 1:
            MoveEntity();
            self->velocityY += 0x1000; // gravity
            self->rotate += 0x18;
            if (self->flags & 0x100) {
                self->velocityY = 0;
                self->hitboxState = 0;
                ((ET_801A4394*) &self->ext)->counter = 0x40;
                self->animCurFrame++;
                PlaySfxPositional(0x691);
                self->step++;
            }
            // fallthrough
        default:
            if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x1C9) {
                DestroyEntity(self);
            }
            break;

        case 2:
            MoveEntity();
            self->velocityY += 0x800;
            if (!(((ET_801A4394*) &self->ext)->counter & 7)) {
                splash = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                if (splash != NULL) {
                    CreateEntityFromEntity(0x22, self, splash);
                    splash->params = 0;
                    splash->zPriority = self->zPriority - 1;
                    splash->posY.i.hi += 0x14;
                }
            }
            ((ET_801A4394*) &self->ext)->counter--;
            if (((ET_801A4394*) &self->ext)->counter != 0) {
                if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) >= 0x1C9) {
                    DestroyEntity(self);
                }
            } else {
                DestroyEntity(self);
            }
            break;
    }
}

/*
 * "Creeping ground attack": crawls toward the player then plants and lobs
 * periodic projectiles. step 0-1: fall to ground (same CheckCollision
 * pattern as func_us_801A1334's landing, including the same unresolved
 * "correction" field read from the collision result -- see TODO below,
 * which is now backed by concrete evidence: m2c reported an unresolved
 * stack slot (sp28) added directly to posY.i.hi, at a fixed offset from the
 * collision result buffer). step 2 (2 sub-states): faces the player, then
 * crawls (velocityX = +-0x6000) until UnkCollisionFunc2 reports a hit
 * (bit 0x80). step 4 (3 sub-states, entered directly via SetStep once
 * D_us_80180CE4 bit 0x100 is set): plants, lobs a projectile (type 0x22)
 * downward-ish every 16 frames for a while, then every 8 frames going the
 * other direction, then self-destructs.
 *
 * Confidence: medium-high on control flow and named fields; the collision
 * result buffer layout (index 6 as a Y-correction) is inferred from the
 * stack offset m2c couldn't resolve, not independently confirmed.
 */
typedef struct {
    /* 0x80 (ext+4) */ u16 counter;
} ET_801A45D0;

void func_us_801A45D0(Entity* self) {
    Collider collider; // .effects & 1 = collided; .unk18 ("top edge of queried
                        // tile collision" per include/game.h) used below as a
                        // ground-height correction -- inferred from an
                        // m2c-unresolved stack slot at exactly that offset
                        // from the Collider buffer, not independently
                        // confirmed.
    Entity* child;

    if (D_us_80180CE4 & 1) {
        self->flags |= 0x100; // TODO: unnamed bit, same as func_us_801A2774/4394
    }
    if ((self->flags & 0x100) && self->step < 4) {
        SetStep(4);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B58);
            self->animCurFrame = 0x69;
            self->hitboxWidth = 8;
            self->hitboxHeight = 0x12;
            self->facingLeft = Random() & 1;
            // fallthrough
        case 1:
            MoveEntity();
            self->velocityY += 0x1800;
            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 0x12, &collider, 0);
            if (collider.effects & 1) {
                self->posY.i.hi += collider.unk18;
                self->step++;
            }
            return;

        case 2:
            switch (self->step_s) {
                case 0:
                    self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
                    self->step_s++;
                    // fallthrough
                case 1:
                    self->velocityX = self->facingLeft ? -0x6000 : 0x6000;
                    AnimateEntity(D_us_80180DE0, self);
                    if (UnkCollisionFunc2(&D_us_80180CE8) & 0x80) {
                        DestroyEntity(self);
                    }
                    break;
            }
            break;

        case 4:
            switch (self->step_s) {
                case 0:
                    self->hitboxState = 0;
                    ((ET_801A45D0*) &self->ext)->counter = (Random() & 0xF) + 0x10;
                    self->animCurFrame = 0x71;
                    PlaySfxPositional(0x691);
                    self->step_s++;
                    // fallthrough
                case 1:
                    if (!(((ET_801A45D0*) &self->ext)->counter & 0xF)) {
                        child = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (child != NULL) {
                            CreateEntityFromEntity(0x22, self, child);
                            child->posY.i.hi += 0x18;
                            child->params = 1;
                            child->zPriority = self->zPriority + 1;
                        }
                    }
                    ((ET_801A45D0*) &self->ext)->counter--;
                    if (((ET_801A45D0*) &self->ext)->counter == 0) {
                        ((ET_801A45D0*) &self->ext)->counter = 0x10;
                        self->animCurFrame = 0x72;
                        self->step_s++;
                        return;
                    }
                    break;

                case 2:
                    if (!(((ET_801A45D0*) &self->ext)->counter & 7)) {
                        child = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (child != NULL) {
                            CreateEntityFromEntity(0x22, self, child);
                            child->posY.i.hi += 0x18;
                            child->zPriority = self->zPriority - 1;
                        }
                    }
                    ((ET_801A45D0*) &self->ext)->counter--;
                    if (((ET_801A45D0*) &self->ext)->counter == 0) {
                        DestroyEntity(self);
                    }
                    break;
            }
            break;
    }
}

/*
 * Ground-collision projectile: flies in a fixed direction (self->rotate at
 * spawn time), and the first time it touches the ground (checked against
 * its RAW local position, ext+0x24 latch), spawns a splash entity (type
 * 0x22) with jittered position. Once splashed, the off-screen check
 * switches to using SCREEN-space coordinates (local + scroll) instead of
 * raw local ones -- this asymmetry is preserved exactly as m2c reported it.
 * Confidence: high on control flow; the two-different-coordinate-systems
 * split is unusual but directly matches the raw decompilation, not a guess.
 */
typedef struct {
    /* 0xA0 (ext+0x24) */ u32 hasSplashed;
} ET_801A493C;

void func_us_801A493C(Entity* self) {
    Collider collider;
    s16 screenX;
    s16 screenY;
    Entity* splash;

    if (D_us_80180CE4 & 1) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180B34);
            self->hitboxWidth = 8;
            self->hitboxHeight = 8;
            self->velocityX = rcos(self->rotate) * 0x60;
            self->velocityY = rsin(self->rotate) * 0x60;
            // fallthrough
        case 1:
            MoveEntity();
            screenX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
            screenY = self->posY.i.hi + g_Tilemap.scrollY.i.hi;

            if (!((ET_801A493C*) &self->ext)->hasSplashed) {
                screenX = self->posX.i.hi;
                screenY = self->posY.i.hi;
                g_api_CheckCollision(screenX, screenY, &collider, 0);
                if (collider.effects & 1) {
                    ((ET_801A493C*) &self->ext)->hasSplashed = 1;
                    splash = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                    if (splash != NULL) {
                        CreateEntityFromCurrentEntity(0x22, splash);
                        splash->posX.i.hi += Random() & 0xF;
                        splash->zPriority = 0x70;
                        splash->params = 2;
                        splash->posY.i.hi += Random() & 0xF;
                    }
                }
            }

            if (screenX < 0 || screenY < 0 || screenX >= 0x201 || screenY >= 0x201) {
                DestroyEntity(self);
                return;
            }
            break;
    }
}

/*
 * Granfaloon's literal chain/rope: a 13-Primitive ribbon rendered along a
 * 14-point position-history trail. The trail buffer lives at ext+8 (0x84)
 * and deliberately spills 4 bytes past the end of the declared `Ext ext`
 * member into `unkB8` (0x84 + 14*4 == 0xBC == sizeof(Entity)) -- accessed
 * here via raw pointer arithmetic from `self`, not as a nested struct
 * member, to avoid relying on undefined behavior through the Ext type
 * itself. self->ext+0 holds the Primitive* (same convention as
 * func_us_801A10A8/38EC).
 *
 * step 0: spawns a chain of 13 primitives via g_api_AllocPrimitives(4, 13),
 * initializes a glow-gradient color ramp across them (dark to bright to
 * dark again), seeds the entire trail buffer to the spawn position, and
 * launches at self->rotate. Falls straight through into the step-1 homing
 * logic on the same frame.
 * step 1: homes toward the player (LimitAngleChange easing the aim angle,
 * capped at 0x80/frame) for 12 frames (ext+4 counter), then raises zPriority
 * and advances to step 2.
 * step 2: flies straight (no more homing), destroys itself once far enough
 * off screen.
 * any other step: skips both the homing and off-screen-check logic.
 * ALL steps (including this frame's transition) fall into the shared tail:
 * shifts the entire trail buffer back by one slot, offsetting by the
 * per-frame scroll delta (g_ScrollDeltaX/Y -- not the total scroll, unlike
 * every other position calc in this file, which is why a dedicated pair of
 * globals is used here), writes the current position into trail[0], then
 * walks the trail rendering each consecutive pair of points as a
 * perpendicular-offset quad joint, forming the rope ribbon. The final
 * segment is a special-cased "pinch to a point" cap using trail[12] alone
 * (preserved exactly as decompiled, including that it does NOT touch that
 * primitive's x2/y2 corners -- likely relying on those retaining a prior
 * frame's value to create a tapering visual).
 *
 * Confidence: LOW-MEDIUM overall -- this is the most speculative function
 * in this batch. Control flow and Entity-field usage are faithful to the
 * decompilation, and the general "position-history ribbon" shape is solid,
 * but: the exact Primitive color/UV constants are kept verbatim without
 * independently verifying the visual result; several 3-4 argument calls to
 * rcos/rsin/ratan2 in the raw output were treated as m2c artifacts and
 * reduced to their established single/double-argument forms (consistent
 * with every other verified use of these functions in this file) rather
 * than guessed as genuine multi-arg overloads; and the final tail-cap's
 * asymmetric corner assignment is preserved literally rather than
 * rationalized. Not yet verified against a real compile/asm-differ pass.
 */
typedef struct { s16 x; s16 y; } ChainTrailPoint;

typedef struct {
    /* 0x7C */ Primitive* prim;
    /* 0x80 */ u16 counter;
    /* 0x82 */ s16 angle;
} ET_801A4AF4;

void func_us_801A4AF4(Entity* self) {
    ChainTrailPoint* trail = (ChainTrailPoint*) ((u8*) self + 0x84); // 14 entries, 0x84..0xBC (last 4 bytes reuse unkB8)
    ET_801A4AF4* ext = (ET_801A4AF4*) &self->ext;
    Primitive* prim;
    s16 angle;
    s32 i;
    s16 x0, y0;

    if (D_us_80180CE4 & 1) {
        DestroyEntity(self);
        return;
    }

    if (self->step == 0) {
        s32 primIdx = g_api_AllocPrimitives(4, 0xD);

        InitializeEntity(D_us_80180B28);
        self->hitboxHeight = 4;
        self->hitboxWidth = 4;
        self->zPriority = 0x38;

        if (primIdx == -1) {
            DestroyEntity(self);
            return;
        }
        prim = &g_PrimBuf[primIdx];
        self->primIndex = primIdx;
        ext->prim = prim;
        self->flags |= FLAG_HAS_PRIMS;

        prim->tpage = 0x14;
        prim->clut = 0x200;
        prim->u2 = 0x38;
        prim->u0 = 0x38;
        prim->v1 = 0x28;
        prim->v0 = 0x28;
        prim->u3 = 0x40;
        prim->u1 = 0x40;
        prim->v3 = 0x40;
        prim->v2 = 0x40;
        prim->b0 = 0xE0;
        prim->g0 = 0xE0;
        prim->r0 = 0xE0;
        prim->priority = 0x38;
        prim->drawMode = 0x37;
        prim->r1 = prim->r0;
        prim->r2 = prim->r0;
        prim->r3 = prim->r0;

        {
            s8 fade = -0x40;
            for (i = 0; i < 0xB; i++) {
                prim->b0 = fade;
                prim->g0 = fade;
                prim->r0 = fade;
                prim->tpage = 0x14;
                prim->clut = 0x200;
                prim->u2 = 0x38;
                prim->u0 = 0x38;
                prim->u3 = 0x40;
                prim->u1 = 0x40;
                prim->v1 = 0x50;
                prim->v0 = 0x50;
                prim->v3 = 0x58;
                prim->v2 = 0x58;
                prim->priority = 0x38;
                prim->drawMode = 0x37;
                prim->r1 = prim->r0;
                prim->r2 = prim->r0;
                prim->r3 = prim->r0;
                prim = prim->next;
                fade -= 0x10;
            }
        }

        prim->b0 = 0;
        prim->g0 = 0;
        prim->r0 = 0;
        prim->tpage = 0x14;
        prim->clut = 0x200;
        prim->u2 = 0x38;
        prim->u0 = 0x38;
        prim->u3 = 0x40;
        prim->u1 = 0x40;
        prim->v1 = 0x40;
        prim->v0 = 0x40;
        prim->v3 = 0x28;
        prim->v2 = 0x28;
        prim->priority = 0x38;
        prim->drawMode = 0x37;
        prim->r1 = prim->r0;
        prim->r2 = prim->r0;
        prim->r3 = prim->r0;

        for (i = 0; i < 0xE; i++) {
            trail[i].x = self->posX.i.hi;
            trail[i].y = self->posY.i.hi;
        }

        angle = self->rotate;
        self->velocityX = rcos(angle) << 7;
        self->velocityY = rsin(angle) << 7;
        ext->angle = angle;
        ext->counter = 0xC;
        goto homing_tail;
    } else if (self->step == 1) {
        goto homing_tail;
    } else if (self->step == 2) {
        MoveEntity();
        {
            s16 screenX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
            s16 screenY = self->posY.i.hi + g_Tilemap.scrollY.i.hi;
            if (screenX < -0x40 || screenY < -0x40 || screenX >= 0x241 || screenY >= 0x201) {
                DestroyEntity(self);
                return;
            }
        }
        goto trail_tail;
    } else {
        goto trail_tail;
    }

homing_tail:
    MoveEntity();
    angle = LimitAngleChange(0x80, ext->angle, GetAngleBetweenEntities(self, g_Entities));
    self->velocityX = rcos(angle) << 7;
    self->velocityY = rsin(angle) << 7;
    ext->angle = angle;
    if (--ext->counter == 0) {
        self->zPriority = 0x50;
        self->step++;
    }

trail_tail:
    for (i = 0; i < 0xD; i++) {
        trail[i].x = trail[i + 1].x - g_ScrollDeltaX;
        trail[i].y = trail[i + 1].y - g_ScrollDeltaY;
    }

    trail[0].x = self->posX.i.hi;
    trail[0].y = self->posY.i.hi;

    x0 = trail[0].x;
    y0 = trail[0].y;
    prim = ext->prim;
    {
        s16 headAngle = ratan2(trail[1].y - y0, trail[1].x - x0) - 0x400;
        s32 dx = (rcos(headAngle) * 2) >> 0xC;
        s32 dy = (rsin(headAngle) * 2) >> 0xC;
        prim->x0 = x0 + dx;
        prim->y0 = y0 + dy;
        prim->x1 = x0 - dx;
        prim->y1 = y0 - dy;
    }
    prim->priority = self->zPriority;

    for (i = 0; i < 0xB; i++) {
        s16 x1 = trail[i + 1].x;
        s16 y1 = trail[i + 1].y;
        s16 segAngle = ratan2(y1 - y0, x1 - x0) - 0x400;
        s32 sumX = x1 + x0;
        s32 sumY = y1 + y0;
        s32 midX = (sumX + (sumX >> 31)) >> 1;
        s32 midY = (sumY + (sumY >> 31)) >> 1;
        s32 sdx = (rcos(segAngle) * 4) >> 0xC;
        s32 sdy = (rsin(segAngle) * 4) >> 0xC;
        s16 px0 = (s16) (midX + sdx);
        s16 py0 = (s16) (midY + sdy);
        s16 px1 = (s16) (midX - sdx);
        s16 py1 = (s16) (midY - sdy);

        prim->x2 = px0;
        prim->y2 = py0;
        prim->x3 = px1;
        prim->y3 = py1;
        prim->priority = self->zPriority;

        prim = prim->next;
        prim->x0 = px0;
        prim->y0 = py0;
        prim->x1 = px1;
        prim->y1 = py1;

        x0 = x1;
        y0 = y1;
    }

    /* Tail-cap: pinch the last primitive's trailing edge to trail[12]
     * (note: overwrites x1/y1 -- which the loop above just set -- and does
     * NOT set x2/y2, preserved exactly as decompiled). */
    prim->x3 = trail[12].x;
    prim->x1 = trail[12].x;
    prim->y3 = trail[12].y;
    prim->y1 = trail[12].y;
    prim->priority = self->zPriority;
}

/*
 * Spawns 6 "type 0x22" child entities around g_CurrentEntity, writing 3
 * boss-specific ext fields on each (offsets 0x84/0x88/0x89, i.e. ext+8/
 * ext+0xC/ext+0xD) that aren't consumed by any function decompiled in this
 * file -- presumably read by entity type 0x22's own update function
 * (not yet decompiled). Kept as raw offsets rather than a guessed struct.
 * Confidence: high on control flow/spawn loop, low on the 3 written fields'
 * real meaning.
 */
void func_us_801A50BC(void) {
    s8 dir;
    s16 spread;
    s32 i;
    Entity* child;

    dir = Random() & 3;
    spread = ((Random() & 0xF) << 8) - 0x800;

    for (i = 0; i < 6; i++) {
        child = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
        if (child != NULL) {
            CreateEntityFromEntity(0x22, g_CurrentEntity, child);
            *((s8*) child + 0x89) = 6 - i;              // ext+0xD, TODO: unnamed
            *((s8*) child + 0x88) = dir;                 // ext+0xC, TODO: unnamed
            child->params = 2;
            *((s16*) ((u8*) child + 0x84)) = spread;     // ext+8, TODO: unnamed
            child->zPriority = g_CurrentEntity->zPriority + 1;
        }
    }
}

/*
 * Shared "debris/spark attack effect" library: step 0 picks one of 7
 * behaviors from D_us_80180FCC (indexed by self->params & 0xF) and jumps
 * straight to step (index+1) -- so self->step 1-7 each correspond to one
 * pattern, not a sequential counter. self->params upper byte (>> 8), if
 * nonzero, overrides zPriority; self->params & 0xF0, if nonzero, overrides
 * with a fixed "glitch-like" palette/blendMode/facingLeft combo and skips
 * pattern selection entirely for this frame.
 *
 * step 1/2: launch at a fixed speed (0x10) along rotate-0x400 (or, for
 * step 2, the same but entered via a slightly different gate order),
 * then move every frame and destroy once the pattern's AnimateEntity call
 * reports done.
 * step 3: stationary -- just plays its animation and destroys when done
 * (also the shared tail for steps 1/2 once launched).
 * step 4: constant downward drift (velocityY = -0x4000).
 * step 5: launches along the CURRENT rotate (not rotate-0x400), then
 * visually offsets rotate by -0x400 afterward.
 * step 6: random initial angle spiral debris -- grows (scaleX += 6/frame),
 * fades (opacity -= 1/frame), and grows vertically (scaleY += 2/frame).
 * step 7: random horizontal launch with a randomized "gravity" scratch
 * value (ext+4 -- reused here as an s32 accumulator, distinct from its
 * u8*-animTable use in step 0-6, same "same offset means different things
 * in different call paths" pattern seen throughout this file... except
 * here it's the SAME function reusing the field for two purposes across
 * different steps); fades opacity, accumulates velocityY via that gravity
 * value, and once self->pose (a real, already-named but generic Entity
 * field) reaches 8, spawns a type-0x22 puff copying its own scaleX.
 *
 * Confidence: high on control flow and the D_us_80180FCC table (values
 * recovered directly from m2c's rodata reconstruction, not guessed).
 * Medium on the ext+4 field's dual role (u8* animTable vs s32 gravity
 * scratch depending on which step path is active). Not yet verified
 * against a real compile.
 */
typedef struct {
    /* 0x80 (ext+4) */ u8* animTable; // steps 0-6; step 7 reuses this same slot as an s32 "gravity" scratch instead
} ET_801A518C;

void func_us_801A518C(Entity* self) {
    ET_801A518C* ext = (ET_801A518C*) &self->ext;
    s16 angle;
    s32 index;
    Entity* puff;
    const D_us_80180FCC_Entry* entry;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180AA4);
            index = self->params & 0xF;
            entry = &D_us_80180FCC[index];
            self->palette = entry->paletteOffset + 0x2E0;
            self->blendMode = entry->blendMode;
            self->animSet = entry->animSet;
            self->unk5A = entry->unk5A;
            self->step = index + 1;
            ext->animTable = entry->animTable;
            if (self->params & 0xFF00) {
                self->zPriority = (self->params & 0xFF00) >> 8;
            }
            if (self->params & 0xF0) {
                self->palette = 0x819F;
                self->blendMode = 0x10;
                self->facingLeft = 1;
                return;
            }
            return;

        case 1:
            if (self->step_s == 0) {
                angle = self->rotate - 0x400;
                self->velocityX = rcos(angle) * 0x10;
                self->velocityY = rsin(angle) * 0x10;
                self->step_s++;
                goto move_and_animate;
            }
            if (self->step_s != 1) {
                return;
            }
            goto move_and_animate;

        case 3:
        anim_check_only:
            if (AnimateEntity(ext->animTable, self) == 0) {
                DestroyEntity(self);
            }
            break;

        case 2:
            if (self->step_s != 0) {
                if (self->step_s != 1) {
                    return;
                }
                goto move_and_animate;
            }
            angle = self->rotate - 0x400;
            self->velocityX = rcos(angle) * 0x10;
            self->velocityY = rsin(angle) * 0x10;
            self->step_s++;
            goto move_and_animate;

        case 4:
            if (self->step_s == 0) {
                self->velocityY = -0x4000;
                self->step_s++;
            }
            goto move_and_animate;

        case 5:
            if (self->step_s == 0) {
                angle = self->rotate;
                self->velocityX = rcos(angle) * 0x10;
                self->velocityY = rsin(angle) * 0x10;
                self->rotate -= 0x400;
                self->step_s++;
            }
            goto move_and_animate;

        case 6:
            if (self->step_s == 0) {
                angle = (self->rotate + 0x100) - Random() * 2;
                self->rotate = angle;
                self->velocityX = rcos(angle) * 0x1C;
                self->velocityY = rsin(angle) * 0x1C;
                self->blendMode = 0x30;
                self->drawFlags = 0xF;
                self->scaleX = 0x20;
                self->scaleY = 0xC0;
                self->opacity = 0x80;
                self->rotate += 0x400;
                self->step_s++;
            }
            MoveEntity();
            self->scaleX += 6;
            self->opacity--;
            self->scaleY += 2;
            goto anim_check_only;

        case 7:
            if (self->step_s == 0) {
                self->drawFlags = 9;
                self->opacity = 0x80;
                self->facingLeft = Random() & 1;
                self->velocityX = (Random() << 8) - 0x8000;
                self->velocityY = -0x28000;
                *((s32*) &ext->animTable) = -(Random() * 0x10) - 0x1000; // ext+4 reused as s32 gravity scratch (see doc comment)
                self->step_s++;
            }
            MoveEntity();
            self->opacity--;
            self->velocityY += *((s32*) &ext->animTable);
            if (self->pose == 8) {
                puff = AllocEntity(g_Entities_64, (Entity*) &g_EvHwCardEnd);
                if (puff != NULL) {
                    CreateEntityFromEntity(0x22, self, puff);
                    puff->params = 6;
                    puff->scaleX = self->scaleX;
                }
            }
            goto anim_check_only;

        default:
            return;
    }
    return;

move_and_animate:
    MoveEntity();
    goto anim_check_only;
}
