#include "stage.h"

typedef struct {
    u8 pad0[8];      // ext+0x0-0x7 (unused by this function)
    s32 swingPos;    // ext+0x8
    s32 swingSpeed;  // ext+0xC
} ET_801AAFCC;


// Per-frame idle motion for a floating pickup: while velocityY is non-negative,
// accumulates a horizontal "swing" value (ext+0xC) into ext+8 and mirrors it
// into velocityX (drives a left-right sway in the renderer), flipping the
// swing direction once it reaches +-1.0 in 16.16 fixed point (0x10000). Then
// unconditionally ramps velocityY up toward 0x4000 by 0x2000/frame -- a short
// ease-in, not real gravity (capped well before any real falling speed).
// Confidence: high on Entity fields (velocityX/velocityY confirmed); the two
// ext fields are local to this entity type and not cross-checked against
// another function, so their names are inferred from behavior only.
void func_us_801AAFCC(void)
{
    ET_801AAFCC* ext = (ET_801AAFCC*)&g_CurrentEntity->ext;
    s32 newSwing;

    if (g_CurrentEntity->velocityY >= 0) {
        newSwing = ext->swingSpeed + ext->swingPos;
        ext->swingPos = newSwing;
        g_CurrentEntity->velocityX = newSwing;
        if ((newSwing == 0x10000) || (newSwing == 0xFFFF0000)) {
            ext->swingSpeed = -ext->swingSpeed;
        }
    }
    if (g_CurrentEntity->velocityY < 0x4000) {
        g_CurrentEntity->velocityY += 0x2000;
    }
}

extern void FallEntity(void);
extern void MoveEntity(void);
extern void func_us_801AAFCC(void); // idle bob/sway, decompiled earlier this pass

// Per-frame movement + tile-collision resolver for a platform-like entity.
// Probes tile collision a fixed 7px above and below the entity's Y position;
// if solid on both checks (bits 0/2, i.e. "&5"), stops horizontal and
// vertical motion, snaps posY to the tile edge (or nudges down slightly if
// the collision was a ceiling hit, bit 2). Otherwise keeps falling/moving
// normally, or -- when arg0 is 0 -- calls the idle sway update instead of
// falling once movement stops.
// Confidence: high on CheckCollision/Collider (confirmed against the real
// signature in src/dra/collider.c and the already-annotated Collider.unk18
// "top edge of queried tile collision" field, same convention used in bo1);
// medium on the overall intent (which axis is "moving" isn't fully clear
// from this function alone, since it reads posY-based collision but branches
// on velocityX).
void func_us_801AB050(s32 arg0)
{
    Collider col;

    if (g_CurrentEntity->velocityX < 0) {
        g_api_CheckCollision(g_CurrentEntity->posX.i.hi, g_CurrentEntity->posY.i.hi - 7, &col, 0);
        if (col.effects & 5) {
            g_CurrentEntity->velocityY = 0;
        }
    }
    g_api_CheckCollision(g_CurrentEntity->posX.i.hi, g_CurrentEntity->posY.i.hi + 7, &col, 0);
    if (arg0 & 0xFFFF) {
        if (!(col.effects & 5)) {
            MoveEntity();
            FallEntity();
            return;
        }
        g_CurrentEntity->velocityX = 0;
        g_CurrentEntity->velocityY = 0;
        if (col.effects & 4) {
            g_CurrentEntity->posY.val += 0x2000;
            return;
        }
        g_CurrentEntity->posY.i.hi = (u16)g_CurrentEntity->posY.i.hi + (u16)col.unk18;
        return;
    }
    if (!(col.effects & 5)) {
        MoveEntity();
        func_us_801AAFCC();
    }
}

extern void (*g_api_PlaySfx)(s32 sfxId);
extern s8 D_us_80181928[]; // per-arg0 heart amount table (small values expected)

// Variable-amount heart pickup: amount looked up from a table by arg0 (likely
// an entity spawn param), added to g_Status.hearts, capped at heartsMax. Same
// idiom as the flat HEART_VESSEL_RICHTER pickup in e_collect.h, but with a
// per-instance amount instead of a fixed constant.
// Confidence: high on control flow/fields (PlayerStatus.hearts/heartsMax
// offsets confirmed against include/game.h). Medium on D_us_80181928's exact
// element type/size (guessed s8, indexed directly by arg0). The extra
// DestroyEntity argument seen in the raw m2c output (&g_Status + 0x244) is a
// stale-register artifact of the class documented repeatedly in bo1 --
// dropped, matching the real single-argument DestroyEntity signature.
void func_us_801AB1AC(s32 arg0)
{
    s32 newHearts;

    g_api_PlaySfx(0x67A);
    newHearts = D_us_80181928[arg0 & 0xFFFF] + g_Status.hearts;
    g_Status.hearts = newHearts;
    if (g_Status.heartsMax < newHearts) {
        g_Status.hearts = g_Status.heartsMax;
    }
    DestroyEntity(g_CurrentEntity);
}

extern void BottomCornerText(u8* str, u8 leftAlign);
extern u8* D_us_80181870[]; // per-arg0 message string table
extern s32 D_us_80181898[]; // per-arg0 gold amount table
extern void (*g_api_FreePrimitives)(s32 primIndex);
extern void (*g_api_PlaySfx)(s32 sfxId);
extern unkGraphicsStruct g_unkGraphicsStruct;

// Gold pickup: amount looked up by (arg0-2) from a table, added to
// g_Status.gold (capped at 999999 == 0xF423F), frees a lingering primitive
// chain if one is active, shows a corner-text message (also looked up by the
// same index) and destroys self. Mirrors the shared heart/life vessel pickups
// above but for gold, with a per-instance message + amount.
// Confidence: high on control flow and PlayerStatus.gold (offset confirmed
// against include/game.h); g_unkGraphicsStruct.BottomCornerTextTimer/
// BottomCornerTextPrims (offsets 0x18/0x1C, tracking a lingering corner-text
// primitive chain) match this use exactly. The second FreePrimitives
// argument seen in the raw m2c output (0xF423F) is a stale-register artifact
// of the same class documented repeatedly in bo1 -- dropped, matching the
// real single-argument FreePrimitives signature.
void func_us_801AB22C(s32 arg0)
{
    s32 idx;
    s32 newGold;

    g_api_PlaySfx(0x6A9);
    idx = (arg0 - 2) & 0xFFFF;
    newGold = g_Status.gold + D_us_80181898[idx];
    g_Status.gold = newGold;
    if (newGold > 0xF423F) {
        g_Status.gold = 0xF423F;
    }
    if (g_unkGraphicsStruct.BottomCornerTextTimer != 0) {
        g_api_FreePrimitives(g_unkGraphicsStruct.BottomCornerTextPrims);
        g_unkGraphicsStruct.BottomCornerTextTimer = 0;
    }
    BottomCornerText(D_us_80181870[idx], 1);
    DestroyEntity(g_CurrentEntity);
}

typedef struct {
    u8 pad0[0xE]; // ext+0x0-0xD (unused by this function)
    u8 tossTimer; // ext+0xE
} ET_801AB308;

extern void SetStep(u8 step);
extern u16 D_us_8018182C[]; // per-arg0 -> subWeapon id table
extern u16 D_us_8018185C[]; // fallback subWeapon id table (indexed by current subWeapon)
extern Entity g_Entities[];
extern void (*g_api_PlaySfx)(s32 sfxId);

// Subweapon pickup. Looks up the subweapon id granted by this pickup
// (D_us_8018182C, indexed by arg0); if the player already has that exact
// subweapon, gives a small "already have it" bounce animation (byte 0x10 at
// ext+0x-adjacent unk6D padding) instead of a pickup, otherwise swaps to
// whichever subweapon D_us_8018185C names as the upgrade/replacement for the
// current one, with a bigger bounce (0x60). On an actual pickup, positions
// self relative to g_Entities[0] (the room's reference/anchor entity) and
// kicks off a short upward-toss animation via SetStep(7); the
// g_Entities[0].facingLeft check picks the toss direction.
// Confidence: high on PlayerStatus (subWeapon offset 0x298 confirmed against
// include/game.h) and Entity fields; medium on the exact subWeapon-swap
// semantics of the two tables (inferred from the equality check, not
// cross-checked against the shared subweapon-pickup logic in
// src/st/collect_subweapon.h, which uses a different table shape). Low on
// ext+0xE's exact meaning (named "tossTimer" from behavior only).
void func_us_801AB308(s32 arg0)
{
    u16 grantedId;
    s16 replacementId;
    u8 bounce;
    ET_801AB308* ext = (ET_801AB308*)&g_CurrentEntity->ext;

    g_api_PlaySfx(0x67C);
    grantedId = D_us_8018182C[arg0 & 0xFFFF];
    g_Status.subWeapon = grantedId;
    if ((s32)(g_Status.subWeapon & 0xFFFF) == grantedId) {
        replacementId = 1;
        bounce = 0x10;
    } else {
        replacementId = D_us_8018185C[g_Status.subWeapon & 0xFFFF];
        bounce = 0x60;
    }
    g_CurrentEntity->unk6D[0] = bounce;
    if (replacementId != 0) {
        g_CurrentEntity->params = replacementId;
        g_CurrentEntity->posY.i.hi = (s16)(g_Entities[0].posY.i.hi + 0xC);
        SetStep(7);
        g_CurrentEntity->velocityY = 0xFFFD8000;
        g_CurrentEntity->animCurFrame = 0;
        ext->tossTimer = 5;
        if (g_Entities[0].facingLeft != 1) {
            g_CurrentEntity->velocityX = 0xFFFE0000;
            return;
        }
        g_CurrentEntity->velocityX = 0x20000;
        return;
    }
    DestroyEntity(g_CurrentEntity);
}

extern s32 g_PlayableCharacter;
extern void (*g_api_PlaySfx)(s32 sfxId);
extern s32 (*g_api_func_800FE044)(s32 amount, s32 type);

// Heart Vessel pickup -- matches the two-branch shape of e_collect.h's heart
// vessel handler exactly: non-Alucard characters get a flat HEART_VESSEL_RICHTER
// (30) added directly to hearts (capped at heartsMax); Alucard instead routes
// through func_800FE044(HEART_VESSEL_INCREASE, 0x4000), which raises heartsMax.
// Confidence: very high (confirmed against the already-decompiled shared
// version of this exact pickup).
void func_us_801AB420(void)
{
    s32 newHearts;

    if (g_PlayableCharacter != 0) {
        g_api_PlaySfx(0x67A);
        newHearts = g_Status.hearts + 0x1E;
        g_Status.hearts = newHearts;
        if (g_Status.heartsMax < newHearts) {
            g_Status.hearts = g_Status.heartsMax;
        }
    } else {
        g_api_PlaySfx(0x67A);
        g_api_func_800FE044(5, 0x4000);
    }
    DestroyEntity(g_CurrentEntity);
}

extern void (*g_api_PlaySfx)(s32 sfxId);
extern s32 (*g_api_func_800FE044)(s32 amount, s32 type);

// Life Vessel pickup. Matches `CollectLifeVessel()` in src/st/collect_life_vessel.h
// byte-for-byte (SFX_HEART_PICKUP == 0x67A, LIFE_VESSEL_INCREASE == 5, type
// 0x8000 == "increase hpMax" branch of func_800FE044). This is presumably a
// boss-room-local copy of the same pickup rather than a shared e_collect entity.
// Confidence: very high (confirmed against an already-decompiled twin).
void func_us_801AB4C4(void)
{
    g_api_PlaySfx(0x67A);
    g_api_func_800FE044(5, 0x8000);
    DestroyEntity(g_CurrentEntity);
}


// Generic "destroy self" callback, no reward. Confidence: very high (trivial,
// no ambiguous fields).
void func_us_801AB514(void)
{
    DestroyEntity(g_CurrentEntity);
}

typedef struct {
    u8 pad0[4];        // 0x7C-0x7F
    u8 cooldownTimer;  // ext+0x4 (0x80): bounce/land/expire countdown, same convention as func_us_801ABF50
    u8 pad1[3];        // 0x81-0x83
    u32 unk8;          // ext+0x8 (0x84): set to 0xFFFF0000 when itemId==0 in step 1; purpose unclear
    u16 unkC;          // ext+0xC (0x88): set to 0x800 alongside unk8; purpose unclear
    s16 approachTimer; // ext+0xE (0x8A): step_s==1 uses it as a simple "wait N frames" delay before
                        // following the entity; step_s==2 reuses it as a growing 1..0x10 open/close
                        // margin counter for the same prompt-icon quad. Never independently reset
                        // between the two uses in this function, so the exact intended relationship
                        // between them isn't fully resolved.
    u8 pad2[8];        // 0x8C-0x93
    u16 castleFlagId;  // ext+0x18 (0x94): 1-based persistence-flag id, 0 = none (same as func_us_801ABF50)
} ET_801AB53C;

extern u8* D_us_801818C0[]; // per-itemId anim-data-pointer table (word-strided, confirmed via lw+*4 scaling in the asm)
extern u16 D_us_801B34F0[];
extern u16 D_us_80181920[];
extern void CheckFieldCollision(u16* table, s32 arg1);
extern s32 AnimateEntity(u8* anim, Entity* self);
extern u16 D_us_80180540[]; // InitializeEntity params (also used by func_us_801ABF50/801AD3E0 below)

// Richter-mode companion to func_us_801ABF50 (same pickup-entity family, dispatched via
// jtbl_us_8019CB8C, 8 cases). Installed as this entity's pfnUpdate directly from
// func_us_801ABF50's step 0 when g_PlayableCharacter indicates Richter is active, and also
// called once immediately at install time. self->params holds the item id throughout (masked
// to 15 bits into a local at entry, same convention as func_us_801ABF50); the same unexplained
// raw self->params 0x8000 bit is checked in step 3 to skip the normal countdown.
//
// Every call (regardless of step): once step != 0, advances the sprite's animation frame via
// AnimateEntity, using a per-item anim data pointer from D_us_801818C0. Then forces step to 5
// if hitFlags is set while in steps 2-4 (same "touch to auto-collect" idiom as func_us_801ABF50).
// Then, always, resets palette to 0 unless unk6D[0] (a bounce/toss timer set by func_us_801AB308)
// is >= 0x18 and the game timer's blink bit and item id don't suppress it, in which case palette
// flashes to 0x815F.
//
// step 0: init frame. Clears blendMode, sets zPriority from a shared graphics-struct base minus
//   0x14. If the item id is below 0x18 (not a real equip-range id), clears params and zeroes the
//   local item id. Then, for item ids in a subweapon-adjacent range, checks whether this id
//   matches the player's current subweapon-upgrade target (D_us_8018185C, same idiom as
//   func_us_801AB308) and if so forces params=1. Finally sets hitboxWidth to 4 in the two cases
//   where the resulting selector is 0 or 2 -- the third possible value (1, the subweapon-match
//   case) leaves hitboxWidth untouched. The exact intent of this 3-way split isn't independently
//   confirmed beyond the mechanical transcription.
// step 1: probes collision at the entity's own position; destroys self if blocked. Either way
//   (destroyed or not -- DestroyEntity doesn't invalidate the struct until slot reuse, same idiom
//   documented in func_us_801ABF50), advances to step 2 and marks a g_CastleFlags+0x1B0
//   persistence bit if castleFlagId is set. If the item id is exactly 0, additionally pokes the
//   still-unexplained unk8/unkC ext fields.
// step 2: fall/land handling. For item id 0, uses a simplified path (land -> cooldownTimer=0x60,
//   no CheckFieldCollision call; airborne -> idle sway via func_us_801AAFCC instead of
//   FallEntity). For any other item id, mirrors func_us_801ABF50's step 2 exactly (cooldownTimer
//   0xF0, CheckFieldCollision call every frame).
// step 3: calls func_us_801AB050(itemId) every frame (note: passes itemId, not a constant 1, as
//   func_us_801ABF50 does). If raw params&0x8000 is set, returns without doing anything further.
//   Otherwise decrements cooldownTimer, and once it hits 0 resets it to 0x40 (itemId != 0) or
//   0x50 (itemId == 0) and advances to step 4.
// step 4: calls func_us_801AB050(itemId); decrements cooldownTimer, jumping to the destroy path
//   (shared with steps 6/7) once it hits 0; otherwise zeroes animCurFrame when the timer's bit 1
//   is set (a blink cue, mirrors func_us_801ABF50's step 4 blink but toggles frame instead of
//   drawMode).
// step 5: routes to the shared reward helpers purely by item-id range: 0-1 -> variable-amount
//   heart (func_us_801AB1AC), 2-0xB -> gold (func_us_801AB22C), 0xC -> heart vessel
//   (func_us_801AB420), 0xD -> no-op destroy (func_us_801AB514), 0xE-0x16 -> subweapon
//   (func_us_801AB308), 0x17 -> life vessel (func_us_801AB4C4), anything else -> plain
//   DestroyEntity. Unlike func_us_801ABF50, equip/accessory-range ids (>= 0x18, other than the
//   life-vessel id 0x17) have no dedicated handling here and simply fall through to destroy --
//   consistent with Richter not using Alucard's equipment system in this game.
// steps 6/7 (identical, shared code): a further sub-dispatch on self->step_s, building a small
//   "pickup prompt" quad Primitive over the entity: step_s 0 does one-time appearance setup
//   (colors, uv, tpage/clut, drawMode, a subweapon-match re-check identical to step 0's) and
//   allocates the Primitive; step_s 1 waits out approachTimer then repositions a tight 2px box
//   around the entity's position each frame (drawMode 0x37) before advancing to step_s 2; step_s
//   2 grows a margin from 1 to 0x10 over approachTimer frames (repositioning the box by +-margin)
//   and, once the margin phase ends, instead fades the box's RGB by -8/frame indefinitely; step_s
//   >= 3 does nothing. A final check compares castleFlagId... no, compares approachTimer itself
//   against 0x20 to free the primitive and advance to step 3/step_s 0 -- given approachTimer only
//   ever reaches 1-0x10 in the traced code paths, this specific comparison appears unreachable;
//   transcribed as-is rather than "corrected" without evidence of what feeds it.
//
// Confidence: medium-high on control flow and all Entity/Collider/Primitive fields (confirmed
// against include/game.h and cross-checked against func_us_801ABF50's near-identical step 2).
// Low on ext field semantics beyond cooldownTimer/castleFlagId (unk8/unkC/approachTimer are
// behavior-inferred only) and on the step 0 hitboxWidth 3-way split and the apparently-dead
// approachTimer==0x20 check called out above.
void func_us_801AB53C(Entity* self)
{
    ET_801AB53C* ext = (ET_801AB53C*) &self->ext;
    u16 itemId = self->params & 0x7FFF;

    if (self->step != 0) {
        AnimateEntity(D_us_801818C0[itemId], self);
    }
    if ((u16)(self->step - 2) < 3 && self->hitFlags != 0) {
        self->step = 5;
    }

    self->palette = 0;
    if (self->unk6D[0] >= 0x18 && !(g_GameTimer & 2) && self->params != 1) {
        self->palette = 0x815F;
    }

    if (self->step >= 8) {
        return;
    }

    switch (self->step) {
        case 0: {
            s32 sel;

            InitializeEntity(D_us_80180540);
            self->blendMode = 0;
            self->zPriority = (u16)((u16)g_unkGraphicsStruct.g_zEntityCenter - 0x14);

            if (itemId < 0x18) {
                self->params = 0;
                itemId = 0;
            }
            sel = itemId - 0xE;
            if ((u16) sel < 9) {
                if (itemId == D_us_8018185C[g_Status.subWeapon]) {
                    itemId = 1;
                    self->params = 1;
                    sel = 1;
                }
            }
            if (sel == 0 || sel == 2) {
                self->hitboxWidth = 4;
            }
            return;
        }

        case 1: {
            Collider col;

            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi, &col, 0);
            if (col.effects & 7) {
                DestroyEntity(self);
            } else {
                self->step++;
                if (ext->castleFlagId != 0) {
                    s32 bit = ext->castleFlagId - 1;
                    g_CastleFlags[0x1B0 + (bit >> 3)] |= (1 << (bit & 7));
                }
            }
            if (itemId == 0) {
                ext->unk8 = 0xFFFF0000;
                ext->unkC = 0x800;
            }
            return;
        }

        case 2: {
            Collider col;

            if (self->velocityY < 0) {
                g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi - 7, &col, 0);
                if (col.effects & 5) {
                    self->velocityY = 0;
                }
            }
            MoveEntity();
            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 7, &col, 0);
            if (itemId == 0) {
                if (col.effects & 5) {
                    ext->cooldownTimer = 0x60;
                    self->posY.i.hi = (s16)(self->posY.i.hi + col.unk18);
                    self->step++;
                } else {
                    func_us_801AAFCC();
                }
                return;
            }
            if (!(col.effects & 5) || self->velocityY <= 0) {
                FallEntity();
            } else {
                self->velocityX = 0;
                self->velocityY = 0;
                ext->cooldownTimer = 0xF0;
                self->posY.i.hi = (s16)(self->posY.i.hi + col.unk18);
                self->step++;
            }
            CheckFieldCollision(D_us_80181920, 2);
            return;
        }

        case 3:
            func_us_801AB050(itemId);
            if (self->params & 0x8000) {
                return;
            }
            if (--ext->cooldownTimer == 0) {
                ext->cooldownTimer = (itemId != 0) ? 0x40 : 0x50;
                self->step++;
            }
            return;

        case 4: {
            u8 t;

            func_us_801AB050(itemId);
            t = --ext->cooldownTimer;
            if (t == 0) {
                goto destroy;
            }
            if (t & 2) {
                self->animCurFrame = 0;
            }
            return;
        }

        case 5:
            if (itemId < 2) {
                func_us_801AB1AC(itemId);
            } else if (itemId < 0xC) {
                func_us_801AB22C(itemId);
            } else if (itemId == 0xC) {
                func_us_801AB420();
            } else if (itemId < 0xE) {
                func_us_801AB514();
            } else if (itemId < 0x17) {
                func_us_801AB308(itemId);
            } else if (itemId == 0x17) {
                func_us_801AB4C4();
            } else {
                goto destroy;
            }
            return;

        case 6:
        case 7:
            switch (self->step_s) {
                case 0: {
                    s32 primIdx;
                    Primitive* prim;

                    self->animCurFrame = 0;
                    if ((u16)(itemId - 0xE) < 9) {
                        if (itemId == D_us_8018185C[g_Status.subWeapon]) {
                            itemId = 1;
                            self->params = 1;
                        }
                    }
                    primIdx = g_api_AllocPrimitives(4, 1);
                    if (primIdx == -1) {
                        return;
                    }
                    self->flags |= 0x800000;
                    self->primIndex = primIdx;
                    prim = &g_PrimBuf[primIdx];
                    prim->tpage = 0x1A;
                    prim->clut = 0x170;
                    prim->v3 = prim->v1 = prim->u3 = prim->u1 = 0x20;
                    prim->b3 = prim->b2 = prim->b1 = prim->b0 = 0x80;
                    prim->g3 = prim->g2 = prim->g1 = prim->g0 = 0x80;
                    prim->r3 = prim->r2 = prim->r1 = prim->r0 = 0x80;
                    prim->v0 = prim->u0 = prim->u2 = 0;
                    prim->drawMode = 8;
                    prim->priority = (u16)(self->zPriority + 1);
                    /* self->step_s is 0 here; transcribed as-is even though it makes this a
                     * near-constant write -- see function-level note on unresolved fields. */
                    prim->y3 = (s16)(self->step_s + 1);
                    return;
                }

                case 1: {
                    Collider col;

                    MoveEntity();
                    g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 7, &col, 0);
                    if ((col.effects & 5) && self->velocityY > 0) {
                        self->velocityX = 0;
                        self->velocityY = 0;
                        self->posY.i.hi = (s16)(self->posY.i.hi + col.unk18);
                        self->step_s++;
                    } else {
                        FallEntity();
                    }
                    CheckFieldCollision(D_us_80181920, 2);
                    self->animCurFrame = 0;
                    if (ext->approachTimer != 0) {
                        ext->approachTimer--;
                        return;
                    }
                    {
                        Primitive* prim = &g_PrimBuf[self->primIndex];
                        s16 x0 = (s16)(self->posX.i.hi - 1);
                        s16 x1 = (s16)(self->posX.i.hi + 1);
                        s16 y0 = (s16)(self->posY.i.hi - 1);
                        s16 y1 = (s16)(self->posY.i.hi + 1);

                        prim->x2 = prim->x0 = x0;
                        prim->x3 = prim->x1 = x1;
                        prim->y1 = prim->y0 = y0;
                        prim->drawMode = 0x37;
                        prim->y3 = prim->y2 = y1;
                    }
                    return;
                }

                case 2: {
                    Primitive* prim = &g_PrimBuf[self->primIndex];
                    s32 margin;

                    func_us_801AB050(itemId);
                    if (ext->approachTimer + 1 < 0x11) {
                        ext->approachTimer++;
                        margin = ext->approachTimer;
                        self->animCurFrame = 0;
                    } else {
                        u8 r = (u8)(prim->r3 - 8);
                        u8 g = (u8)(prim->g3 - 8);
                        u8 b = (u8)(prim->b3 - 8);

                        margin = 0x20 - (ext->approachTimer + 1);
                        prim->r3 = prim->r2 = prim->r1 = prim->r0 = r;
                        prim->g3 = prim->g2 = prim->g1 = prim->g0 = g;
                        prim->b3 = prim->b2 = prim->b1 = prim->b0 = b;
                    }
                    prim->x2 = prim->x0 = (s16)(self->posX.i.hi - margin);
                    prim->x3 = prim->x1 = (s16)(self->posX.i.hi + margin);
                    prim->y1 = prim->y0 = (s16)(self->posY.i.hi - margin);
                    prim->y3 = prim->y2 = (s16)(self->posY.i.hi + margin);
                    if (ext->approachTimer == 0x20) {
                        g_api_FreePrimitives(self->primIndex);
                        self->flags &= ~0x800000;
                        ext->cooldownTimer = 0xD0;
                        self->step = 3;
                        self->step_s = 0;
                    }
                    return;
                }
            }
            return;
    }

destroy:
    DestroyEntity(self);
}

extern s32 AnimateEntity(u8* anim, Entity* self);
extern u16 D_us_8018054C[]; // InitializeEntity params (step 0, func_us_801ABDB0)
extern s32 D_us_8018192C[]; // per-params(&0xF) initial velocityY table
extern u8* D_us_801819D8[]; // per-params(&0xF) animation table

// Entity init + per-frame fall/animate. First frame: sets up sprite defaults,
// splits `params` into two halves -- the high byte (if set) overrides
// zPriority, and the low nibble selects both an initial fall speed
// (D_us_8018192C) and an animation (D_us_801819D8); a params high-nibble bit
// also forces an alternate palette. Every frame after: advances posY by the
// stored fall speed and destroys self once the animation finishes.
// Confidence: medium-high -- Entity fields and control flow are solid; the
// exact bit-packing of `params` (which nibble means what) is inferred from
// the masks alone, not cross-checked against a spawner.
void func_us_801ABDB0(Entity* self)
{
    u32 highByte;

    if (self->step == 0) {
        InitializeEntity(D_us_8018054C);
        self->animSet = 2;
        self->pose = 0;
        self->poseTimer = 0;
        self->blendMode = 0x30;
        if (self->params & 0xF0) {
            self->palette = 0x8195;
            self->blendMode = 0x10;
        }
        highByte = self->params & 0xFF00;
        if (highByte != 0) {
            self->zPriority = (s16)(highByte >> 8);
        }
        self->params = (u16)(self->params & 0xF);
        self->velocityY = D_us_8018192C[self->params];
        return;
    }
    self->posY.val += self->velocityY;
    if (AnimateEntity(D_us_801819D8[self->params], self) == 0) {
        DestroyEntity(self);
    }
}


// Draws a small flat-colored 7px-radius square Primitive (a hit-spark/flash
// marker) centered on the entity's screen position (posX.i.hi, posY.i.hi),
// using the entity's already-allocated prim slot (primIndex). All 4 vertices
// get the same flat color, toggling between full white (0xFF) and dim gray
// (0x80) based on bit 1 of arg1.
// Confidence: very high -- every field matches include/primitive.h's
// Primitive layout exactly (sizeof(Primitive) == 0x34 confirmed by the
// primIndex*0x34 stride).
void func_us_801ABEAC(Entity* self, s32 arg1)
{
    s16 cx, cy;
    s16 left, right, top, bottom;
    u8 brightness;
    Primitive* prim;

    cx = self->posX.i.hi;
    left = cx - 7;
    right = cx + 7;
    prim = &g_PrimBuf[self->primIndex];
    prim->x2 = left;
    prim->x0 = left;
    prim->x3 = right;
    prim->x1 = right;

    cy = self->posY.i.hi;
    top = cy - 7;
    bottom = cy + 7;
    prim->y1 = top;
    prim->y0 = top;
    prim->y3 = bottom;
    prim->y2 = bottom;

    brightness = (arg1 & 2) ? 0xFF : 0x80;
    prim->b3 = brightness;
    prim->b2 = brightness;
    prim->b1 = brightness;
    prim->b0 = brightness;
    prim->g3 = brightness;
    prim->g2 = brightness;
    prim->g1 = brightness;
    prim->g0 = brightness;
    prim->r3 = brightness;
    prim->r2 = brightness;
    prim->r1 = brightness;
    prim->r0 = brightness;
}

typedef struct {
    u16 sparkleArg;    // ext+0x0 (0x7C): countdown, also passed as func_us_801ABEAC's arg1
    u8 pad0[2];        // 0x7E-0x7F
    u8 cooldownTimer;  // ext+0x4 (0x80): bounce/blink/expire countdown, reused across steps 2-4
    u8 pad1[0xB];      // 0x81-0x8B
    u16 iconSlot;      // ext+0x10 (0x8C): index into D_us_801B34F0 (icon VRAM slot table)
    u8 pad2[6];        // 0x8E-0x93
    u16 castleFlagId;  // ext+0x18 (0x94): 1-based persistence-flag id, 0 = none
} ET_801ABF50;

extern void func_us_801AB53C(Entity* self);
extern u16 D_us_801B34F0[]; // 32-slot icon-VRAM allocation table (0 = free, nonzero = claimed/timer)
extern u16 D_us_80181920[]; // CheckFieldCollision table id for this room
extern void CheckFieldCollision(u16* table, s32 arg1);

// Equip/accessory pickup entity, dispatched via jtbl_us_8019CBAC (6 cases,
// steps 0-5). A prelude forces step to 5 (give reward + destroy) whenever
// hitFlags is set while in steps 2-4 -- i.e. touching the player's hitbox at
// any point after the icon is up auto-collects it. self->params holds the
// item id for the whole function's lifetime (masked to 15 bits into a local
// at entry): ids < 0xA9 index g_api_equipDefs (a weapon/armor/cape item),
// ids >= 0xA9 index g_api_accessoryDefs (after subtracting 0xA9). Bit 0x8000
// of the *raw* self->params (re-read directly, not the masked local) is
// checked in step 3 to short-circuit into a "reset icon timer to 0x10 and
// wait" branch instead of the normal countdown -- who sets that bit isn't
// visible in this function.
//
// step 0: if the active character is Richter (g_PlayableCharacter != 0),
//   hands off entirely to func_us_801AB53C (tags entityId=3, clears params,
//   installs it as pfnUpdate, resets to step 0, and runs it once immediately)
//   -- this pickup type apparently behaves differently for Richter and reuses
//   the companion entity function. Otherwise does a generic InitializeEntity
//   and clears the sparkle counter, then falls into the shared tail below.
// step 1: probes tile collision at the entity's own position; if blocked
//   (effects & 7), destroys self. Otherwise finds the first free slot (0-31)
//   in D_us_801B34F0; none free -> destroy without ever reaching the shared
//   tail. On success: optionally marks a g_CastleFlags+0x1B0 bit (a second,
//   bit-packed sub-region of g_CastleFlags distinct from the one-byte-per-
//   flag convention used elsewhere in this file), allocates a GT4 Primitive
//   for the icon, claims the slot (timer 0x1E0), loads the icon graphic via
//   g_api_LoadEquipIcon (icon/iconPalette straight from the Equipment or
//   Accessory def), and lays out the icon quad's UV/tpage/clut/priority from
//   the slot index (a 4x8 icon atlas: column = slot&7, row = slot>>3&3).
//   Advances to step 2.
// step 2: standard fall/land handling (mirrors func_us_801AB050's shape but
//   open-coded here): probes collision above then below, and once landed
//   (blocked below with velocityY > 0) zeroes velocity, sets cooldownTimer
//   to 0xF0, snaps posY to the tile edge, advances to step 3, then always
//   runs CheckFieldCollision and returns via the shared tail.
// step 3: calls func_us_801AB050(1) (fall+collision helper) every frame; if
//   params' raw 0x8000 bit is set, just keeps resetting D_us_801B34F0's slot
//   entry to 0x10 and waits. Otherwise decrements cooldownTimer, and once it
//   hits 0 resets it to 0x50 and advances to step 4.
// step 4: calls func_us_801AB050(1); decrements cooldownTimer, destroying
//   self once it hits 0; otherwise blinks the icon Primitive's drawMode
//   between 8 and 2 based on the timer's bit 1 (a blink-before-despawn cue).
// step 5: frees a lingering primitive chain from g_unkGraphicsStruct if one
//   is active, plays the pickup jingle, adds the item to the inventory
//   (weapon/armor/cape items use EquipKind 0, accessories use a hardcoded
//   EquipKind 2 rather than the accessory's own equipType field -- transcribed
//   as-is, not independently explained), shows the item's name via
//   BottomCornerText, and destroys self.
//
// Shared tail (all exits except step 0's Richter branch and step 1's
// no-free-slot destroy): once step >= 2, decrements sparkleArg toward 0 and
// draws the sparkle/flash Primitive via func_us_801ABEAC every frame --
// including on frames where this same tail was reached via a destroy call
// moments earlier, since DestroyEntity doesn't invalidate the struct's
// memory until the slot is actually reused.
//
// Confidence: high on control flow, Entity/Collider/Equipment/Accessory
// fields (all confirmed against include/game.h) and the icon-slot atlas
// math. Medium on ext field naming (behavior-inferred, not cross-checked
// against another function) and on the two spots called out above (the
// unexplained raw-params 0x8000 bit and the hardcoded EquipKind 2).
void func_us_801ABF50(Entity* self)
{
    ET_801ABF50* ext = (ET_801ABF50*) &self->ext;
    u16 itemId = self->params & 0x7FFF;

    if ((u16)(self->step - 2) < 3 && self->hitFlags != 0) {
        self->step = 5;
    }
    if (self->step >= 6) {
        goto tail;
    }

    switch (self->step) {
        case 0:
            if (g_PlayableCharacter != 0) {
                self->entityId = 3;
                self->params = 0;
                self->pfnUpdate = func_us_801AB53C;
                SetStep(0);
                func_us_801AB53C(self);
                return;
            }
            InitializeEntity(D_us_80180540);
            ext->sparkleArg = 0;
            goto tail;

        case 1: {
            Collider col;
            s32 slot;
            s32 primIdx;

            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi, &col, 0);
            if (col.effects & 7) {
                goto destroy_then_tail;
            }

            for (slot = 0; D_us_801B34F0[slot] != 0 && slot < 0x20; slot++) {
            }
            if (slot >= 0x20) {
                DestroyEntity(self);
                return;
            }

            if (ext->castleFlagId != 0) {
                s32 bit = ext->castleFlagId - 1;
                g_CastleFlags[0x1B0 + (bit >> 3)] |= (1 << (bit & 7));
            }

            primIdx = g_api_AllocPrimitives(4, 1);
            if (primIdx == -1) {
                DestroyEntity(self);
                return;
            }
            self->flags |= 0x800000;
            self->primIndex = primIdx;
            D_us_801B34F0[slot] = 0x1E0;

            ext->iconSlot = (u16) slot;
            {
                s32 icon, iconPalette;
                Primitive* prim = &g_PrimBuf[primIdx];
                s32 col4, row4, u, v;

                if (itemId < 0xA9) {
                    icon = g_api_equipDefs[itemId].icon;
                    iconPalette = g_api_equipDefs[itemId].iconPalette;
                } else {
                    icon = g_api_accessoryDefs[itemId - 0xA9].icon;
                    iconPalette = g_api_accessoryDefs[itemId - 0xA9].iconPalette;
                }
                g_api_LoadEquipIcon(icon, iconPalette, slot);

                col4 = (slot & 7) << 4;
                u = col4 | 1;
                col4 = col4 | 0xF;
                row4 = (slot & 0x18) << 1;
                v = row4 | 0x81;
                row4 = row4 | 0x8F;

                prim->tpage = 0x1A;
                prim->clut = (u16)(slot + 0x1D0);
                prim->v3 = (u8) row4;
                prim->v1 = (u8) row4;
                prim->u2 = (u8) u;
                prim->u0 = (u8) u;
                prim->u3 = (u8) col4;
                prim->u1 = (u8) col4;
                prim->v2 = (u8) v;
                prim->v0 = (u8) v;
                prim->priority = 0x80;
                prim->drawMode = 6;

                ext->sparkleArg = (u16) row4;
            }
            self->step++;
            goto tail;
        }

        case 2: {
            Collider col;

            if (self->velocityY < 0) {
                g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi - 7, &col, 0);
                if (col.effects & 5) {
                    self->velocityY = 0;
                }
            }
            g_api_CheckCollision(self->posX.i.hi, self->posY.i.hi + 7, &col, 0);
            if (!(col.effects & 5) || self->velocityY <= 0) {
                FallEntity();
            } else {
                self->velocityX = 0;
                self->velocityY = 0;
                ext->cooldownTimer = 0xF0;
                self->posY.i.hi = (s16)(self->posY.i.hi + col.unk18);
                self->step++;
            }
            CheckFieldCollision(D_us_80181920, 2);
            goto tail;
        }

        case 3:
            func_us_801AB050(1);
            if (self->params & 0x8000) {
                D_us_801B34F0[ext->iconSlot] = 0x10;
                goto tail;
            }
            if (--ext->cooldownTimer == 0) {
                ext->cooldownTimer = 0x50;
                self->step++;
            }
            goto tail;

        case 4: {
            u8 t = --ext->cooldownTimer;
            Primitive* prim;

            func_us_801AB050(1);
            if (t == 0) {
                goto destroy_then_tail;
            }
            prim = &g_PrimBuf[self->primIndex];
            prim->drawMode = (t & 2) ? 8 : 2;
            goto tail;
        }

        case 5: {
            s32 kind;
            u8* name;

            if (g_unkGraphicsStruct.BottomCornerTextTimer != 0) {
                g_api_FreePrimitives(g_unkGraphicsStruct.BottomCornerTextPrims);
                g_unkGraphicsStruct.BottomCornerTextTimer = 0;
            }
            g_api_PlaySfx(0x67C);
            if (itemId < 0xA9) {
                kind = 0; /* EQUIP_HAND */
                name = (u8*) g_api_equipDefs[itemId].name;
                g_api_AddToInventory(itemId, kind);
            } else {
                itemId -= 0xA9;
                kind = 2; /* hardcoded, not accessoryDef.equipType -- see note above */
                name = (u8*) g_api_accessoryDefs[itemId].name;
                g_api_AddToInventory(itemId, kind);
            }
            BottomCornerText(name, 1);
            goto destroy_then_tail;
        }
    }

destroy_then_tail:
    DestroyEntity(self);
tail:
    if (self->step >= 2) {
        if (ext->sparkleArg != 0) {
            ext->sparkleArg--;
        }
        func_us_801ABEAC(self, ext->sparkleArg);
    }
}

/*
 * Decodes ONE character from a Shift-JIS-ish text string into the g_Pix
 * scratch buffer (`dest`), and returns the advanced read cursor. `stride` is
 * the byte pitch between glyph rows in `dest` (the caller passes charW>>1,
 * i.e. bytes-per-row of a 4bpp/nibble-packed cell), and `*widthAccum` is the
 * running column write offset, updated in place by this call's glyph width.
 *
 * Encoding: plain ASCII 'a'-'z'/'A'-'Z' are treated as a compact shorthand
 * for the corresponding full-width Shift-JIS Latin letters (folded to the
 * 0x8281../0x8260.. code ranges via a fixed subtraction); ASCII space
 * (0x20) maps to the full-width space code 0x8140 directly. Anything else
 * is read as a literal 2-byte Shift-JIS code. Code 0x8168 additionally
 * skips 2 extra bytes in the source string after being read -- transcribed
 * as observed; its purpose (an embedded parameter of some kind?) isn't
 * independently confirmed.
 *
 * g_api_func_80106A28(code, 1) resolves the code to a 16-row x 6-byte glyph
 * cell (already prototyped in include/game.h; this is the first use of it
 * in the codebase). If every row's first byte is 0 (and the code isn't the
 * full-width space, which skips this check), every row is left-shifted by
 * one byte (byte[5] zeroed) and the whole check is retried -- transcribed
 * mechanically; the reason a freshly resolved glyph would need repeated
 * left-shifting isn't independently confirmed (possibly a sub-byte/row
 * alignment fixup baked into the original font data).
 *
 * The remaining logic trims trailing blank columns from the 6-byte cell to
 * get a proportional width: it finds the rightmost byte-column (0-5) that
 * has any nonzero byte across all 16 rows, then additionally checks whether
 * that column's high nibble (the right pixel of the packed byte) is blank
 * across all 16 rows to decide whether to round the width up by a further
 * column. The full 16-row x 6-byte cell is always blitted verbatim into
 * `dest` at `*widthAccum` (row pitch = `stride`); only the computed width
 * used to advance `*widthAccum` for the NEXT character is trimmed.
 *
 * Confidence: medium-high on the SJIS/ASCII-shorthand decoding and the
 * glyph blit (both directly confirmed against the asm); medium on the
 * width-trim semantics (arithmetic shape is clear, exact intended visual
 * rounding rule is inferred); low on the leading-blank-row shift loop's
 * real purpose.
 */
void* func_us_801AC538(u8* textPtr, s16* widthAccum, u8* dest, s32 stride)
{
    u8 c;
    u16 code;
    s32 extraWidth;
    u8* rows;
    u8* src;
    u8* dst;
    s32 row;
    s32 col;
    s32 maxCol;
    s32 blankCount;
    s32 rowOff;

    c = *textPtr;
    textPtr++;
    extraWidth = 0;

    if ((u32)(c - 0x61) < 0x1A) {
        code = c - 0x7DE0;
    } else if ((u32)((c - 0x41) & 0xFFFF) < 0x1A) {
        code = c - 0x7DE1;
    } else if (c == 0x20) {
        code = 0x8140;
        extraWidth = 2;
    } else {
        code = (u16)((c << 8) | *textPtr);
        textPtr++;
        if (code == 0x8140) {
            extraWidth = 2;
        }
    }

    if (code == 0x8168) {
        textPtr += 2;
    }

    rows = (u8*)g_api_func_80106A28(code, 1);

    for (;;) {
        blankCount = 0;
        if (code != 0x8140) {
            for (row = 0; row < 0x10 && rows[row * 6] == 0; row++) {
                blankCount++;
            }
        }
        if (blankCount != 0x10) {
            break;
        }
        for (row = 0; row < 0x10; row++) {
            u8* r = &rows[row * 6];
            for (col = 0; col < 5; col++) {
                r[col] = r[col + 1];
            }
            r[5] = 0;
        }
    }

    maxCol = 0;
    for (row = 0; row < 0x10; row++) {
        for (col = 0; col < 6; col++) {
            if (rows[row * 6 + col] != 0 && maxCol < col) {
                maxCol = col;
            }
        }
    }

    blankCount = 0;
    for (row = 0; row < 0x10 && !(rows[row * 6 + maxCol] & 0xF0); row++) {
        blankCount++;
    }
    if (blankCount != 0x10) {
        maxCol++;
    }
    if (maxCol < 6) {
        maxCol++;
    }

    src = rows;
    rowOff = 0;
    for (row = 0; row < 0x10; row++) {
        dst = &dest[*widthAccum + rowOff];
        for (col = 0; col < 6; col++) {
            dst[col] = src[col];
        }
        src += 6;
        rowOff += stride;
    }

    *widthAccum = (u16)(*widthAccum + maxCol + extraWidth);
    return textPtr;
}

typedef struct {
    u16 stepTimer;          // ext+0x0 (0x7C): multi-purpose frame counter, reused across steps 6/7/8/9
    u16 widthAccum;         // ext+0x2 (0x7E): saved rendered-name pixel width from step 6, used in step 9
    u8 textCycleCounter;    // ext+0x4 (0x80): counts down 4..0 between name-row rebuild cycles
    u8 pad0;                // 0x81
    u16 scrollPhase;        // ext+0x6 (0x82): increments each rebuild cycle; &7 indexes a position table
    u8 pad1[8];              // 0x84-0x8B
    u16 iconSlot;            // ext+0x10 (0x8C): index into D_us_801B34F0 (icon VRAM slot table)
    u8 pad2[2];               // 0x8E-0x8F
    u16 hoverTimer;            // ext+0x14 (0x90): countdown driving the hover oscillation (step 1)
    u8 pad3[2];                 // 0x92-0x93
    s32 hoverVelocityDelta;      // ext+0x18 (0x94): +-0x200, added to velocityY each frame, flips sign
                                  // every hoverTimer cycle -- a floating/bobbing motion for the relic
} ET_801AC7E8;

extern u16 D_us_801B34F0[];
extern u16 D_us_80181920[];
extern u8* D_us_801819EC; // shared name-prefix string ("Obtained " or similar), decoded before the relic's own name
extern u16 D_us_80181A00[]; // per-scrollPhase&7-ish index tables for the row-reveal/rebuild animation (8 entries each)
extern u16 D_us_801819F0[];
extern u16 D_us_80181A10[];
extern u16 D_us_80181A20[];
extern u16 D_us_80181A30[];
extern u16 D_us_80181A40[];
extern void* func_us_801AC538(u8* textPtr, s16* widthAccum, u8* dest, s32 stride);

// Relic pickup entity, dispatched via jtbl_us_8019CBD4 (10 cases, steps 0-9; steps 2/3/4 all map to
// the shared tail directly and have no case body of their own). Same family as func_us_801ABF50 and
// func_us_801AB53C (hitFlags-forces-step-5 prelude while step is 1-4, self->params masked to 15 bits
// as the item id at entry -- here a relic id indexing g_api_relicDefs), but relics hover in place
// rather than falling, and show a "You got the <name>" style text banner via a 7-Primitive chain
// (1 icon + 2 side "beam" quads + 4 name-text row quads) instead of func_us_801ABF50's single icon.
//
// step 0: if g_Status[itemId] (a per-relic owned-flags byte array) has bit 0 set, the relic is
//   already owned -- destroys self immediately. Otherwise finds a free icon-VRAM slot (0-30, one
//   fewer than the 0-31 range used by func_us_801ABF50/801AB53C against the same D_us_801B34F0
//   table -- transcribed as observed) and allocates a 7-Primitive GT4 chain; on failure of either,
//   resets self->step to 0 and returns directly without going through the shared tail. On success:
//   claims the slot (timer 0x10), loads the relic's icon (g_api_relicDefs[itemId].icon/iconPalette)
//   and lays out prim 0 as the icon quad (uv/tpage/clut from the slot index, same atlas math as the
//   other two functions) with the remaining 6 prims left in drawMode 8 (hidden, to be configured by
//   later steps). Sets posY's low 16 bits (the fixed-point fractional half) to a raw 0x8000 and
//   initializes the hover oscillation (velocityY=0x4000, hoverTimer=0x40, hoverVelocityDelta=-0x200).
//   Note: neither this step nor step 1 below ever advances self->step themselves; since the hitFlags
//   prelude only forces step 1-4 (not 0), something external (most likely the spawner presetting an
//   initial step rather than relying on this function to leave step 0) must be responsible for ever
//   reaching step 1 in practice -- not resolved further here.
// step 1: floating/bobbing motion -- adds hoverVelocityDelta to velocityY every frame; once
//   hoverTimer (reset to 0x40) counts down to 0, flips hoverVelocityDelta's sign and restarts the
//   timer (a slow up/down oscillation). Calls MoveEntity, and refreshes D_us_801B34F0's claimed slot
//   back to 0x10 every frame (keeping the icon-VRAM reservation alive).
// steps 2-4: no case-specific body; fall straight into the shared tail.
// step 5: calls func_800FE044(itemId, 0x2000) (marks the relic as owned, presumably). For itemId in
//   [0x14, 0x15] there is a g_Status[itemId] ^= 2 toggle gated behind a nested id check that -- given
//   the outer dispatch already requires itemId elsewhere -- appears unreachable in practice; kept
//   as-is rather than removed, since altering apparently-dead branches without evidence risks
//   silently changing behavior the disassembly doesn't actually support removing. Sets flags
//   |= 0x10000, clears a work rect via ClearImage, and builds prims 1-2 of the chain into a 2-quad
//   "beam" shape (fixed appearance, not yet animated). Advances to step 6.
// step 6: clears a 0xC00-byte region of g_Pix, then decodes D_us_801819EC (a shared name-prefix
//   string) followed immediately by g_api_relicDefs[itemId].name through func_us_801AC538 into
//   g_Pix, uploads it via LoadTPage, resets stepTimer to 0, and saves the final rendered width into
//   widthAccum. Advances to step 7.
// step 7: over 8 frames (stepTimer), animates prims 1-2 (the beam quads) outward -- x edges grow by
//   3px each side, y edges shift by differing amounts per prim -- then advances to step 8.
// step 8: over 8 frames (stepTimer, shared counter/reset logic with step 7), grows prims 1-2 further
//   using a smooth (t*120)/11 easing curve and fades their green/blue channels while repositioning
//   from small lookup tables, then advances to step 9.
// step 9: sets the icon prim (0) to drawMode 0 (visible) and repositions its x from the saved
//   widthAccum; after 0x61 (97) frames (stepTimer), destroys self.
//
// Shared tail (steps >= 2, run every frame regardless of which case above executed): draws the
// sparkle/flash Primitive via func_us_801ABEAC using the raw g_Timer as its arg1 (unlike the other
// two functions' per-entity countdown -- this one just free-rides the global frame counter for its
// blink pattern). Then, once every (textCycleCounter-worth of) few frames, scans the 4 name-text row
// prims (3-6 in the chain) for one that has fully faded (drawMode==8); if found, resets its
// appearance and repositions it from a per-scrollPhase&7 lookup table (a new "row" cycling into
// view), advancing scrollPhase either way. Independently of that, every frame it also walks those
// same 4 prims and, for each not already hidden, shifts it by 1-2px (direction/shared-state chosen
// by a per-prim low state byte at Primitive.p1&3) and fades its RGB by -6/frame, hiding it (drawMode
// 8) once its state byte reaches 0x11 -- a continuous "rows scroll past and fade" marquee effect for
// the relic's name banner.
//
// Confidence: high on control flow, Entity/Collider/Equipment/RelicDesc fields (all confirmed
// against include/game.h) and the icon-slot atlas math (identical to the other two functions in this
// family). Medium-low on the exact visual intent of the step 7/8/9 beam-growth and the tail's
// row-scroll math -- the arithmetic is transcribed directly from the asm and believed accurate, but
// (being purely cosmetic) wasn't cross-checked against any reference beyond internal consistency.
// Low on the step 5 dead-looking g_Status toggle branch and on why steps 0/1 never advance step.
void func_us_801AC7E8(Entity* self)
{
    ET_801AC7E8* ext = (ET_801AC7E8*) &self->ext;
    u16 itemId = self->params & 0x7FFF;

    if (self->step != 0 && self->step < 5 && self->hitFlags != 0) {
        self->step = 5;
    }
    if (self->step >= 0xA) {
        goto tail;
    }

    switch (self->step) {
        case 0: {
            s32 slot;
            s32 primIdx;

            if (((u8*) &g_Status)[itemId] & 1) {
                DestroyEntity(self);
                return;
            }
            InitializeEntity(D_us_80180540);

            for (slot = 0; D_us_801B34F0[slot] != 0 && slot < 0x1F; slot++) {
            }
            if (slot >= 0x1F) {
                self->step = 0;
                return;
            }
            primIdx = g_api_AllocPrimitives(4, 7);
            if (primIdx == -1) {
                self->step = 0;
                return;
            }
            self->flags |= 0x800000;
            self->primIndex = primIdx;
            ext->iconSlot = (u16) slot;
            D_us_801B34F0[slot] = 0x10;

            {
                Primitive* prim = &g_PrimBuf[primIdx];
                s32 i;
                s32 icon = g_api_relicDefs[itemId].icon;
                s32 iconPalette = g_api_relicDefs[itemId].iconPalette;
                s32 col4 = (slot & 7) << 4;
                s32 u = col4 | 1;
                s32 row4 = (slot & 0x18) << 1;
                s32 v = row4 | 0x81;

                col4 |= 0xF;
                row4 |= 0x8F;
                g_api_LoadEquipIcon(icon, iconPalette, slot);

                for (i = 0; prim != NULL; i++, prim = prim->next) {
                    if (i == 0) {
                        prim->tpage = 0x1A;
                        prim->clut = (u16)(slot + 0x1D0);
                        prim->u2 = prim->u0 = (u8) u;
                        prim->u3 = prim->u1 = (u8) col4;
                        prim->v2 = prim->v0 = (u8) v;
                        prim->v3 = prim->v1 = (u8) row4;
                        prim->drawMode = 6;
                    } else {
                        prim->drawMode = 8;
                    }
                    prim->priority = 0x7E;
                }
            }

            self->posY.i.lo = (s16) 0x8000;
            self->velocityY = 0x4000;
            ext->hoverTimer = 0x40;
            ext->hoverVelocityDelta = -0x200;
            goto tail;
        }

        case 1: {
            s32 vY = self->velocityY + ext->hoverVelocityDelta;

            if (--ext->hoverTimer == 0) {
                ext->hoverTimer = 0x40;
                ext->hoverVelocityDelta = -ext->hoverVelocityDelta;
            }
            self->velocityY = vY;
            MoveEntity();
            D_us_801B34F0[ext->iconSlot] = 0x10;
            goto tail;
        }

        case 5: {
            RECT rect;

            g_api_func_800FE044(itemId, 0x2000);
            if (itemId >= 0x16 && itemId < 0x14) {
                /* unreachable given itemId >= 0x16 above; transcribed as observed, not removed */
                ((u8*) &g_Status)[itemId] ^= 2;
            }
            self->flags |= 0x10000;

            rect.x = 0;
            rect.y = 0x100;
            rect.w = 0x40;
            rect.h = 0x10;
            ClearImage(&rect, 0, 0, 0);

            {
                Primitive* prim = &g_PrimBuf[self->primIndex];
                s32 i;

                for (i = 0; i < 3 && prim != NULL; i++, prim = prim->next) {
                    if (i == 0) {
                        prim->type = 6;
                        prim->y0 = 0xA0;
                        prim->u1 = 0xF0;
                        prim->clut = 0x1A1;
                        prim->priority = 0x1FE;
                        prim->tpage = 0x10;
                        prim->x0 = 0x10;
                        prim->u0 = 0;
                        prim->v0 = 0;
                        prim->v1 = 0x10;
                        prim->drawMode = 8;
                    } else {
                        prim->type = 3;
                        prim->x3 = prim->x2 = prim->x1 = prim->x0 = 0x80;
                        prim->y3 = prim->y2 = prim->y1 = prim->y0 = 0xA7;
                        prim->r3 = prim->r2 = prim->r1 = prim->r0 = 0;
                        prim->g3 = prim->g2 = prim->g1 = prim->g0 = 0x80;
                        prim->priority = 0x1FD;
                        prim->drawMode = 0x11;
                    }
                }
            }
            self->step++;
            goto tail;
        }

        case 6: {
            u8* textPtr = D_us_801819EC;
            s16 widthAccum = 0;
            s32 usingName = 0;

            {
                s32 n;
                u8* p = g_Pix;
                for (n = 0xC00; n > 0; n--) {
                    *p++ = 0;
                }
            }
            for (;;) {
                if (*textPtr != 0) {
                    textPtr = (u8*) func_us_801AC538(textPtr, &widthAccum, g_Pix, 0xC0);
                    continue;
                }
                if (usingName) {
                    break;
                }
                usingName = 1;
                textPtr = (u8*) g_api_relicDefs[itemId].name;
            }
            LoadTPage(g_Pix, 0, 0, 0, 0x100, 0x180, 0x10);
            ext->stepTimer = 0;
            ext->widthAccum = (u16) widthAccum;
            self->step++;
            goto tail;
        }

        case 7: {
            Primitive* prim = g_PrimBuf[self->primIndex].next;
            s32 i;

            for (i = 0; i < 2 && prim != NULL; i++, prim = prim->next) {
                s16 y0 = prim->y1, y1 = prim->y3;
                if (i == 0) {
                    prim->x2 = (s16)(prim->x2 - 3);
                    prim->x3 = (s16)(prim->x3 + 3);
                    y0 = (s16)(y0 - 4);
                    y1 = (s16)(y1 + 2);
                } else {
                    prim->x0 = (s16)(prim->x0 - 3);
                    prim->x1 = (s16)(prim->x1 + 3);
                    y0 = (s16)(y0 - 2);
                    y1 = (s16)(y1 + 4);
                }
                prim->y1 = prim->y0 = y0;
                prim->y3 = prim->y2 = y1;
            }
            goto step_7_8_tail;
        }

        case 8: {
            Primitive* prim = g_PrimBuf[self->primIndex].next;
            s32 i;
            s32 t = ext->stepTimer + 1;
            s32 growAmount = (t * 120) / 11;
            s16 shrinkA = (s16)(0x80 - t * 12);
            s16 shrinkB = (s16)(t * 12 + 0x80);
            s32 tblIdx = ext->stepTimer;

            for (i = 0; i < 3 && prim != NULL; i++, prim = prim->next) {
                s16 y;
                if (i == 0) {
                    prim->x2 = (s16)(growAmount + 0x68);
                    prim->x1 = shrinkA;
                    prim->x0 = shrinkB;
                    prim->x3 = (s16)(0x98 - growAmount);
                    y = (s16)(D_us_80181A00[tblIdx] + 0xA7);
                    prim->y1 = prim->y0 = y;
                    prim->b2 = prim->b1 = (u8)(prim->b2 - 0x10);
                    y = (s16)(D_us_801819F0[tblIdx] + 0xA7);
                } else {
                    prim->x0 = (s16) growAmount;
                    prim->x1 = (s16)(0x98 - growAmount);
                    prim->x3 = shrinkA;
                    prim->x2 = shrinkB;
                    y = (s16)(D_us_80181A10[tblIdx] + 0xA7);
                    prim->y1 = prim->y0 = y;
                    prim->g1 = prim->g0 = (u8)(prim->g1 - 0x10);
                    y = (s16)(D_us_80181A20[tblIdx] + 0xA7);
                }
                prim->y3 = prim->y2 = y;
            }
            goto step_7_8_tail;
        }

        case 9: {
            Primitive* prim = &g_PrimBuf[self->primIndex];

            prim->drawMode = 0;
            prim->x0 = (s16)(0x80 - ext->widthAccum);
            if (++ext->stepTimer >= 0x61) {
                DestroyEntity(self);
                return;
            }
            goto tail;
        }
    }
    goto tail;

step_7_8_tail:
    if (++ext->stepTimer == 8) {
        ext->stepTimer = 0;
        self->step++;
    }
    goto tail;

tail:
    if (self->step >= 2) {
        Primitive* p = &g_PrimBuf[self->primIndex];
        s32 k;

        func_us_801ABEAC(self, g_Timer);

        for (k = 0; k < 3 && p != NULL; k++) {
            p = p->next;
        }
        if (ext->textCycleCounter != 0) {
            ext->textCycleCounter--;
        } else {
            Primitive* q = p;
            s32 j;
            s32 found = 0;

            for (j = 0; j < 4 && q != NULL; j++, q = q->next) {
                if (q->drawMode == 8) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                s32 idx = ext->scrollPhase & 7;
                s16 cx = (s16)(self->posX.i.hi + D_us_80181A30[idx]);
                s16 cy = (s16)(self->posY.i.hi + D_us_80181A40[idx]);

                q->tpage = 0x1A;
                q->clut = 0x1B1;
                q->u3 = q->u1 = 0x10;
                q->v2 = q->v0 = 0x50;
                q->u2 = q->u0 = 0;
                q->v3 = q->v1 = 0x60;
                q->x2 = q->x0 = (s16)(cx - 6);
                q->x3 = q->x1 = (s16)(cx + 6);
                q->y1 = q->y0 = (s16)(cy - 6);
                q->y3 = q->y2 = (s16)(cy + 6);
                q->r3 = q->r2 = q->r1 = q->r0 = 0x80;
                q->g3 = q->g2 = q->g1 = q->g0 = 0x80;
                q->b3 = q->b2 = q->b1 = q->b0 = 0x80;
                q->priority = 0x7F;
                q->p1 = 0;
                q->drawMode = 0x37;
            }
            ext->scrollPhase++;
            ext->textCycleCounter = 4;
        }
    }

    {
        Primitive* p = &g_PrimBuf[self->primIndex];
        s32 k;

        for (k = 0; k < 3 && p != NULL; k++) {
            p = p->next;
        }
        while (p != NULL) {
            if (p->drawMode != 8) {
                u8 state = p->p1;

                if (state & 3) {
                    s16 y1 = p->y1, y3 = p->y3;
                    p->y1 = (s16)(y1 - 1);
                    p->y0 = y1;
                    p->y3 = (s16)(y3 - 1);
                    p->y2 = y3;
                } else {
                    s16 x2 = p->x2, x3 = p->x3;
                    p->y3 = p->y2 = (s16)(p->y3 - 2);
                    p->x2 = (s16)(x2 + 1);
                    p->x0 = x2;
                    p->x3 = (s16)(x3 - 1);
                    p->x1 = x3;
                }
                state++;
                p->p1 = state;
                p->r3 = p->r2 = p->r1 = p->r0 = (u8)(p->r3 - 6);
                p->g3 = p->g2 = p->g1 = p->g0 = (u8)(p->g3 - 6);
                p->b3 = p->b2 = p->b1 = p->b0 = (u8)(p->b3 - 6);
                if (state >= 0x11) {
                    p->drawMode = 8;
                }
            }
            p = p->next;
        }
    }
}

typedef struct {
    u8 pad0[0x38];   // ext+0x0-0x37 (unused by this function)
    u16 flagIndex;   // ext+0x38: index into the g_CastleFlags+0x100 persistence bitset
} ET_801AD2B8;
typedef void (*BehaviorFn)(Entity* self);

extern void func_us_801AB53C(Entity* self); // now decompiled above -- single-arg (see its own comment)
extern void func_us_801ABF50(Entity* self); // now decompiled above -- single-arg (see its own comment)
extern u16 D_us_8018108C[]; // per-id behavior-select table (>= 0x80 selects func_us_801ABF50, biases params)

// Generic "persistent breakable" dispatcher: on first frame, computes a
// per-instance flag index (params + 0x40) into a *bit-packed* region of
// g_CastleFlags starting at +0x100 (distinct from the one-byte-per-flag
// convention used elsewhere in this file, e.g. func_us_801A032C/801A051C's
// g_CastleFlags[0xC1] -- this is a second, denser sub-region of the same
// 0x300-byte buffer). If that bit is already set (this object was already
// destroyed on a previous room visit), destroys itself immediately without
// running any behavior. Otherwise looks up which of two shared behavior
// functions actually drives this object from a table, storing the choice
// directly in the Entity's own `unkB8` field (normally an Entity* "next
// part" pointer, repurposed here as a function pointer -- a lightweight
// vtable so many different breakable props can share one entity type). Once
// the object is actually destroyed (hitFlags set while step < 5), sets the
// persistence bit so it won't respawn on a later room visit.
// Confidence: medium-high on the control flow and the bit-packed persistence
// idiom (the double meaning of g_CastleFlags is confirmed structurally, even
// though the exact bit region's purpose is inferred); low on unkB8-as-
// function-pointer's real behavior, since both candidate functions
// (func_us_801AB53C/801ABF50) are still blocked by the same missing-jumptable
// issue affecting 14 other functions in this boss and haven't been
// decompiled yet. The extra DestroyEntity argument seen in the raw m2c
// output (DestroyEntity(self, self)) is a stale-register artifact of the
// same class documented repeatedly in bo1 -- dropped.
void func_us_801AD2B8(Entity* self, u32 arg1)
{
    ET_801AD2B8* ext = (ET_801AD2B8*)&self->ext;
    u16 id;
    u16 behaviorId;

    (void) arg1;
    if (self->step == 0) {
        id = self->params + 0x40;
        ext->flagIndex = id;
        if ((g_CastleFlags[0x100 + (id >> 3)] >> (id & 7)) & 1) {
            DestroyEntity(self);
            return;
        }
        behaviorId = D_us_8018108C[(id - 0x40) & 0xFFFF];
        if (behaviorId < 0x80) {
            *(BehaviorFn*)&self->unkB8 = func_us_801AB53C;
        } else {
            *(BehaviorFn*)&self->unkB8 = func_us_801ABF50;
            behaviorId -= 0x80;
        }
        self->params = (u16)(behaviorId + 0x8000);
        goto dispatch;
    }
    id = ext->flagIndex;
    if (self->step < 5) {
        u32 bitIndex = id >> 3;
        if (self->hitFlags != 0) {
            g_CastleFlags[0x100 + bitIndex] = g_CastleFlags[0x100 + bitIndex] | (1 << (id & 7));
            self->step = 5;
        }
    }
dispatch:
    (*(BehaviorFn*)&self->unkB8)(self);
}

extern u16 D_us_80180540[]; // InitializeEntity params
extern void* func_us_801AC538(u8* textPtr, s16* widthAccum, u8* dest, s32 stride);

typedef struct {
    u8* textPtr;   // ext+0x0 (0x7C): raw cursor into the string being rendered
    u16 charW;     // ext+0x4 (0x80): glyph cell width
    u16 charH;     // ext+0x6 (0x82): glyph cell height / CLUT id (dual use, per asm)
    u8 pad[2];     // ext+0x8-0x9 (0x84-0x85, unused by this function)
    u16 pageCounter; // ext+0xA (0x86): frames-since-texture-upload counter
} ET_801AD3E0;

/*
 * Text-box rendering entity (5-state machine, dispatched via
 * jtbl_us_8019CBFC -- confirmed via asm/us/boss/bo3/data/0.data.s: entries
 * 0x801AD428/0x801AD50C/0x801AD6E8/0x801AD7C0/0x801AD8C4, matching the
 * 5 cases below in order). Confidence: medium overall -- control flow and
 * Primitive fields (using the real names from include/primitive.h,
 * confirmed against func_us_801AD3E0.s throughout) are faithfully
 * transcribed, but two spots are flagged individually below where a
 * register's value couldn't be cleanly traced to a source.
 *
 * step 0: one-time init, allocates a 3-Primitive GT4 chain (index cached
 * in self->primIndex), clears it, then reads the first 2 bytes of the
 * text string pointed to by self->ext (already assumed populated by the
 * spawner) into charW/charH.
 * step 1: clears a work rect via ClearImage, then walks the Primitive
 * chain building 3 quads (a leading, middle, and trailing cap) sized from
 * charW/charH and the entity's own posX/posY; simultaneously walks the
 * text string, decoding each character via func_us_801AC538 into the
 * g_Pix scratch buffer (a 0x01 byte in the string is a newline/row-advance
 * control code), until the string's NUL terminator, then calls LoadTPage
 * to upload g_Pix and advances to the flush tail shared with step 3/4.
 * step 2: just advances to the next step (a one-frame delay).
 * step 3: on step_s/pageCounter==8, re-derives each Primitive's y-position
 * from a scaled pageCounter*charH term (a scroll/reveal animation), then
 * falls into step 4's tail once pageCounter reaches 8.
 * step 4: clears the chain's first Primitive drawMode, advances
 * pageCounter, and self-destroys once pageCounter exceeds self->params
 * (a display-duration counter).
 */
void func_us_801AD3E0(Entity* self)
{
    ET_801AD3E0* ext = (ET_801AD3E0*) &self->ext;

    switch (self->step) {
        case 0: {
            Primitive* prim;
            s32 primBase;

            InitializeEntity(D_us_80180540);
            self->flags = (self->flags | 0x10000) ^ 0x8000000;
            if (self->params == 0) {
                self->params = 0x60;
            }

            primBase = g_api_AllocPrimitives(4, 3);
            if (primBase == -1) {
                self->step = 0;
                return;
            }
            self->primIndex = primBase;
            self->flags |= 0x800000;
            prim = &g_PrimBuf[primBase];
            while (prim != NULL) {
                prim->drawMode = 8;
                prim = prim->next;
            }

            ext->charW = *ext->textPtr;
            ext->charH = ext->textPtr[1];
            ext->textPtr += 2;
            return;
        }

        case 1: {
            Primitive* prim;
            u8* textPtr;
            s16 widthAccum;
            RECT rect;
            s32 i;

            rect.x = 0;
            rect.y = 0x180;
            rect.w = 0x40;
            rect.h = ext->charH;
            ClearImage(&rect, 0, 0, 0);

            prim = &g_PrimBuf[self->primIndex];
            if (prim != NULL) {
                for (i = 0; prim != NULL; i++, prim = prim->next) {
                    if (i == 0) {
                        prim->type = 6; /* PRIM_SPRT */
                    } else {
                        prim->type = 3; /* PRIM_G4 */
                    }
                    prim->tpage = 0x10;
                    prim->x0 = (s16)(self->posX.i.hi - (ext->charW >> 1));
                    prim->u0 = 0;
                    prim->v0 = 0x80;
                    prim->y0 = (s16)(self->posY.i.hi - (ext->charH >> 1));

                    if (i == 0) {
                        prim->u1 = (u8) ext->charW;
                        prim->clut = 0x1A1;
                        prim->priority = 0x1FD;
                        prim->drawMode = 8;
                        prim->v1 = (u8) ext->charH;
                    } else {
                        s16 x, y;

                        prim->r3 = 0;
                        prim->x2 = prim->x0 = (s16)((self->posX.i.hi - (ext->charW >> 1)) - 4);
                        prim->g3 = prim->b3 = prim->u0 = prim->r0 = 0;
                        prim->g0 = prim->b0 = prim->type = 0;
                        prim->g1 = prim->b1 = prim->r1 = prim->g2 = 0;
                        prim->b2 = prim->r2 = 0;
                        x = (s16)((self->posX.i.hi + (ext->charW >> 1)) + 4);

                        if (i == 1) {
                            prim->b3 = prim->b2 = prim->b1 = prim->b0 = 0x80;
                            y = (s16)((self->posY.i.hi - (ext->charH >> 1)) - 4);
                        } else {
                            prim->g3 = prim->g2 = prim->g1 = prim->g0 = 0x80;
                            y = (s16)((self->posY.i.hi + (ext->charH >> 1)) + 4);
                        }

                        prim->y3 = prim->y2 = prim->y1 = prim->y0 = y;
                        prim->priority = 0x1FC;
                        prim->drawMode = 0x11;
                        (void) x; /* consumed via x3/x1 below */
                        prim->x3 = prim->x1 = x;
                    }
                }
            }
            self->step++;

            textPtr = ext->textPtr;
            widthAccum = 0;
            {
                s32 v0 = (ext->charW >> 1) * ext->charH;
                u8* dst = g_Pix;

                if (v0 > 0) {
                    s32 a2 = 0;
                    do {
                        *dst = 0;
                        dst++;
                        a2++;
                        v0 = (ext->charW >> 1) * ext->charH;
                    } while (a2 < v0);
                }
            }
            while (*textPtr != 0) {
                if (*textPtr == 1) {
                    textPtr++;
                    widthAccum = 0;
                } else {
                    textPtr = (u8*) func_us_801AC538(textPtr, &widthAccum, g_Pix, ext->charW >> 1);
                }
            }
            /* tp param below is carried over from the char-decode loop in the
             * real asm ($a1 is never explicitly reloaded before this call) --
             * not independently resolved, kept as 0. */
            LoadTPage(g_Pix, 0, 0, 0, 0x180, ext->charW, ext->charH);
            return;
        }

        case 2:
            self->step++;
            return;

        case 3: {
            Primitive* prim = &g_PrimBuf[self->primIndex];
            s32 i;

            ext->pageCounter++;
            if (prim != NULL) {
                for (i = 0; prim != NULL; i++, prim = prim->next) {
                    s32 scaled = (ext->charH + 8) * ext->pageCounter;

                    if (scaled < 0) {
                        scaled += 7;
                    }
                    scaled >>= 3;

                    if (i == 0) {
                        prim->y3 = prim->y2 = (s16)(prim->y2 + scaled);
                        prim->v1 = (u8)(prim->v1 - 0x10);
                        prim->b0 = prim->v1;
                    } else {
                        prim->y1 = prim->y3 = (s16)(prim->y2 - scaled);
                        prim->g3 = (u8)(prim->g3 - 0x10);
                        prim->g2 = prim->g3;
                    }
                }
            }
            if (ext->pageCounter != 8) {
                return;
            }
            /* fallthrough into step 4's shared tail */
        }

        case 4:
            g_PrimBuf[self->primIndex].drawMode = 0;
            ext->pageCounter++;
            if (ext->pageCounter > self->params) {
                DestroyEntity(self);
            }
            return;
    }
}

