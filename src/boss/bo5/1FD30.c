#include "common.h"
#include <stage.h>

extern void DestroyEntity(Entity* entity);
extern void InitializeEntity(u16 arg0[]);
extern s32 UnkCollisionFunc3(s16* sensors);
// Still raw INCLUDE_ASM stubs in this overlay's own create_entity.c.
extern void func_us_801A922C(s32 entityId, Entity* dest);
extern void func_us_801A92A0(s32 arg0, Entity* source, Entity* dest);
// Return value observed to matter at one call site (func_us_801A3E78), so
// not void -- treated as a "still playing" bool, ignored where unused.
extern s32 AnimateEntity(u8* anim, Entity* self);

extern u16 D_us_801804F0[]; // InitializeEntity() param table
extern u16 D_us_80180484[]; // InitializeEntity() param table
extern u16 D_us_801804CC[]; // InitializeEntity() param table
extern u16 D_us_80180454[]; // InitializeEntity() param table
extern u16 D_us_8018046C[]; // InitializeEntity() param table
extern Entity g_Entities_224[32]; // secondary entity pool, per its own AllocEntity(start,start+0x1780) call
extern u8* D_us_801805DC[]; // AnimateEntity() data table, indexed by (self->params >> 12)
extern u8 D_us_80180604[]; // byte lookup, same index as D_us_801805DC
extern u8 D_us_8018061C[]; // blendMode lookup, same index
extern u8 D_us_801805FC[]; // hitboxHeight lookup, same index
extern u16 D_us_8018060C[]; // animSet lookup, same index
extern u8 D_us_80180A18[]; // AnimateEntity() data table for func_us_801A3E78
extern u8* D_us_80180C1C[]; // per-idx encoded byte strings (0xFF = escape, 0 = terminator)
extern s32 D_us_801B5570; // last AllocPrimitives() result for func_us_801A48C8's sprite row
extern void ReplaceBreakableWithItemDrop(Entity* entity);

// Small state block touched by func_us_801A459C's init and (not yet written)
// sibling step-machine functions elsewhere in this file.
extern u16 D_us_801B5540;
extern u16 D_us_801B5542;
extern u16 D_us_801B5544;
extern u16 D_us_801B5546;
extern u16 D_us_801B5548;
extern s16 D_us_801B554A; // index (0-4) into D_us_801B5554[]; read via `lh`, unlike its siblings
extern u16 D_us_801B554C;
extern u16 D_us_801B554E;
extern u16 D_us_801B5550;
extern u8 D_us_801B5552;
extern u8 D_us_801B5553;
extern Primitive* D_us_801B5554[5]; // score-digit-style prim pool, indexed by D_us_801B554A
extern s16 D_us_801806C0[6]; // pos-delta table (x,y,params x2) for func_us_801A1010's 2 boss-part inits
extern s32 D_us_801806DC; // "music already stopped" flag checked by func_us_801A4494
extern s32 D_us_80180B74; // SFX-already-played flag checked by func_us_801A4494
extern s16 PLAYER_posX_i_hi;

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

// Palette-swap scratch bytes for the 2 on-screen GPU draw buffers, touched by
// func_us_801A1404's shared tail every frame.
extern u8 g_GpuBuffers_0_draw_r0;
extern u8 g_GpuBuffers_0_draw_g0;
extern u8 g_GpuBuffers_0_draw_b0;
extern u8 g_GpuBuffers_1_draw_r0;
extern u8 g_GpuBuffers_1_draw_g0;
extern u8 g_GpuBuffers_1_draw_b0;
extern s16 PLAYER_posY_i_hi;

// Per-animation-frame hitbox-shape index (0-6) lookup, consumed by
// func_us_801A425C; values >= 7 mean "no hitbox update this frame".
extern u8 D_us_80180B0C[];

// --- func_us_801A1C14 (main per-frame conductor) support ---
extern u16 D_us_801804D8[]; // InitializeEntity() param table (step 0)
extern u8 D_us_8018091C[]; // AnimateEntity() data table (step 1)
extern u8 D_us_80180804[]; // AnimateEntity() data table (step 3)
extern u8 D_us_801808BC[]; // AnimateEntity() data table (step 4, step_s 0)
extern u8 D_us_801808D4[]; // AnimateEntity() data table (step 4, step_s 2)
extern u8 D_us_80180830[]; // AnimateEntity() data table (step 4/8, drift dive)
extern u8 D_us_80180858[]; // AnimateEntity() data table (step 8, step_s 1)
extern u8 D_us_8018088C[]; // AnimateEntity() data table (step 10, step_s 0)
extern u8 D_us_80180894[]; // AnimateEntity() data table (step 10, step_s 1)
extern u8 D_us_8018089C[]; // AnimateEntity() data table (step 10, step_s 2)
extern u8 D_us_801808AC[]; // AnimateEntity() data table (step 10, step_s 3, bat-form path)
extern u8 D_us_80180938[]; // AnimateEntity() data table (step 12, step_s 0)
extern u8 D_us_80180944[]; // AnimateEntity() data table (step 12, step_s 2)
extern s16 D_us_80180AAC[]; // CheckColliderOffsets()/func_us_801A1BA0() sensor table (multiple steps)
extern s16 D_us_80180AE4[]; // CheckColliderOffsets() sensor table (step 16)
extern s16 D_us_80180ADC[]; // CheckColliderOffsets() sensor table (steps 12/32)
extern u8 D_us_80180AEC[]; // per-(side,distance-bucket) animCurFrame-target lookup (step 10)
extern u8 D_us_80180950[]; // AnimateEntity() data table (step 8, step_s 3)
extern void func_us_801A3B88(Entity* self); // forward decl: assigned as pfnUpdate before its own definition later in this file
extern u8 D_us_801809D8[]; // AnimateEntity() data table (step 36, step_s 6)
extern u8 D_us_80180A2C[]; // AnimateEntity() data table (step 64)
extern u8 D_us_80180A48[]; // AnimateEntity() data table (step 66)
extern s16 D_us_80180ACC[]; // UnkCollisionFunc3() sensor table (step 66)
extern s16 GetDistanceToPlayerX(void);
extern s32 GetDistanceToPlayerY(void);
extern u8 CheckColliderOffsets(s16* sensors, u8 facing);
extern char D_us_8019F5D4[]; // debug FntPrint() format string (step 130, cheat/debug step)
extern s32 D_us_801806E4[]; // per-index packed hi16/lo16 initial primitive-corner offsets (step 36, step_s 3)
extern u8 D_us_80180884[]; // AnimateEntity() data table (steps 13/22)
extern u8 D_us_801808FC[]; // AnimateEntity() data table (step 16, step_s 2)
extern u8 D_us_8018096C[]; // AnimateEntity() data table (step 20)
extern u8 D_us_80180928[]; // AnimateEntity() data table (step 32)
extern u8 D_us_8018098C[]; // AnimateEntity() data table (step 20 launcher)
extern u8 D_us_80180824[]; // AnimateEntity() data table (step 13)
extern u8 D_us_80180844[]; // AnimateEntity() data table (step 13, step_s 3+)

extern void func_us_801AEE54(s32 entityId, Entity* source, Entity* dest);
extern s32 D_us_801B5574; // last AllocPrimitives() result for func_us_801A45F0's HUD icon chain
extern Entity* D_us_801B553C; // owner entity for func_us_801A45F0's HUD icon chain
extern s32 D_us_801B556C;
extern Primitive* D_us_801B5568; // 6th slot of the HUD icon chain, separate from D_us_801B5554[5]

// Shared collision helpers, defined in st_common.h (not pulled in by this
// overlay's own includes) -- same pattern as the other st_* helpers already
// extern'd above.
extern s32 UnkCollisionFunc2(s16* posX);
extern u8 GetPlayerCollisionWith(Entity* self, u16 w, u16 h, u16 flags);
extern s16 D_us_8018062C[]; // UnkCollisionFunc2() sensor table for func_us_801A0184
extern u16 D_8009748E; // debug hit counter, incremented alongside PLAYER_posY_i_hi

extern u8 GetSideToPlayer(void); // st_common.h, not pulled in by this overlay
extern void PlaySfxPositional(u16 sfxId); // st_common.h, not pulled in by this overlay
extern s32 D_us_801806E0; // global bitflag checked by func_us_801A3FD4 (bit 1: "self is a chain-part")
extern Entity g_Entities_160[32]; // secondary entity pool, same AllocEntity(start,start+0x1780) convention as g_Entities_224
extern u16 D_us_801804E4[]; // InitializeEntity() param table
extern s16 D_us_80180ABC[]; // UnkCollisionFunc3() sensor table
extern u8 D_us_801809F0[]; // AnimateEntity() data table (spin-launch anim)
extern u8 D_us_801809F8[]; // AnimateEntity() data table (falling anim)

extern Entity g_Entities_64[32]; // same 32-entity-window convention as g_Entities_160/224
extern u32 D_us_801B55E8; // bitfield of "waited-on" flags, set/cleared/tested by func_us_801A4A78's opcodes 2/3
extern PfnEntityUpdate D_us_80180398[]; // entityId -> update-function-pointer lookup, shared with func_us_801AEE54 (e_misc.c)
extern u16 D_us_801804A8[]; // InitializeEntity() param table
extern u8 D_us_80180A00[]; // AnimateEntity() data table (grow-in-place anim, func_us_801A3B88 step 1)
extern u8 D_us_80180A0C[]; // AnimateEntity() data table (func_us_801A3B88 step 2)

// --- func_us_801A4E40 (death-cutscene byte-code interpreter) support ---
extern s32 D_us_801B55EC; // "cutscene primitives allocated" pending flag, set by step5's alloc, tested at top of func_us_801A4E40
extern Entity D_us_80181D5C; // dummy/persistent owner entity passed to func_us_801A45F0 from the cutscene's step 0
extern u8 D_us_80180B78[]; // per-(bit0) lookup, opcode 5
extern u8 D_us_80180B7C[]; // per-(bit0) lookup, opcode 5
extern u16 D_us_80180B80[]; // per-byte0 u16 lookup, opcode 5
extern u16 D_us_80180B84[]; // tpage/clut-ish u16 lookup, opcodes 5 and 19 (LoadTPage)
extern u16 D_us_80180B88[]; // per-byte0 u16 lookup, opcode 5
extern u16 D_us_80180B8C[]; // per-index u16 lookup used while initializing the 0x48-primitive death-burst chain (step 5)

extern void SetStep(u8 step); // st_common.h, not pulled in by this overlay
extern u16 D_us_80180460[]; // InitializeEntity() param table
extern u8 D_us_80180C24[]; // AnimateEntity() data table
extern u8 D_us_80180C48[]; // AnimateEntity() data table
extern u8 D_us_80180C5C[]; // AnimateEntity() data table
extern u8 D_us_80180C6C[]; // AnimateEntity() data table
extern u8 D_us_80180C74[]; // AnimateEntity() data table
extern u8 D_us_80180C84[]; // AnimateEntity() data table
extern u8 D_us_80180C90[]; // AnimateEntity() data table
extern u8 D_us_80180C98[]; // AnimateEntity() data table
extern u8 D_us_80180CA8[]; // AnimateEntity() data table
extern u8 D_us_80180CC0[]; // AnimateEntity() data table
extern u8 D_us_80180CCC[]; // AnimateEntity() data table
extern u8 D_us_80180CD4[]; // AnimateEntity() data table

extern s16 D_us_801806B4[]; // per-params {count, radius, zPriorityBase} triples, stride 3 s16
extern SVECTOR* D_us_80180684[12]; // 3 groups of 4 corner-vector pointers for the rotating fan shape
extern VECTOR D_us_801806CC; // fixed forward-translation vector for TransMatrix
extern GAME_IMPORT GpuBuffer* g_CurrentBuffer;
extern DR_ENV* (*g_api_func_800EDB08)(POLY_GT4* poly);
extern void (*g_api_func_8010DFF0)(s32 arg0, s32 arg1);
extern void (*g_api_func_8010E168)(s32 arg0, s16 arg1);

void func_us_8019FD30(Entity* self) {
    s32 idx;

    idx = (self->params >> 12) & 0xFFFF;

    if (self->step != 0) {
        Entity* part;

        AnimateEntity(D_us_801805DC[idx], self);
        if (self->hitParams != 0) {
            g_api_PlaySfx(0x634);
            part = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
            if (part != NULL) {
                func_us_801A922C(2, part);
                part->params = D_us_80180604[idx];
            }
            ReplaceBreakableWithItemDrop(self);
        }
    } else {
        InitializeEntity(D_us_80180454);
        self->zPriority = g_unkGraphicsStruct.g_zEntityCenter - 0x14;
        self->blendMode = D_us_8018061C[idx];
        self->hitboxHeight = D_us_801805FC[idx];
        self->hitboxOffX = 8;
        self->hitboxOffY = 8;
        self->animSet = D_us_8018060C[idx];
        if (idx == 0) {
            self->hitboxWidth = 4;
            self->hitboxOffX = 2;
            self->hitboxOffY = 2;
        }
    }
}

// Scripted camera-boundary trigger: step 0 just inits; step 1 waits until
// the player is within 0x20 units of self vertically, then (if
// self->params selects one of 15 known zones) checks a per-zone g_PlayerX
// threshold and, if crossed, widens/narrows the tilemap's scroll box
// (left/right/x/width) accordingly and advances step so it only fires once.
void func_us_8019FE84(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801804A8);
            return;

        case 1: {
            s16 dy = self->posY.i.hi - PLAYER_posY_i_hi;
            s16 zone;

            if (dy < 0) {
                dy = -dy;
            }
            if (dy >= 0x20) {
                return;
            }

            zone = (s16)self->params;
            if ((u16)zone >= 0xF) {
                return;
            }

            switch (zone) {
                case 0:
                case 11:
                    if (g_PlayerX < 0x281) {
                        return;
                    }
                    g_Tilemap.width = 0x280;
                    g_Tilemap.right -= 1;
                    break;

                case 2:
                case 4:
                case 13:
                case 14:
                    if (g_PlayerX < 0x301) {
                        return;
                    }
                    g_Tilemap.width = 0x300;
                    g_Tilemap.right -= 1;
                    break;

                case 1:
                case 10:
                case 12:
                    if (g_PlayerX >= 0x180) {
                        return;
                    }
                    g_Tilemap.x = 0x180;
                    g_Tilemap.left += 1;
                    break;

                case 3:
                    if (g_PlayerX >= 0x100) {
                        return;
                    }
                    g_Tilemap.x = 0x100;
                    g_Tilemap.left += 1;
                    break;

                case 5:
                case 8:
                    if (g_PlayerX >= 0x80) {
                        return;
                    }
                    g_Tilemap.x = 0x80;
                    break;

                case 6:
                case 7:
                    if (g_PlayerX < 0x481) {
                        return;
                    }
                    g_Tilemap.width = 0x480;
                    break;

                case 9:
                    if (g_PlayerX < 0x281) {
                        return;
                    }
                    g_Tilemap.width = 0x280;
                    break;

                default:
                    return;
            }

            self->step++;
            return;
        }

        default:
            return;
    }
}

// Step machine for a stationary hazard sharing its slot with a trailing
// "gate" part (self+1 as an adjacent Entity array slot): step 0 spawns/
// positions the gate part unless a one-time g_CastleFlags[0x60] latch is
// already set (in which case self just relocates itself instead); step 1
// (and step 0's non-latched path) waits for the gate part to vanish, then
// sets the latch and advances; step 2 runs a wall-sensor check and plays a
// periodic clank sound, advancing once the sensor is clear. Every step ends
// by refreshing 2 alternating player-hitbox checks.
void func_us_801A0184(Entity* self) {
    Entity* gate = (Entity*)((u8*)self + 0xBC);
    u16 hitboxW1;
    u8 collision;

    switch (self->step) {
        case 0: {
            u8 flag = g_CastleFlags[0x60];

            InitializeEntity(D_us_801804CC);
            self->animCurFrame = 1;

            if (flag != 0) {
                self->step = 0x10;
                self->posX.i.hi += 0x20;
                self->posY.i.hi += 0x20;
                goto tail;
            }

            func_us_801A92A0(0x1A, self, gate);
            gate->posX.i.hi += 0x10;
            gate->posY.i.hi += 0x10;
            goto checkGate;
        }

        case 1:
        checkGate:
            if (gate->entityId != 0) {
                goto tail;
            }
            g_CastleFlags[0x60] = 1;
            goto stepIncrement;

        case 2: {
            s32 sensor = UnkCollisionFunc2(D_us_8018062C);

            self->velocityX = sensor;
            if ((g_Timer & 7) == 0) {
                g_api_PlaySfx(0x644);
            }
            if (sensor & 0x60) {
                goto tail;
            }
            goto stepIncrement;
        }

        default:
            goto tail;
    }

stepIncrement:
    self->step++;

tail:
    self->hitboxState = 1;
    self->hitboxOffX = 8;
    self->hitboxOffY = -0x44;
    self->hitboxWidth = 8;
    self->hitboxHeight = 6;

    hitboxW1 = 8;
    collision = GetPlayerCollisionWith(self, hitboxW1, 6, 4);
    if (collision & 4) {
        hitboxW1 = 0x10;
        D_8009748E++;
        PLAYER_posY_i_hi++;
    }

    self->hitboxOffX = -0x10;
    self->hitboxOffY = -0x3C;
    self->hitboxWidth = 0x10;
    self->hitboxHeight = 0x30;
    GetPlayerCollisionWith(self, hitboxW1, 0x30, 7);
}

void func_us_801A037C(Entity* self) {
    Entity* part;

    if (self->step == 0) {
        InitializeEntity(D_us_801804CC);
        self->animCurFrame = 2;
        self->hitboxState = 2;
        self->hitPoints = 4;
        self->hitboxOffX = -8;
        self->hitboxOffY = -0x20;
        self->hitboxWidth = 8;
        self->hitboxHeight = 0x10;
        if (PLAYER_posX_i_hi < 0x80) {
            self->hitboxState = 0;
        }
    }

    if (self->flags & 0x100) {
        part = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (part != NULL) {
            func_us_801A92A0(2, self, part);
            part->params = 1;
            part->posX.i.hi -= 8;
            part->posY.i.hi -= 8;
        }
        g_api_PlaySfx(0x655);
        DestroyEntity(self);
    }
}

// Hippogryph "feather fan" ring effect: step 0 (which never advances, so this
// really means "always") allocates and lays out a GT4 primitive chain sized
// by D_us_801806B4[params] (an anchor quad + `count` fan blades + a fixed
// 3-quad tail), builds a rotation matrix from an angle swept once per frame
// across the whole ring, and projects each blade's 4 GTE SVECTORs
// (D_us_80180684) through the PS1 hardware transform pipeline into the
// primitive chain's screen coordinates. The always-run tail below applies a
// simple homing/oscillation update to a secondary "target offset" (ext+8/
// +0xC/+0x10) and, when close enough to the player, nudges self's own
// hitbox/velocity and may play a hit sound.
//
// LOW CONFIDENCE on the exact GTE scratch-buffer construction around the
// g_api_func_800EDB08 POLY_GT4 staging block (raw byte tag/color values
// copied via unaligned load/store in the original asm) -- reproduced as a
// direct byte-for-byte local buffer rather than named POLY_GT4 fields, since
// the true semantics of that internal helper aren't recoverable from headers
// alone. Everything else (Entity/Primitive field writes, control flow,
// AllocPrimitives sizing) is derived with high confidence.
void func_us_801A0490(Entity* self) {
    s32* ext84 = (s32*)((u8*)self + 0x84);
    s32* ext88 = (s32*)((u8*)self + 0x88);
    s32* ext8C = (s32*)((u8*)self + 0x8C);
    s32* ext9C = (s32*)((u8*)self + 0x9C);

    if (self->step == 0) {
        s32 primIndex;
        Primitive* p;
        s16* table = &D_us_801806B4[self->params * 3];
        s16 count = table[0];
        s16 radius = table[1];
        s16 zBase = table[2];
        s32 i;

        InitializeEntity(D_us_80180484);
        *ext9C = 0;
        self->zPriority = g_unkGraphicsStruct.g_zEntityCenter + 8;

        primIndex = g_api_AllocPrimitives(PRIM_GT4, count + 6);
        if (primIndex == -1) {
            self->step = 0;
            return;
        }

        p = &g_PrimBuf[primIndex];
        self->primIndex = primIndex;
        self->ext.prim = p;
        self->flags |= FLAG_HAS_PRIMS;

        /* anchor quad, centered on self */
        p->tpage = 0xF;
        p->clut = 0x76;
        p->u0 = p->u2 = 0x60;
        p->u1 = p->u3 = 0x80;
        p->v0 = p->v1 = 0;
        p->v2 = p->v3 = 0x18;
        p->x0 = p->x2 = self->posX.i.hi - 0x10;
        p->x1 = p->x3 = self->posX.i.hi + 0x10;
        p->y0 = p->y1 = self->posY.i.hi - 0xA;
        p->y2 = p->y3 = p->y1 + 0x18;
        p->priority = zBase - 1;
        p->drawMode = 2;
        p = p->next;

        /* `count` fan-blade quads */
        for (i = 0; i < count; i++) {
            p->tpage = 0xF;
            p->clut = 0x76;
            p->u0 = p->u2 = 0x78;
            p->u1 = p->u3 = 0x80;
            p->v0 = p->v1 = 0x41;
            p->v2 = p->v3 = radius + 0x41;
            p->priority = zBase;
            p->drawMode = 2;
            p = p->next;
        }

        /* fixed 1 + 2 tail quads (glow / halo layers) */
        p->tpage = 0xF;
        p->clut = 0x76;
        p->u0 = p->u2 = 0x60;
        p->u1 = p->u3 = 0x80;
        p->v0 = p->v1 = 0x24;
        p->v2 = p->v3 = 0x3E;
        p->priority = zBase;
        p->drawMode = 2;
        p = p->next;

        for (i = 0; i < 2; i++) {
            p->tpage = 0xF;
            p->clut = 0x75;
            p->u0 = p->u2 = 0xB1;
            p->u1 = p->u3 = 0xE0;
            p->v0 = p->v1 = 1;
            p->v2 = p->v3 = 0x61;
            p->priority = zBase;
            p->drawMode = 2;
            p = p->next;
        }

        /* Copy the current GPU buffer's draw environment, clip it to this
         * shape's screen footprint, and register a temporary DR_ENV for the
         * first 2 chain primitives via the internal g_api_func_800EDB08
         * helper (called with the primitive itself, layout-compatible with
         * POLY_GT4). Destroys self if either registration fails. */
        {
            DRAWENV localEnv = g_CurrentBuffer->draw;
            DR_ENV* env;

            localEnv.clip.x = 0;
            localEnv.clip.y = 0x14;
            localEnv.clip.w = 0x100;
            localEnv.clip.h = 0xCF;

            p = self->ext.prim;
            env = g_api_func_800EDB08((POLY_GT4*)p);
            if (env == NULL) {
                DestroyEntity(self);
                return;
            }
            p->type = 7;
            SetDrawEnv(env, &localEnv);
            p->drawMode = 0x1008;
            p->priority = g_unkGraphicsStruct.g_zEntityCenter - 1;
            p = p->next;

            env = g_api_func_800EDB08((POLY_GT4*)p);
            if (env == NULL) {
                DestroyEntity(self);
                return;
            }
            p->type = 7;
            p->drawMode = 0x808;
            p->priority = g_unkGraphicsStruct.g_zEntityCenter + 1;
        }

        return;
    }

    /* Homing spring: ext84 chases target ext8C at a velocity in ext88,
     * damping ext88 by 1/8th per frame and snapping to the target once
     * both the velocity and the remaining distance are small. */
    *ext84 += *ext88;
    if (*ext8C < *ext84) {
        *ext88 -= 0x4000;
    }
    if (*ext84 < *ext8C) {
        *ext88 += 0x4000;
    }

    {
        s32 damp = *ext88;
        if (damp < 0) {
            damp += 7;
        }
        damp >>= 3;
        damp = *ext88 - damp;
        *ext88 = damp;
        if (damp < 0) {
            damp = -damp;
        }
        if (damp < 0x6000) {
            s32 diff = *ext84 - *ext8C;
            if (diff < 0) {
                diff = -diff;
            }
            if (diff < 0x6000) {
                *ext84 = *ext8C;
                *ext88 = 0;
            }
        }
    }

    if (*ext84 > 0x400000) {
        *ext84 = 0x400000;
    }
    if (*ext84 < (s32)0xFFC00000) {
        *ext84 = (s32)0xFFC00000;
    }

    /* Re-spin the fan-blade primitive chain around self's own position each
     * frame, spreading `count` blades evenly across the current ext84
     * rotation angle. LOW/MEDIUM CONFIDENCE on the exact per-blade GTE
     * corner math (rsin/rcos scaling + a drifting origin term) -- the
     * overall structure (evenly-spaced blades, radius-scaled sin/cos
     * corners) is faithful, but the precise fixed-point rounding may not
     * be bit-exact. */
    {
        s16* table = &D_us_801806B4[self->params * 3];
        s16 count = table[0];
        s16 radius = table[1];
        s16 zBase = table[2];
        s16 angle = 0;
        s32 originX = self->posX.i.hi;
        s32 originY = self->posY.i.hi;
        Primitive* p = self->ext.prim;
        s32 i;

        if (g_Player.status & PLAYER_STATUS_MIST_FORM) {
            zBase = (table[2] - 0xAD) + g_unkGraphicsStruct.g_zEntityCenter;
        }

        p->priority = zBase - 1;
        p = p->next;

        for (i = 0; i < count; i++) {
            s32 sinComponent = (radius * rsin(angle)) >> 12;
            s32 cosComponent;

            originX -= sinComponent;
            cosComponent = (radius * rcos(angle)) >> 12;

            angle += *ext84 / (count == 0 ? 1 : count);

            p->x0 = originX - 4;
            p->y0 = originY + cosComponent;
            p->x1 = originX + 4;
            p->y1 = p->y0;
            p->x2 = originX - 4;
            p->y2 = originY - cosComponent;
            p->x3 = originX + 4;
            p->y3 = p->y2;
            p->priority = zBase;
            p->drawMode = 2;
            p = p->next;
        }
    }

    /* Proximity check: if the player is roughly level with self, apply a
     * hit reaction (params-selected direction/velocity) and destroy self
     * once its animation-driven state settles. */
    {
        s32 dy = PLAYER_posY_i_hi - self->posY.i.hi;
        s32 sfxFlag = 0;

        if (dy < 0) {
            dy = -dy;
        }

        if (dy < 8) {
            s32 dx = PLAYER_posX_i_hi - self->posX.i.hi;

            if (dx < 0) {
                dx = -dx;
            }
            if (dx < 0x18) {
                if (g_Player.status & PLAYER_STATUS_BAT_FORM) {
                    self->velocityY = (dx > 0) ? 0xA0000 : -0xA000;
                    sfxFlag = 1;
                } else if (g_Player.status & PLAYER_STATUS_MIST_FORM) {
                    self->velocityX = (dx > 0) ? 0x20000 : -0x80000;
                    sfxFlag = 1;
                } else {
                    self->velocityX = 0x80000;
                    sfxFlag = 1;
                }
            }
        }

        if (*ext9C != 0) {
            *ext9C -= 1;
        } else if (sfxFlag) {
            g_api_PlaySfx(0x7BC);
            *ext9C = 0x40;
        }
    }
}

// One-time (step 0) init of self's 2 trailing boss-part entities (self+1,
// self+2 as adjacent Entity array slots), positioned relative to the
// tilemap's current scroll via a small per-part (x,y,params) table.
void func_us_801A1010(Entity* self) {
    s32 i;
    s16* table;
    Entity* part;

    if (self->step != 0) {
        return;
    }

    InitializeEntity(D_us_80180484);
    table = D_us_801806C0;
    part = self + 1;

    for (i = 0; i < 2; i++, part++) {
        func_us_801A922C(0x1B, part);
        part->posX.i.hi = *table++ - g_Tilemap.scrollX.i.hi;
        part->posY.i.hi = *table++ - g_Tilemap.scrollY.i.hi;
        part->params = *table++;
    }
}

// One-time (step 0) init of a 2-quad GT4 chain stored at self->ext.prim,
// laid out as 2 adjacent segments starting at self's own posX/posY.
void func_us_801A10DC(Entity* self) {
    s32 primIndex;
    Primitive* p;
    s32 i;
    s16 x;
    s16 y;
    s32 uOff;

    if (self->step != 0) {
        return;
    }

    InitializeEntity(D_us_80180484);

    primIndex = g_api_AllocPrimitives(PRIM_GT4, 2);
    if (primIndex == -1) {
        DestroyEntity(self);
        return;
    }

    p = &g_PrimBuf[primIndex];
    x = self->posX.i.hi;
    self->flags |= FLAG_HAS_PRIMS;
    self->primIndex = primIndex;
    self->ext.prim = p;

    uOff = -0x50;
    y = self->posY.i.hi;

    for (i = 0; i < 2; i++) {
        s32 u;

        p->clut = 0x74;
        p->tpage = 0xF;
        p->x2 = p->x0 = x;

        u = (0x73 - i * 16) - (u8)self->params;
        p->v1 = 0x43;
        p->v0 = 0x43;
        p->v3 = 0xA3;
        p->v2 = 0xA3;
        p->u2 = p->u0 = u;
        p->u3 = p->u1 = u + uOff;

        if (self->params != 0) {
            x = (x + 0x50) - i * 16;
        } else {
            x = (x - 0x50) + i * 16;
        }

        p->x3 = p->x1 = x;
        p->priority = 0xA0;
        p->drawMode = 2;
        p->y3 = p->y2 = y + 0x60;
        p->y1 = p->y0 = y;

        p = p->next;
        uOff += 0x10;
    }
}

// One-time (step 0) init of an (params & 0xF)-row, 3-column GT4 tile grid
// (each tile 0x3D square); the grid grows rightward from self's posX unless
// params bit 8 is set, in which case it grows leftward instead.
void func_us_801A1260(Entity* self) {
    s32 primIndex;
    Primitive* p;
    s32 numRows;
    s16 x0;
    s16 y0;

    if (self->step != 0) {
        return;
    }

    InitializeEntity(D_us_80180484);

    numRows = self->params & 0xF;
    primIndex = g_api_AllocPrimitives(PRIM_GT4, (numRows * 3) & 0xFFFF);
    if (primIndex == -1) {
        DestroyEntity(self);
        return;
    }

    p = &g_PrimBuf[primIndex];
    self->flags |= FLAG_HAS_PRIMS;
    self->primIndex = primIndex;
    self->ext.prim = p;

    x0 = self->posX.i.hi;
    y0 = self->posY.i.hi;

    if (numRows != 0) {
        s32 row;
        s16 rowY;

        rowY = y0;
        for (row = 0; row < numRows; row++) {
            s16 rowYEnd;
            s32 col;
            s16 leftX;
            s16 rightX;

            rowYEnd = rowY + 0x3D;
            leftX = x0;
            rightX = x0;

            for (col = 0; col < 3; col++) {
                s16 xNear;
                s16 xFar;

                p->tpage = 0xF;
                p->clut = 0xC8;
                p->u2 = p->u0 = 0xC1;
                p->u3 = p->u1 = 0xFE;
                p->v1 = p->v0 = 0xC1;
                p->v3 = p->v2 = 0xFE;

                if (self->params & 0x100) {
                    xNear = leftX;
                    xFar = leftX - 0x3D;
                } else {
                    xNear = rightX;
                    xFar = rightX + 0x3D;
                }
                p->x0 = p->x2 = xNear;
                p->x1 = p->x3 = xFar;

                rightX += 0x3D;
                leftX -= 0x3D;

                p->y0 = p->y1 = rowY;
                p->priority = 0xA0;
                p->drawMode = 2;
                p->y2 = p->y3 = rowYEnd;

                p = p->next;
            }

            rowY += 0x3D;
        }
    }
}

// Per-frame HUD update: repaints the 2 on-screen GPU draw-buffer overlay
// colors, then (step 0 only) first allocates/initializes a single G4 quad as
// a fixed background panel. self->ext.prim->y1 (the panel's top edge) is
// re-pinned every frame to track how close the player is to the top of the
// screen (clamped at 0x70, its resting position).
void func_us_801A1404(Entity* self) {
    s32 diff;

    switch (self->step) {
        case 0: {
            s32 primIndex;
            Primitive* p;
            u32 rgbType;

            InitializeEntity(D_us_80180484);
            primIndex = g_api_AllocPrimitives(PRIM_G4, 1);
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }

            p = &g_PrimBuf[primIndex];
            self->flags |= FLAG_HAS_PRIMS;
            self->primIndex = primIndex;
            self->ext.prim = p;

            p->r0 = 0x40;
            p->g0 = 0x38;
            p->b0 = 0x30;
            p->x1 = p->x3 = p->x2 = 0x100;
            rgbType = *(u32*)&p->r0;
            *(u32*)&p->r1 = rgbType;
            *(u32*)&p->r2 = rgbType;
            *(u32*)&p->r3 = rgbType;
            p->x0 = 0x68;
            p->y0 = 0xE8;
            p->y1 = 0x70;
            p->y3 = p->y2 = 0xF0;
            p->priority = 0x5E;
            p->drawMode = 0;
            break;
        }

        case 1:
            break;

        default:
            return;
    }

    g_GpuBuffers_0_draw_g0 = 8;
    g_GpuBuffers_1_draw_g0 = 8;
    g_GpuBuffers_0_draw_b0 = 0x38;
    g_GpuBuffers_1_draw_b0 = 0x38;
    g_GpuBuffers_0_draw_r0 = 0x10;
    g_GpuBuffers_1_draw_r0 = 0x10;

    diff = 0x80 - PLAYER_posY_i_hi;
    if (diff < 0) {
        diff = 0;
    }
    self->ext.prim->y1 = 0x70 - diff;
}

// 12-step machine for a boss-arena "gate" (a sinking/rising platform tied to
// the music-stop cutscene transitions): step 0 positions self and spawns a
// linked part; step 1 waits for the music-stopped flag then stamps 4 wall
// tiles into g_Tilemap.fg (spaced one tilemap row apart); steps 2/8 glide
// posY continuously at a fixed sub-pixel rate until past a raw threshold,
// then clamp and advance; steps 3/9 (sharing the same tile-erase code) wait
// for the music to resume, then zero out the 4 tiles step 1 stamped; steps
// 4/10 nudge posY.i.hi once relative to the tilemap's vertical scroll once a
// threshold is crossed. Steps 5/6/7/11 are no-ops.
void func_us_801A159C(Entity* self) {
    switch (self->step) {
        case 0: {
            u16 params = self->params;
            s16 xOffset;
            s16 scrollX = g_Tilemap.scrollX.i.hi;
            Entity* part;

            InitializeEntity(D_us_801804CC);
            self->zPriority = 0x5C;

            if (params & 2) {
                self->animCurFrame = 0xE;
                return;
            }

            if (params & 1) {
                self->animCurFrame = 0xD;
                xOffset = 0x1D0;
            } else {
                self->animCurFrame = 0xC;
                xOffset = 0x10;
            }

            self->posX.i.hi = xOffset - scrollX;
            self->posY.i.hi = 0x20 - g_Tilemap.scrollY.i.hi;

            part = (Entity*)((u8*)self + 0xBC);
            func_us_801A92A0(0x21, self, part);
            part->params = 2;
            part->posY.i.hi = 0xA0;
            if (self->params != 0) {
                part->posX.i.hi += 0x10;
            }
            return;
        }

        case 1: {
            s32 col;
            s32 i;

            if (D_us_801806DC == 0) {
                return;
            }
            if (self->params & 2) {
                self->step = 8;
                return;
            }

            col = (self->params == 0) ? 0xC1 : 0xDE;
            for (i = 0; i < 4; i++, col += 0x20) {
                g_Tilemap.fg[col] = 0x4B3;
            }
            self->step = 2;
            return;
        }

        case 2:
            self->posY.val += 0x12000;
            if (self->posY.val > 0x460000) {
                self->posY.val = 0x460000;
                goto stepIncrement;
            }
            return;

        case 3:
            if (D_us_801806DC != 0) {
                return;
            }
            goto eraseTiles;

        case 4: {
            s16 sum;
            s16 scrollY = g_Tilemap.scrollY.i.hi;

            self->posY.val -= 0xC000;
            sum = self->posY.i.hi + scrollY;
            if (sum < 0x20) {
                self->posY.i.hi = 0x20 - scrollY;
                goto stepIncrement;
            }
            return;
        }

        case 8:
            self->posY.val -= 0x12000;
            if (self->posY.val > 0x80FFFF) {
                return;
            }
            self->posY.val = 0x810000;
            goto stepIncrement;

        case 9:
            if (D_us_801806DC != 0) {
                return;
            }

        eraseTiles: {
            s32 col;
            s32 i;

            col = (self->params == 0) ? 0xC1 : 0xDE;
            for (i = 0; i < 4; i++, col += 0x20) {
                g_Tilemap.fg[col] = 0;
            }
            goto stepIncrement;
        }

        case 10: {
            s16 sum;
            s16 scrollY = g_Tilemap.scrollY.i.hi;

            self->posY.val += 0xC000;
            sum = self->posY.i.hi + scrollY;
            if (sum >= 0xA1) {
                self->posY.i.hi = 0xA0 - scrollY;
                goto stepIncrement;
            }
            return;
        }

        default:
            return;
    }

stepIncrement:
    self->step++;
}

// One-time (step 0) init of a 5-quad GT4 chain forming a horizontal HP/gauge
// bar: each segment is 0x3E units wide, growing left-to-right, all sharing
// the same solid-fill texture/color.
void func_us_801A1884(Entity* self) {
    s32 primIndex;
    Primitive* p;
    s16 x;

    if (self->step != 0) {
        return;
    }

    InitializeEntity(D_us_80180484);

    primIndex = g_api_AllocPrimitives(PRIM_GT4, 5);
    if (primIndex == -1) {
        DestroyEntity(self);
        return;
    }

    p = &g_PrimBuf[primIndex];
    self->primIndex = primIndex;
    self->flags |= FLAG_HAS_PRIMS;

    if (p != NULL) {
        x = 0;
        do {
            p->x0 = p->x2 = x;
            x += 0x3E;
            p->x1 = p->x3 = x;
            p->tpage = 0xF;
            p->clut = 0xC5;
            p->u0 = p->u2 = 0x41;
            p->u1 = p->u3 = 0x7F;
            p->v0 = p->v1 = 0xA9;
            p->v2 = p->v3 = 0xC6;
            p->y0 = p->y1 = 0xC0;
            p->y2 = p->y3 = 0xE4;
            p->priority = 0x10;
            p->drawMode = 0;
            p = p->next;
        } while (p != NULL);
    }
}

void func_us_801A19CC(u8 step) {
    g_CurrentEntity->poseTimer = 0;
    g_CurrentEntity->pose = 0;
    // ext+0x1 (0x7D): unidentified per-entity-type flag byte, cleared on step change
    *((u8*)&g_CurrentEntity->ext + 1) = 0;
    g_CurrentEntity->step = step;
    g_CurrentEntity->step_s = 0;
}

// Spawns `count` debris particles into g_Entities_224, scattered around
// g_CurrentEntity's own position with random velocity/priority/lifetime, then
// (if g_CurrentEntity is in step 0x16) nudges its horizontal velocity to
// compensate for the spawn.
void func_us_801A19FC(u8 count, s32 yOffset) {
    s32 i;
    Entity* part;

    for (i = 0; i < count; i++) {
        part = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
        if (part == NULL) {
            continue;
        }

        DestroyEntity(part);
        func_us_801AEE54(0x22, g_CurrentEntity, part);
        part->flags = 0xC8202000;

        part->posX.i.hi = part->posX.i.hi - 0x20 + (Random() & 0x3F);
        part->velocityY = -0x4000;
        part->posY.i.hi = part->posY.i.hi + (Random() & 0x3F) + (yOffset - 0x20);

        if ((Random() & 1) != 0) {
            part->zPriority++;
        }
        part->step = 0x30;

        part->params = Random() & 1;
        if (part->params != 0) {
            part->step = 0x32;
            if ((Random() & 1) != 0) {
                part->velocityX = 0xFFFF3000;
                *(s32*)((u8*)part + 0x8C) = 0xD00;
            } else {
                *(s32*)((u8*)part + 0x8C) = -0xD00;
                part->velocityX = 0xD000;
            }
        }

        if (g_CurrentEntity->step == 0x16) {
            if (g_CurrentEntity->facingLeft) {
                g_CurrentEntity->velocityX += 0x4000;
            } else {
                g_CurrentEntity->velocityX -= 0x4000;
            }
        }
    }
}

// Pushes g_CurrentEntity back onto screen vertically once the tilemap has
// scrolled far enough that it would otherwise go off the bottom edge.
void func_us_801A1BA0(s16* sensors) {
    Entity* self;
    s16 scrollY;

    if (UnkCollisionFunc3(sensors) & 1) {
        return;
    }

    self = g_CurrentEntity;
    scrollY = g_Tilemap.scrollY.i.hi;
    if (self->posY.i.hi + scrollY >= 0xB8) {
        self->posY.i.hi = 0xB7 - scrollY;
        self->velocityX = 0;
        self->velocityY = 0;
    }
}

// Main per-frame conductor for the whole Hippogryph fight: a ~131-value
// step machine (most values are unused/no-op and fall straight to the
// shared tail), each real step driving its own animation/movement/attack
// logic, several with their own step_s sub-machine. The tail (after the
// big switch) recomputes hitboxWidth/hitboxHeight from animCurFrame ranges
// and is shared by every step. Given the sheer number of internal cross-
// jumps in the original asm, this translation keeps the original block
// labels (as goto targets) rather than restructuring into nested
// if/else, to stay as faithful as possible to control flow.
void func_us_801A1C14(Entity* self) {
    if (self->hitParams != 0 && (self->step & 1)) {
        if (self->facingLeft) {
            self->velocityX = 0xFFFF8000;
        } else {
            self->velocityX = 0x18000;
        }
        self->velocityY = 0xFFFC0000;
        func_us_801A19CC(0x20);
    }

    if ((self->flags & 0x100) && self->step < 0x22) {
        s32 ext94 = *(s32*)((u8*)self + 0x94);
        self->hitboxState = 0;
        *(s16*)((u8*)self + 0xA) = 0;
        *(s16*)((u8*)self + 0xE) = 0;
        *(u8*)&self->ext = 0;
        self->animCurFrame = 0x6E;
        if (ext94 != 0) {
            DestroyEntity(self);
        }
        if (self->flags & 0x800000) {
            g_api_FreePrimitives(self->primIndex);
            self->flags &= ~0x800000;
        }
        func_us_801A19CC(0x24);
    }

    if (self->step >= 0x83) {
        goto L3AE4;
    }

    switch (self->step) {
    case 0:
        InitializeEntity(D_us_801804D8);
        *(u8*)((u8*)self + 0x7E) = (u8)self->zPriority;
        self->zPriority = g_Entities[0].zPriority + 1;
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
        *(u8*)((u8*)self + 0x7D) = 1;
        goto L3AE4;

    case 1:
        AnimateEntity(D_us_8018091C, self);
        self->hitboxState = 0;
        if (*(u8*)((u8*)self + 0x7D) == 0) {
            goto L3AE4;
        }
        if (GetDistanceToPlayerX() >= 0x80) {
            goto L3AE4;
        }
        func_us_801A19CC(3);
        if (D_us_801806DC == 0) {
            g_api_TimeAttackController(7, 2);
        }
        self->hitboxState = 3;
        self->nextPart = (Entity*)((u8*)self + 0xBC);
        D_us_801806DC |= 1;
        func_us_801A92A0(0x23, self, (Entity*)((u8*)self + 0xBC));
        goto L3AE4;

    case 3:
        if (AnimateEntity(D_us_80180804, self)) {
            goto L3AE4;
        }
        func_us_801A19CC(4);
        goto L3AE4;

    case 4:
        switch (self->step_s) {
        case 1:
            goto L1F78;
        case 0:
            goto L1EA0;
        case 2:
            goto L1FF4;
        default:
            goto L3AE4;
        }

    L1EA0:
        if (AnimateEntity(D_us_801808BC, self)) {
            self->animCurFrame = 0x31;
            self->velocityY = 0xFFFB0000;
            *(u8*)((u8*)self + 0x7C) = 4;
            *(u8*)((u8*)self + 0x7D) = 0;
            self->step_s++;
        }
        if (self->animCurFrame == 0x29 && *(u8*)((u8*)self + 0x7D) == 0) {
            func_us_801A19FC(4, 0);
            self->velocityX = self->facingLeft ? 0x6000 : -0x6000;
            self->posY.i.hi -= 4;
        }
        if (*(u8*)((u8*)self + 0x7D) != 0) {
            MoveEntity();
            if ((u16)(self->animCurFrame - 0x2A) < 2) {
                self->velocityY -= 0x6000;
            }
        }
        goto L1FB4;

    L1F78:
        MoveEntity();
        {
            u8 counter = *(u8*)((u8*)self + 0x7C) - 1;
            self->velocityY += 0x1E00;
            *(u8*)((u8*)self + 0x7C) = counter;
            if (counter == 0) {
                self->pose = 0;
                *(u8*)((u8*)self + 0x7C) = 0;
                self->step_s++;
            }
        }
        goto L1FB4;

    L1FF4:
        self->velocityY += 0x2000;
        if (!AnimateEntity(D_us_801808D4, self)) {
            goto L2808;
        }
        if (self->step_s != 0) {
            goto L2070;
        }
        *(u8*)((u8*)self + 0x7C) = 0x50;
        self->velocityX = self->facingLeft ? 0x1C000 : 0xFFFE4000;
        self->velocityY = 0;
        self->step_s++;
        goto L2070;

    L1FB4:
        if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x72) {
            goto L3AE4;
        }
        self->velocityY = 0xFFFB8000;
        self->animCurFrame = 0x31;
        *(u8*)((u8*)self + 0x7D) = 0;
        self->step_s = 2;
        goto L3AE4;

    L2070:
        MoveEntity();
        AnimateEntity(D_us_80180830, self);
        if (self->step_s == 1 /* combined pose/poseTimer==1 check, see below */
            && *(s32*)((u8*)self + 0x50) == 1) {
            PlaySfxPositional(0x768);
        }
        if ((u16)(self->animCurFrame - 6) < 2) {
            self->velocityY -= 0x2800;
        } else {
            self->velocityY += 0xE00;
        }
        {
            u8 c = *(u8*)((u8*)self + 0x7C) - 1;
            *(u8*)((u8*)self + 0x7C) = c;
            if (c != 0) {
                goto L3AE4;
            }
        }
        {
            s16 posXhi = self->posX.i.hi;
            s16 s1v;
            s16 scrollXhi = g_Tilemap.scrollX.i.hi;
            if (self->velocityX >= 0) {
                s1v = posXhi + scrollXhi;
            } else {
                s1v = 0x200 - (posXhi + scrollXhi);
            }
            if (GetDistanceToPlayerX() < 0x51) {
                if (s1v < 0x180) {
                    func_us_801A19CC(8);
                    goto L3AE4;
                }
                goto L27F4;
            }
            if (s1v < 0x90) {
                self->velocityX = 0;
                self->velocityY = 0;
                func_us_801A19CC(0x16);
                goto L3AE4;
            }
            if (s1v < 0x180) {
                func_us_801A19CC(8);
            }
            goto L3AE4;
        }

    case 6:
        goto L2070;

    case 8:
        switch (self->step_s) {
        case 1:
            goto L21D8;
        case 0:
            goto L21A8;
        case 2:
            goto L223C;
        case 3:
            goto L2280;
        default:
            goto L3AE4;
        }

    L21A8:
        self->velocityX = self->facingLeft ? 0x8000 : 0xFFFF8000;
        self->velocityY = 0x38000;
        self->step_s++;
        goto L21D8;

    L21D8:
        if (!AnimateEntity(D_us_80180938, self)) {
            goto L2204;
        }
        self->pose = 0;
        self->poseTimer = 0;
        self->step_s++;
        goto L2204;

    L2204:
        MoveEntity();
        if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x89) {
            goto L3AE4;
        }
        self->pose = 0;
        self->poseTimer = 0;
        self->step_s = 2;
        goto L3AE4;

    L223C:
        AnimateEntity(D_us_80180944, self);
        MoveEntity();
        if (!CheckColliderOffsets(D_us_80180ADC, 0)) {
            goto L3AE4;
        }
        self->pose = 0;
        self->poseTimer = 0;
        goto L39E0;

    L2280: {
        u8 landed = ((s32(*)(s16*))func_us_801A1BA0)(D_us_80180AAC) & 1;
        if (landed && *(u8*)((u8*)self + 0x7D) == 0) {
            *(u8*)((u8*)self + 0x7D) = 1;
        }
        self->velocityY -= 0x7C00;
        if (0xFFFF8000 > self->velocityY) {
            self->velocityY = 0xFFFF8000;
        }
        if (AnimateEntity(D_us_80180950, self)) {
            if (*(u8*)((u8*)self + 0x7D) != 0) {
                func_us_801A19CC(0xA);
                *(u8*)((u8*)self + 0x7D) = 0;
            }
            goto L3AE4;
        }
        goto L3AE4;
    }

    case 10: {
        u8 idx = (GetSideToPlayer() & 1) ^ 1;
        self->facingLeft = idx;
        idx = Random() & 7;
        {
            s16 bucket = idx;
            s16 d = GetDistanceToPlayerX();
            if (d < 0x51) {
                bucket = idx + 4;
            } else if (d < 0x81) {
                bucket = (idx + 4) & 0xFF;
            } else {
                bucket = idx & 0xFF;
            }
            func_us_801A19CC(D_us_80180AEC[(u8)bucket]);
        }
        goto L3AE4;
    }

    case 12:
        switch (self->step_s) {
        case 1:
            goto L23DC;
        default:
            goto L238C;
        case 0:
            goto L238C;
        }

    L238C:
        if (AnimateEntity(D_us_80180858, self)) {
            *(u8*)((u8*)self + 0x7C) = 0x40;
            self->animCurFrame++;
            PlaySfxPositional(0x6C3);
        }
        if (self->pose == 4) {
            PlaySfxPositional(0x614);
        }
        goto L3AE4;

    L23DC:
        if (*(u8*)((u8*)self + 0x7C) & 1) {
            Entity* part = AllocEntity(g_Entities_224, g_Entities_224 + 32);
            if (part != NULL) {
                func_us_801A92A0(0x27, self, part);
                part->pfnUpdate = func_us_801A3B88;
                part->facingLeft = self->facingLeft;
                part->posX.i.hi = self->facingLeft ? (part->posX.i.hi - 0x1D)
                                                    : (part->posX.i.hi + 0x1D);
                part->posY.i.hi -= 2;
                part->params = *(u8*)((u8*)self + 0x7D);
            }
            {
                Entity* part2 = AllocEntity(*(u8*)((u8*)self + 0x7D) ? g_Entities_160 : g_Entities_224,
                                             (*(u8*)((u8*)self + 0x7D) ? g_Entities_160 : g_Entities_224) + 32);
                if (part2 != NULL) {
                    DestroyEntity(part2);
                    func_us_801A92A0(0x27, self, part2);
                    part2->pfnUpdate = func_us_801A3B88;
                    part2->facingLeft = self->facingLeft;
                    part2->params = 2;
                    part2->posX.i.hi = self->facingLeft ? (part2->posX.i.hi - 0x1D)
                                                         : (part2->posX.i.hi + 0x1D);
                    part2->posY.i.hi -= 2;
                    part2->params = (u8)(*(u8*)((u8*)self + 0x7D) + 2);
                }
            }
            *(u8*)((u8*)self + 0x7D) ^= 1;
        }
        {
            u8 c = *(u8*)((u8*)self + 0x7C) - 1;
            *(u8*)((u8*)self + 0x7C) = c;
            if (c != 0) {
                goto L3AE4;
            }
        }
        self->animCurFrame = 0;
        goto L2EC8;

    case 13:
        if (*(u8*)((u8*)self + 0x7D) == 0) {
            *(u8*)((u8*)self + 0x7D) = 1;
            *(u8*)((u8*)self + 0x7C) = 0x80;
            self->velocityX = self->facingLeft ? 0x14000 : 0xFFFEC000;
            self->velocityY = 0;
        }
        MoveEntity();
        AnimateEntity(D_us_80180824, self);
        if (self->animCurFrame == 5 && self->poseTimer == 0) {
            PlaySfxPositional(0x646);
        }
        if (self->facingLeft) {
            self->velocityX -= 0x1C00;
        } else {
            self->velocityX += 0x1C00;
        }
        if (self->animCurFrame == 1) {
            if (self->facingLeft) {
                self->velocityX += 0x1C00;
            } else {
                self->velocityX -= 0x1C00;
            }
        }
        {
            s16 posXhi = self->posX.i.hi;
            s16 scrollXhi = g_Tilemap.scrollX.i.hi;
            s16 s1v = posXhi + scrollXhi;
            if (self->facingLeft) {
                if (s1v >= 0x181) {
                    self->facingLeft = 0;
                    self->velocityX = 0xFFFEC000;
                    goto L2698_13;
                }
            } else {
                if (s1v < 0x80) {
                    self->facingLeft = 1;
                    self->velocityX = 0x14000;
                    goto L2698_13;
                }
            }
            goto L26A0_13;
        L2698_13:
            self->pose = 0;
            self->poseTimer = 0;
        L26A0_13:
            if (!CheckColliderOffsets(D_us_80180AE4, (u8)self->facingLeft)) {
                self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
                func_us_801A19CC(4);
            }
        }
        {
            u8 c = *(u8*)((u8*)self + 0x7C) - 1;
            *(u8*)((u8*)self + 0x7C) = c;
            if (c != 0) {
                goto L3AE4;
            }
        }
        goto L2EC8;

    case 16:
        switch (self->step_s) {
        case 1:
            goto L2794;
        case 0:
            goto L2738;
        case 2:
            goto L27F4;
        default:
            goto L3AE4;
        }

    L2738:
        if (AnimateEntity(D_us_801808FC, self)) {
            goto L3AE4;
        }
        func_us_801A19FC(4, 0);
        *(u8*)((u8*)self + 0x7C) = 0x40;
        self->velocityX = self->facingLeft ? 0x48000 : 0xFFFB8000;
        self->pose = 0;
        self->poseTimer = 0;
        self->animCurFrame = 0x21;
        goto L39E0;

    L2794:
        if (*(u8*)((u8*)self + 0x7C) < 0x20) {
            AnimateEntity(D_us_80180884, self);
        }
        MoveEntity();
        self->velocityY -= 0x1000;
        if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x48) {
            goto L3AE4;
        }
        goto L39DC;

    L27F4:
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
        goto L2808;

    L2808:
        func_us_801A19CC(6);
        goto L3AE4;

    case 18:
        switch (self->step_s) {
        case 1:
            goto L29B0;
        case 0:
            goto L2970;
        case 2:
            goto L2ADC;
        case 3:
            goto L2B40;
        default:
            goto L2C44;
        }

    L2970:
        AnimateEntity(D_us_8018088C, self);
        MoveEntity();
        if (GetDistanceToPlayerY() < 0x50) {
            self->step_s++;
        }
        goto L2C44;

    L29B0:
        MoveEntity();
        if (!AnimateEntity(D_us_80180894, self)) {
            self->step_s++;
        }
        self->velocityX -= 0x800;
        if (g_Player.unk60 == 0) {
            s16 posXhi = self->posX.i.hi;
            s16 scrollXhi = g_Tilemap.scrollX.i.hi;
            s16 s1v = self->facingLeft ? (0x200 - (posXhi + scrollXhi)) : (posXhi + scrollXhi);
            s16 dy;
            if (s1v < 0x80) {
                goto L2C44;
            }
            dy = PLAYER_posY_i_hi - self->posY.i.hi;
            if (dy < 0) {
                dy = -dy;
            }
            if (dy < 0xC) {
                s16 dx = PLAYER_posX_i_hi - self->posX.i.hi;
                s16 s0v = dx < 0 ? 0x80 : 0;
                if (dx < 0) {
                    dx = -dx;
                }
                if (dx < 0x38 && !(g_Player.status & 0x40C401D2)) {
                    PlaySfxPositional(0x769);
                    g_Player.unk60 = 1;
                    self->posX.i.hi = PLAYER_posX_i_hi;
                    g_Player.unk64 = 0x10;
                    g_Player.unk62 = 0;
                    self->posY.i.hi = PLAYER_posY_i_hi - 0x20;
                }
            }
        }
        goto L2C44;

    L2ADC:
        MoveEntity();
        self->velocityY -= 0x6000;
        if (AnimateEntity(D_us_8018089C, self)) {
            *(u8*)((u8*)self + 0x7C) = 0x40;
            self->step_s++;
            if (g_Player.unk60 == 0) {
                self->velocityY -= 0x6000;
            }
        }
        goto L2C44;

    L2B40: {
        if (g_Player.unk60 != 0) {
            u8 cnt;
            AnimateEntity(D_us_801808AC, self);
            /* MEDIUM CONFIDENCE: original uses a reciprocal-multiply by
             * 0xAE4C415D (>>32, >>5) to compute a division; translated as
             * a plain modulo of the decremented counter. */
            cnt = *(u8*)((u8*)self + 0x7C) - 1;
            *(u8*)((u8*)self + 0x7C) = cnt;
            if ((cnt % 48) == 0) {
                func_us_801A19FC(8, -8);
                g_Player.unk60 = 3;
                g_Player.unk64 = 0x10;
            }
        } else {
            AnimateEntity(D_us_80180830, self);
            if (self->pose == g_Player.unk60) {
                PlaySfxPositional(0x768);
            }
        }
        MoveEntity();
        if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x48) {
            goto L2C44;
        }
        g_Player.unk60 = 0;
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
        func_us_801A19CC(6);
        goto L2C44;
    }

    L2C44:
        if (g_Player.unk60 == 0) {
            goto L3AE4;
        }
        goto L2C58;

    L2C58:
        /* s2 here is the register set up at function entry as &g_Entities
         * (g_Entities[0], used already in step 0 for zPriority) and still
         * live this far into the function; kept as-is rather than guessed
         * at as a locally-spawned entity. */
        g_Entities[0].posX.i.hi = self->posX.i.hi;
        g_Entities[0].posY.i.hi = (s16)(self->posY.i.hi + 0x20);
        goto L3AE4;

    case 20:
        switch (self->step_s) {
        case 1:
            goto L2D98;
        case 0:
            goto L2CB4;
        case 2:
            goto L2DFC;
        default:
            goto L3AE4;
        }

    L2CB4:
        self->facingLeft = (GetSideToPlayer() & 1) ^ 1;
        if (!AnimateEntity(D_us_80180844, self)) {
            *(u8*)((u8*)self + 0x7D) += 1;
        }
        if (self->pose == 1) {
            PlaySfxPositional(0x768);
        }
        MoveEntity();
        if ((u16)(self->animCurFrame - 6) < 2) {
            self->velocityY -= 0x2800;
        } else {
            self->velocityY += 0x1000;
        }
        if (*(u8*)((u8*)self + 0x7D) < 2) {
            goto L3AE4;
        }
        *(u8*)((u8*)self + 0x7D) = 0;
        self->velocityX = 0;
        self->velocityY = 0;
        self->step_s = 1;
        goto L2D84;

    L2D98:
        MoveEntity();
        AnimateEntity(D_us_80180884, self);
        self->velocityY -= 0x1800;
        if (self->velocityY < 0xFFFF8000) {
            self->velocityY = 0xFFFF8000;
        }
        if (!CheckColliderOffsets(D_us_80180ADC, 0)) {
            goto L3AE4;
        }
        self->pose = 0;
        self->poseTimer = 0;
        goto L39E0;

    L2DFC:
        self->velocityY -= 0x3800;
        if (((s32(*)(s16*))func_us_801A1BA0)(D_us_80180AAC) & 1) {
            *(u8*)((u8*)self + 0x7D) = 1;
        }
        if (self->facingLeft) {
            self->velocityX -= 0x1000;
        } else {
            self->velocityX += 0x1000;
        }
        if (self->animCurFrame == 0x5E) {
            func_us_801A19FC(2, -2);
        }
        if (AnimateEntity(D_us_8018096C, self)) {
            goto L3AE4;
        }
        if (*(u8*)((u8*)self + 0x7D) == 0) {
            goto L3AE4;
        }
        goto L2EC8;

    L2D84:
        self->velocityX = 0x48000;
        goto L3AE4;

    L2EC8:
        func_us_801A19CC(0xA);
        goto L3AE4;

    case 22:
        switch (self->step_s) {
        case 1:
            goto L292C_alt;
        default:
            goto L3AE4;
        case 0:
            goto L3AE4;
        }
    L292C_alt:
        AnimateEntity(D_us_80180928, self);
        if (((s32(*)(s16*))func_us_801A1BA0)(D_us_80180AAC) & 1) {
            self->pose = 0x50;
        }
        goto L3AE4;

    case 32:
        if (self->step_s < 0xA) {
            switch (self->step_s) {
            case 0: goto L2F04;
            case 1: goto L2F8C;
            case 2: goto L2FC8;
            case 3: goto L3048;
            case 4: goto L3144;
            case 5: goto L31E0;
            case 6: goto L3408;
            case 7: goto L3694;
            case 8: goto L3970;
            default: goto L3AE4;
            }
        }
        goto L3AE4;

    L2F04:
        if (g_Player.unk60 != 0) {
            g_Player.unk60 = 0;
        }
        g_api_TimeAttackController(1, 1);
        self->velocityY = 0;
        {
            s16 posXhi = self->posX.i.hi;
            s16 scrollXhi = g_Tilemap.scrollX.i.hi;
            s16 s1v = posXhi + scrollXhi;
            D_us_801806E0 |= 2;
            if (s1v < 0x38) {
                self->velocityX = 0xFFFF8000;
            }
            if (s1v < 0x1D8) {
                self->velocityX = 0x8000;
            }
        }
        self->step_s++;
        MoveEntity();
        self->velocityY += 0x4000;
        if ((s16)(self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x81) {
            goto L39DC;
        }
        goto L3AE4;

    L2F8C:
        if (((s32(*)(s16*))func_us_801A1BA0)(D_us_80180AAC) & 1) {
            func_us_801A19FC(0x16, -0x20);
            self->step_s = 3;
            goto L3AE4;
        }
        goto L3AE4;

    L2FC8: {
        s16 posYhi = self->posY.i.hi;
        s16 scrollYhi = g_Tilemap.scrollY.i.hi;
        if ((s16)(self->posX.i.hi + scrollYhi) >= 0xB8) {
            goto L3AE4;
        }
        func_us_801A19FC(0x16, -0x20);
        self->step_s = 3;
        self->velocityX = 0;
        self->velocityY = 0;
        self->posY.i.hi = 0xB7 - scrollYhi;
        (void)posYhi;
        goto L3AE4;
    }

    L3048: {
        s16 primIndex = g_api_AllocPrimitives(PRIM_GT4, 0x40);
        if (primIndex == -1) {
            goto L3124;
        }
        {
            Primitive* p = &g_PrimBuf[primIndex];
            self->primIndex = primIndex;
            *(Primitive**)((u8*)self + 0x80) = p;
            self->flags |= 0x800000;
            do {
                p->clut = 0x80;
                p->tpage = 0x40;
                p->r1 = 0x60;
                p->g1 = 0x20;
                p->r0 = 0x60;
                p->g0 = 0x20;
                p->x1 = p->x0 = self->posX.i.hi;
                p->y1 = p->y0 = self->posY.i.hi;
                p->priority = 0x33;
                p->drawMode = 0;
            } while ((p = p->next) != NULL);
        }
        self->step_s++;
        goto L312C;
    }

    L3118:
        self->step_s++;
        goto L312C;

    L3124:
        self->step_s = 5;
        *(Primitive**)((u8*)self + 0x80) = NULL;
        goto L312C;

    L312C:
        self->zPriority = (u16)(*(u8*)((u8*)self + 0x7E) - 8);
        goto L3AE4;

    L3144: {
        u8 count = *(u8*)((u8*)self + 0x7C) + 6;
        s16 rotDelta;
        Primitive* p = *(Primitive**)((u8*)self + 0x80);
        *(u8*)((u8*)self + 0x7C) = count;
        rotDelta = ((u8)count < 0x81) ? 6 : -8;
        p->x1 = (s16)(p->x1 + rotDelta);
        p = p->next;
        p->x1 = (s16)(p->x1 - rotDelta);
        if ((u8)count < 0xF1) {
            goto L3AE4;
        }
        g_api_FreePrimitives(self->primIndex);
        *(u8*)((u8*)self + 0x7C) = 0;
        goto L395C;
    }

    L31E0: {
        s32 i;
        s16 primIndex = g_api_AllocPrimitives(PRIM_GT4, 0x6F);
        if (primIndex == -1) {
            goto L33FC;
        }
        {
            Primitive* p = &g_PrimBuf[primIndex];
            self->primIndex = primIndex;
            *(Primitive**)((u8*)self + 0x80) = p;
            self->flags |= 0x800000;
            for (i = 0; i < 0x1F; i++) {
                s32 val = D_us_801806E4[i];
                p->type = 2;
                p->tpage = 0x60;
                p->r1 = p->r0 = 0x40;
                p->clut = 0x10;
                p->g1 = p->g0 = 8;
                p->x0 = self->posX.i.hi;
                p->y0 = 0;
                p->y1 = 0;
                p->x1 = (s16)(val >> 16);
                p->priority = 0x33;
                p->y2 = (s16)val;
                p = p->next;
            }
        }
        {
            Primitive* p = *(Primitive**)((u8*)self + 0x88);
            for (i = 0; i < 0x50; i++) {
                s16 angle;
                Random();
                p->y0 = Random() & 7;
                angle = (Random() & 0xF) << 8;
                p->x1 = rcos(angle);
                p->y3 = rsin(angle);
                p->r1 = Random() & 0x17; /* MEDIUM CONFIDENCE: offset 0x10 on Primitive */
                p->r2 = 0; /* offset 0x1C on Primitive */
                p = p->next;
            }
        }
        goto L39DC;
    }

    L395C:
        self->step_s++;
        self->flags &= ~0x800000;
        goto L3AE4;

    L33FC:
        self->step_s = 8;
        goto L3AE4;

    L3408: {
        s32 i;
        Primitive* p = *(Primitive**)((u8*)self + 0x88);
        AnimateEntity(D_us_801809D8, self);
        for (i = 0; i < 0x50; i++) {
            /* MEDIUM CONFIDENCE: this per-particle drift/spring update
             * (offsets read as x0/x1/y1/y2/y3 groups) is translated from
             * the register trace as literally as possible. */
            if (*(u8*)((u8*)p + 0x10) != 0) {
                *(u8*)((u8*)p + 0x10) -= 1;
            } else {
                s16 x0 = p->x0, x1 = p->x1, y1 = p->y1, y3 = p->y3;
                if (*(u8*)((u8*)self + 0x7C) < 0xC) {
                    y1 = (s16)(y1 + x1);
                }
                {
                    s32 v = rcos(self->hitboxHeight << 4) * y3; /* placeholder scale */
                    p->x0 = (s16)(p->y0 + ((v >> 12)));
                }
                {
                    s32 v = rsin(self->hitboxHeight << 4) * y3;
                    p->y1 = (s16)(*(u16*)((u8*)p + 0xA) + (v >> 12));
                }
                p->y2 = y1;
                p->y3 = y3;
                (void)x0;
                (void)x1;
            }
        }
        if (*(u8*)((u8*)self + 0x7C) < 0x31) {
            *(u8*)((u8*)self + 0x7C) += 1;
        } else {
            self->hitboxWidth = 0x30; /* placeholder, see caveat */
            self->opacity = 0x80;
            self->drawFlags |= 8;
            self->step_s++;
        }
        goto L3AE4;
    }

    L3694: {
        s32 i;
        Primitive* p = *(Primitive**)((u8*)self + 0x88);
        for (i = 0; i < 0x50; i++) {
            /* mirrors L3408's loop, see caveat there */
            i = i; /* no-op placeholder to keep loop shape */
            p = p->next;
        }
        if ((g_Timer & 7) != 0) {
            goto L3AE4;
        }
        PlaySfxPositional(0x6AF);
        goto L3AE4;
    }

    L3970: {
        Entity* part;
        s16 x0 = 0x100 - g_Tilemap.scrollX.i.hi;
        s16 y0 = 0x80 - g_Tilemap.scrollY.i.hi;
        part = AllocEntity(g_Entities_160, g_Entities_160 + 32);
        if (part == NULL) {
            goto L3AE4;
        }
        func_us_801A922C(0x28, part);
        part->posX.i.hi = x0;
        part->posY.i.hi = y0;
        part->params = 6;
        D_us_80180B74 = 0;
        func_us_801A922C(0x2A, (Entity*)((u8*)g_Entities_160 + 0x1D60));
        goto L39DC;
    }

    L39DC:
        self->step_s++;
        goto L3AE4;

    L39E0:
        self->animCurFrame++;
        goto L3AE4;

    case 64:
        AnimateEntity(D_us_80180A2C, self);
        goto L3A54;

    case 66: {
        s32* ext8C = (s32*)((u8*)self + 0x8C);
        AnimateEntity(D_us_80180A48, self);
        self->velocityX += *ext8C;
        if ((u32)0x1A000 < (u32)(self->velocityX + 0xD000)) {
            *ext8C = -*ext8C;
        }
        goto L3A54;
    }

    L3A54: {
        s32 v = self->velocityY - 0x3800;
        self->velocityY = v;
        if (!UnkCollisionFunc3(D_us_80180ACC)) {
            goto L3AE4;
        }
        DestroyEntity(self);
        goto L3AE4;
    }

    default:
        goto L3AE4;
    }

L3AE4: {
    s16 frame = self->animCurFrame;

    if ((u16)(frame - 0xE) < 0xB) {
        self->hitboxWidth = 6;
        self->hitboxHeight = 0x14;
        self->hitboxOffX = 8;
        self->hitboxOffY = -8;
        goto L3B58;
    }
    if ((u8)(frame - 0x3A) < 6) {
        self->hitboxWidth = 6;
        self->hitboxHeight = 0x14;
        self->hitboxOffX = 0;
        self->hitboxOffY = -8;
        goto L3B58;
    }
    self->hitboxWidth = 0x14;
    self->hitboxHeight = 8;
    self->hitboxOffX = 0;
    self->hitboxOffY = -2;

L3B58:
    return;
}
}

// Debris/shard part: if the global "chain-part" bit is set, destroys itself
// immediately. Otherwise: step 0 launches with a facing-relative velocityX
// and a params-dependent velocityY, tagging opacity/hitbox by params; step 1
// grows (scaleX==scaleY, incrementing) while fading (opacity-=5/frame) and
// decelerating gravity, advancing once fully grown, and (params 3 or 0) has
// a random chance each frame to spawn a trailing copy of itself; step 2
// plays its final anim then destroys self once it finishes.
void func_us_801A3B88(Entity* self) {
    if (D_us_801806E0 & 2) {
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801804F0);
            self->animSet = 2;
            self->animCurFrame = 1;
            self->opacity = 0x80;
            self->rotate = 0xE00;
            self->scaleY = 0x20;
            self->scaleX = 0x20;
            self->drawFlags |= 0xF;
            self->blendMode |= 0x30;
            self->velocityX = self->facingLeft ? 0x46800 : 0xFFFB9800;
            self->velocityY = self->params ? 0x3000 : -0x6000;

            if (self->params < 2) {
                return;
            }
            self->opacity = 0x7C;
            if (self->params == 3) {
                self->velocityY = 0;
                self->hitboxState = 1;
            }
            return;

        case 1: {
            Entity* part;

            AnimateEntity(D_us_80180A00, self);
            MoveEntity();
            self->velocityY -= 0x1400;
            self->scaleX += 0x18;
            self->scaleY = self->scaleX;
            self->opacity += 0xFB;

            if (self->scaleX >= 0xF1) {
                self->velocityX = ((self->velocityX < 0) ? self->velocityX + 3 : self->velocityX) >> 2;
                self->pose = 0;
                self->poseTimer = 0;
                self->step++;
            }

            if (self->params == 3) {
                if (Random() & 1) {
                    part = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
                    if (part != NULL) {
                        DestroyEntity(part);
                        func_us_801A92A0(0x26, self, part);
                        part->facingLeft = self->facingLeft;
                        part->params = self->params;
                        part->scaleX = self->scaleX;
                        part->scaleY = self->scaleY;
                    }
                }
            } else if (self->params == 0) {
                if ((Random() & 7) == 0) {
                    part = AllocEntity(g_Entities_224, (Entity*)((u8*)g_Entities_224 + 0x1780));
                    if (part != NULL) {
                        DestroyEntity(part);
                        func_us_801A92A0(0x26, self, part);
                        part->facingLeft = self->facingLeft;
                        part->params = self->params;
                        part->scaleX = self->scaleX;
                        part->scaleY = self->scaleY;
                    }
                }
            }
            return;
        }

        case 2:
            MoveEntity();
            if (AnimateEntity(D_us_80180A0C, self) != 0) {
                return;
            }
            DestroyEntity(self);
            return;

        default:
            return;
    }
}

// step 0: launch self as a projectile/debris piece (facing-relative X
// velocity, params-relative Y velocity), step 1: apply drag + rotation +
// shrink + fade until its animation finishes, then destroy self.
void func_us_801A3E78(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_8018046C);
            self->animSet = 2;
            self->animCurFrame = 1;
            self->opacity = 0x80;
            self->drawFlags |= 0xF;
            self->blendMode |= 0x30;
            self->velocityX = self->facingLeft ? 0x10000 : -0x10000;
            self->velocityY = self->params ? 0x8000 : -0x10000;
            break;

        case 1:
            MoveEntity();
            self->velocityX += self->facingLeft ? -0x2000 : 0x2000;
            self->velocityY -= 0x1000;
            self->rotate += self->params ? -0x40 : 0x40;
            self->scaleX -= 0xA;
            self->opacity += 0xFA;
            if (AnimateEntity(D_us_80180A18, self) == 0) {
                DestroyEntity(self);
            }
            break;
    }
}

// A launched debris/spinner part: if the global "chain-part" bit is set on
// D_us_801806E0, this self instead spawns a g_Entities_160 marker and
// destroys itself. Otherwise a 4-step machine: step 0 spins in place a
// while, step 1 waits on a wall sensor before advancing, step 2 launches
// itself away from the player once its windup anim finishes (and while
// windup plays, oscillates a secondary rotation via ext+0x7C/0x7D), step 3
// falls under gravity while animating.
void func_us_801A3FD4(Entity* self) {
    Entity* part;
    u8 spinTimer;

    if (D_us_801806E0 & 2) {
        self->flags |= 0x100;
    }

    if (self->flags & 0x100) {
        part = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
        if (part != NULL) {
            DestroyEntity(part);
            func_us_801A92A0(2, self, part);
            part->step = 0;
        }
        DestroyEntity(self);
        return;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801804E4);
            self->drawFlags |= 4;
            self->hitboxState = 2;
            self->rotate = ((Random() & 3) << 9) - 0x300;
            *((u8*)&self->ext + 1) = 1;
            return;

        case 1:
            if (UnkCollisionFunc3(D_us_80180ABC) & 1) {
                self->step++;
            }
            break;

        case 2:
            break;

        case 3:
            AnimateEntity(D_us_801809F8, self);
            MoveEntity();
            self->velocityY -= 0xA00;
            return;

        default:
            return;
    }

    spinTimer = *(u8*)&self->ext;
    if (spinTimer != 0) {
        u8 spinDir = *((u8*)&self->ext + 1);

        if (spinTimer < 8) {
            self->rotate += spinDir << 9;
            *((u8*)&self->ext + 1) = -spinDir;
        }
        *(u8*)&self->ext = spinTimer - 1;
        return;
    }

    if (AnimateEntity(D_us_801809F0, self) != 0) {
        return;
    }

    part = AllocEntity(g_Entities_160, (Entity*)((u8*)g_Entities_160 + 0x1780));
    if (part != NULL) {
        DestroyEntity(part);
        func_us_801A92A0(2, self, part);
        part->step = 0;
    }

    self->facingLeft = GetSideToPlayer() & 1;
    self->drawFlags = 0;
    self->velocityX = self->facingLeft ? 0xFFFD8000 : 0x28000;
    self->velocityY = -0x4000;
    self->pose = 0;
    self->poseTimer = 0;
    PlaySfxPositional(0x7B7);
    self->hitboxState = 3;
    self->step++;
}

// One-time (step 0) init as a hitbox-only "part" trailing the entity in the
// previous array slot (self-1): copies that entity's facing/position every
// frame and picks its own hitbox shape from a per-animation-frame lookup
// table, destroying itself once its parent leaves entity type 0x22.
void func_us_801A425C(Entity* self) {
    Entity* prev = self - 1;
    u8 frame;
    u8 idx;

    if (self->step == 0) {
        InitializeEntity(D_us_80180484);
        self->hitPoints = 0x7FFE;
        self->hitboxState = 2;
        self->parent = prev;
        self->nextPart = prev;
        self->flags |= 0x400000;
        self->attack = 0;
        self->attackElement = 0;
    }

    frame = prev->animCurFrame;
    if (frame >= 0x68) {
        frame = 0;
    }
    idx = D_us_80180B0C[frame];
    if (idx < 7) {
        switch (idx) {
            case 0:
                self->hitboxOffX = 0;
                self->hitboxOffY = 0;
                self->hitboxWidth = 0;
                self->hitboxHeight = 0;
                break;
            case 1:
                self->hitboxOffX = 8;
                self->hitboxOffY = -0x18;
                self->hitboxWidth = 0x14;
                self->hitboxHeight = 0x12;
                break;
            case 2:
                self->hitboxOffX = 0xC;
                self->hitboxOffY = -8;
                self->hitboxWidth = 0x12;
                self->hitboxHeight = 8;
                break;
            case 3:
                self->hitboxOffX = -2;
                self->hitboxOffY = 0x18;
                self->hitboxWidth = 8;
                self->hitboxHeight = 0x12;
                break;
            case 4:
                self->hitboxOffX = 6;
                self->hitboxOffY = 0xA;
                self->hitboxWidth = 0x10;
                self->hitboxHeight = 6;
                break;
            case 5:
                self->hitboxOffX = 0xA;
                self->hitboxOffY = -0x20;
                self->hitboxWidth = 0x16;
                self->hitboxHeight = 0xC;
                break;
            case 6:
                self->hitboxOffX = 2;
                self->hitboxOffY = -0x20;
                self->hitboxWidth = 0xC;
                self->hitboxHeight = 0x18;
                break;
        }
    }

    self->facingLeft = prev->facingLeft;
    self->posX.i.hi = prev->posX.i.hi;
    self->posY.i.hi = prev->posY.i.hi;

    if (prev->entityId != 0x22) {
        DestroyEntity(self);
    }
}

void func_us_801A4430(Entity* self) {
    if (self->step == 0) {
        InitializeEntity(D_us_801804F0);
        self->hitboxWidth = 0x18;
        self->hitboxHeight = 4;
        self->hitboxOffX = -0x30;
        self->hitboxOffY = -2;
    }
}

// Ending-transition step machine: stop the music (step 0/1), wait for the
// fade-out API to finish then play the "boss defeated" cue (step 2), else
// play a one-shot cue guarded by D_us_80180B74 (step 3). Step 4+ is a no-op.
void func_us_801A4494(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180484);
            // fall through
        case 1:
            if (D_us_801806DC == 0) {
                return;
            }
            stopMusicFlag = 1;
            currentMusicId = 0x334;
            break;

        case 2:
            if (g_api_func_80131F68()) {
                return;
            }
            stopMusicFlag = 0;
            g_api_PlaySfx(currentMusicId);
            break;

        case 3:
            if (D_us_80180B74 != 0) {
                return;
            }
            g_api_PlaySfx(0x91);
            break;

        default:
            return;
    }
    self->step++;
}

void func_us_801A459C(void) {
    D_us_801B5548 = 2;
    D_us_801B5546 = 2;
    D_us_801B554A = 0;
    D_us_801B554E = 0;
    D_us_801B5552 = 0;
    D_us_801B5553 = 8;
    D_us_801B5542 = D_us_801B5544 + 0x14;
}

// One-time HUD icon setup: allocates a 7-sprite chain, stashes its owner and
// first 6 links into the D_us_801B55xx state block (shared with the
// score-digit machinery), then paints a small multi-part icon glyph across
// the last 3 links.
void func_us_801A45F0(Entity* self) {
    s32 primIndex;
    Primitive* p;
    Primitive* p2;

    primIndex = g_api_AllocPrimitives(PRIM_SPRT, 7);
    D_us_801B5574 = primIndex;
    if (primIndex == -1) {
        return;
    }

    D_us_801B553C = self;
    D_us_801B5578 = 0;
    D_us_801B5570 = -1;
    D_us_801B556C = -1;
    func_us_801A459C();

    p = &g_PrimBuf[D_us_801B5574];
    D_us_801B5554[0] = p;
    p->drawMode = 8;

    D_us_801B5554[1] = p->next;
    p = D_us_801B5554[1];
    p->drawMode = 8;

    D_us_801B5554[2] = p->next;
    p = D_us_801B5554[2];
    p->drawMode = 8;

    D_us_801B5554[3] = p->next;
    p = D_us_801B5554[3];
    p->drawMode = 8;

    D_us_801B5554[4] = p->next;
    p = D_us_801B5554[4];
    p->drawMode = 8;

    D_us_801B5568 = p->next;
    p = D_us_801B5568;
    p->type = 4;
    p->drawMode = 8;

    p2 = p->next;
    p2->type = 3;
    p2->r3 = p2->r2 = p2->r1 = p2->r0 = 0xFF;
    p2->g3 = p2->g2 = p2->g1 = p2->g0 = 0;
    p2->b3 = p2->b2 = p2->b1 = p2->b0 = 0;
    p2->x2 = p2->x0 = 4;
    p2->x3 = p2->x1 = 0xF8;
    p2->priority = 0x1FD;
    p2->drawMode = 8;

    p2 = p2->next;
    p2->type = 1;
    p2->x0 = 3;
    p2->y0 = 0x2F;
    p2->v0 = 0x4A;
    p2->b0 = 0xFF;
    p2->g0 = 0xFF;
    p2->r0 = 0xFF;
    p2->priority = 0x1FC;
    p2->drawMode = 8;
}

void func_us_801A47B4(s16 arg0) {
    RECT rect;

    rect.x = 0;
    rect.y = arg0 * 12 + 0x180;
    rect.w = 0x40;
    rect.h = 0xC;
    ClearImage(&rect, 0, 0, 0);
}

// Clears the on-screen row for the current score-digit-style prim slot
// (D_us_801B554A indexes D_us_801B5554[]) and reinitializes that Primitive.
void func_us_801A4810(void) {
    Primitive* p;

    func_us_801A47B4(D_us_801B554A);

    p = D_us_801B5554[D_us_801B554A];
    p->tpage = 0x10;
    p->clut = D_us_801B5550;
    p->u0 = 0;
    p->y0 = D_us_801B5542;
    p->x0 = D_us_801B5540 + 4;
    p->u1 = 0xC0;
    p->v1 = 0xC;
    p->priority = 0x1FF;
    p->drawMode = 0;
    p->v0 = D_us_801B554A * 12 - 0x80;
}

// Builds a row of PRIM_SPRT tile primitives from a per-idx encoded byte
// string (D_us_80180C1C[idx]): 0xFF starts a 2-byte escape that terminates
// the string when followed by 0; any other nonzero byte becomes one sprite
// (packing its low/high nibbles into u0/v0), while a plain 0 byte is a gap
// that just advances the column without consuming a primitive.
void func_us_801A48C8(s32 idx, Entity* dest) {
    u8* str;
    u8 b;
    s32 count;
    s32 primIndex;
    Primitive* prim;
    s32 x;
    s32 y0;

    idx &= 0xFFFF;
    str = D_us_80180C1C[idx];
    count = 0;
    for (;;) {
        b = *str++;
        if (b == 0xFF) {
            b = *str++;
            if (b == 0) {
                break;
            }
        }
        if (b != 0) {
            count++;
        }
    }

    primIndex = g_api_AllocPrimitives(PRIM_SPRT, count & 0xFFFF);
    if (primIndex == -1) {
        DestroyEntity(dest);
        return;
    }

    prim = &g_PrimBuf[primIndex];
    D_us_801B5570 = primIndex;
    str = D_us_80180C1C[idx];

    if (prim != NULL) {
        x = 0x38;
        y0 = D_us_801B5544;
        for (;;) {
            b = *str++;
            if (b != 0) {
                prim->u0 = (b & 0xF) << 3;
                prim->type = 6;
                prim->tpage = 0x1E;
                prim->clut = 0x196;
                prim->v0 = (b & 0xF0) >> 1;
                prim->v1 = 8;
                prim->u1 = 8;
                prim->priority = 0x1FF;
                prim->drawMode = 8;
                prim->x0 = x;
                prim->y0 = y0 + 6;
                x += 8;
                prim = prim->next;
            } else {
                x += 4;
            }
            if (prim == NULL) {
                break;
            }
        }
    }
}

void func_us_801A4A4C(u8* arg0) {
    D_us_801B557C = arg0 + 0x100000;
    D_us_801B557A = 0;
    D_us_801B5578 = 1;
}

// Advances the per-frame timeline set up by func_us_801A4A4C: D_us_801B557C
// is a byte cursor into a scripted command stream, gated by 2-byte big-
// endian timestamps compared against the running frame counter
// D_us_801B557A. Each due command is one of: 0 = spawn (pool index, entity
// id, 12-bit packed X/Y), 1 = destroy pool slot, 2 = wait for/consume a
// D_us_801B55E8 flag bit (rewinding + un-ticking the frame counter if not
// set yet), 3 = set a D_us_801B55E8 flag bit. Keeps processing commands
// (and re-checking new timestamps) until one isn't due yet.
void func_us_801A4A78(void) {
    u8* cmd;
    u16 trigger;
    u8 opcode;

    D_us_801B557A++;
    if (D_us_801B557A > 0xFFFE) {
        D_us_801B5578 = 0;
        return;
    }

    for (;;) {
        cmd = D_us_801B557C;

        trigger = *D_us_801B557C++ << 8;
        trigger |= *D_us_801B557C++;

        if (D_us_801B557A < trigger) {
            D_us_801B557C = cmd;
            return;
        }

        D_us_801B557C++;
        opcode = cmd[2];

        switch (opcode) {
            case 1: {
                u8 idx = *D_us_801B557C++;
                DestroyEntity(&g_Entities_64[idx]);
                break;
            }

            case 0: {
                u8 idx = *D_us_801B557C++;
                Entity* e = &g_Entities_64[idx];

                DestroyEntity(e);
                e->entityId = *D_us_801B557C++;
                e->pfnUpdate = D_us_80180398[e->entityId];
                e->posX.i.hi = *D_us_801B557C++ << 4;
                e->posX.i.hi |= *D_us_801B557C++;
                e->posY.i.hi = *D_us_801B557C++ << 4;
                e->posY.i.hi |= *D_us_801B557C++;
                break;
            }

            case 2: {
                u8 bit = *D_us_801B557C;

                if ((D_us_801B55E8 >> bit) & 1) {
                    D_us_801B557C++;
                    D_us_801B55E8 &= ~(1u << bit);
                } else {
                    D_us_801B557C = cmd;
                    D_us_801B557A--;
                    return;
                }
                break;
            }

            case 3: {
                u8 bit = *D_us_801B557C++;
                D_us_801B55E8 |= 1u << bit;
                break;
            }

            default:
                D_us_801B557C++;
                break;
        }
    }
}

// Scrolls the "current" score-digit prim's V texture coordinate by `delta`
// (advancing to the next digit once it wraps to 0), while every OTHER digit
// prim in the 5-slot pool physically shifts down by `delta`.
void func_us_801A4D3C(u8 delta) {
    s32 idx;
    s32 i;
    Primitive* rec;

    idx = D_us_801B554A + 1;
    while (idx >= 5) {
        idx -= 5;
    }

    if (g_CurrentEntity->step_s == 0) {
        rec = D_us_801B5554[idx];
        rec->v1 -= delta;
        rec->v0 += delta;
        if (rec->v1 == 0) {
            g_CurrentEntity->step_s++;
            rec->drawMode = 8;
        }
    }

    for (i = 0; i < 5; i++) {
        if (i != idx) {
            D_us_801B5554[i]->y0 -= delta;
        }
    }

    D_us_801B554C++;
}

// Death-cutscene driver for the Hippogryph fight. Step 0 triggers the
// (already-established) HUD-icon-chain setup via func_us_801A45F0, called
// here with a dummy persistent Entity (D_us_80181D5C) rather than a real
// spawned one; that function's `void` signature was already WSL-confirmed
// byte-exact for its OTHER call site, so rather than touch it we invoke it
// here through a function-pointer cast to recover the register it leaves
// its bool-ish result in (see func_us_801A45F0's asm: v0 is 0 on alloc
// failure and 1 on success at every return point already, just never
// spelled out in C). Step 1 is a 25-opcode byte-code interpreter for the
// scripted death sequence, reusing D_us_801B553C (elsewhere an Entity*
// "owner" for the HUD icon chain) as a raw byte-stream cursor -- hence the
// explicit u8*/Entity* casts throughout this function only. Steps 2-7 are
// mostly straightforward primitive-chain fades/animations; a couple of
// opcodes (5, 14, 19) involve packed-nibble address reconstruction or
// LoadTPage parameters this session couldn't fully pin down from the
// register trace alone (flagged individually below) -- WSL build is the
// real arbiter for those.
void func_us_801A4E40(Entity* self) {
    if (self->step != 0) {
        if (D_us_801B55EC != 0 && D_us_801B5538 == 0 &&
            ((g_Settings.D_8003CB04 & 0x200) || g_GameClearFlag)) {
            if (g_pads[0].tapped == 0x800) {
                g_api_FreePrimitives(self->primIndex);
                D_us_801B5538 = 1;
                self->flags ^= 0x800000;
                if (D_us_801B5570 != -1) {
                    g_api_FreePrimitives(D_us_801B5570);
                }
                if (D_us_801B556C != -1) {
                    g_api_FreePrimitives(D_us_801B556C);
                }
                g_api_PlaySfx(0xA);
                self->step = 1;
                self->step_s = 0;
            }
        }
    }

    if (self->step != 0 && D_us_801B5578 != 0) {
        func_us_801A4A78();
    }

    if (self->step >= 8) {
        return;
    }

    switch (self->step) {
    case 0: {
        s32 ok;

        if (g_CastleFlags[0x62] || g_DemoMode || !g_PlayableCharacter) {
            D_us_801806DC = 0;
            goto destroy_and_return;
        }

        ok = ((s32(*)(Entity*))func_us_801A45F0)(&D_us_80181D5C);
        if ((ok & 0xFF) == 0) {
            return;
        }

        self->flags |= 0x802000;
        D_us_801B55E8 = 0;
        D_us_801B55EC = 0;
        D_us_801B5538 = 0;
        g_CutsceneHasControl = 1;
        self->step++;
        self->primIndex = D_us_801B5574;
        return;
    }

    case 1: {
        u8 op;

    opcode_loop:
        if (D_us_801B5552 != 0) {
            if (D_us_801B5538 == 0) {
                D_us_801B5552--;
                return;
            }
        }

        op = *(u8*)D_us_801B553C;
        D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);

        switch (op) {
        case 0:
            self->step = 7;
            return;

        case 1: {
            D_us_801B5546 = D_us_801B5548;
            if (!(D_us_801B554E & 1)) {
                D_us_801B5542 += 0xC;
            }
            D_us_801B554A++;
            if (D_us_801B554A >= 5) {
                D_us_801B554A = 0;
            }
            func_us_801A4810();
            if (!(D_us_801B554E & 1)) {
                if (D_us_801B554A < 4) {
                    goto opcode_loop;
                }
                D_us_801B554E |= 1;
            }
            D_us_801B554C = 0;
            self->step_s = 0;
            self->step++;
            return;
        }

        case 2: {
            u8 b = *(u8*)D_us_801B553C;
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            D_us_801B5553 = b;
            goto opcode_loop;
        }

        case 3: {
            u8 b = *(u8*)D_us_801B553C;
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            D_us_801B5552 = b;
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            return;
        }

        case 4: {
            Primitive* p;
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            p = D_us_801B5554[0];
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            return;
        }

        case 5: {
            /* LOW-MEDIUM CONFIDENCE: bit0-of-b1-selected corner-swap below
             * mirrors the register trace; the underlying visual intent
             * (which corner gets which UV) is a guess. */
            u8 b0, b1;
            Primitive* p;
            u8 a0v, a1v;
            u16 clut;
            u8 uA, uB;

            if (D_us_801B5538 != 0) {
                D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);
                goto opcode_loop;
            }

            b0 = ((u8*)D_us_801B553C)[0];
            b1 = ((u8*)D_us_801B553C)[1];
            p = D_us_801B5568;
            a0v = D_us_80180B78[b1 & 1];
            a1v = D_us_80180B7C[b1 & 1];
            clut = D_us_80180B80[b0];
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);

            p->tpage = 0x90;
            p->clut = clut;
            if (b1 & 0x80) {
                uA = a0v + 0x2F;
                uB = a0v;
            } else {
                uA = a0v;
                uB = a0v + 0x2F;
            }
            p->u2 = p->u0 = uA;
            p->u3 = p->u1 = uB;
            p->v0 = p->v1 = a1v;
            p->v2 = p->v3 = a1v + 0x48;

            p->x3 = p->x2 = p->x1 = p->x0 = D_us_801B5540 - 0x1E;
            p->y3 = p->y2 = p->y1 = p->y0 = D_us_801B5544 + 0x24;

            D_us_801B5550 = D_us_80180B88[b0];
            func_us_801A459C();
            func_us_801A4810();
            p->priority = 0x1FE;
            func_us_801A48C8(b0, self);
            p->drawMode = 0;
            D_us_801B554C = 6;
            self->step = 3;
            return;
        }

        case 6: {
            Primitive* p;
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            p = D_us_801B5554[0];
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            p = p->next;
            p->drawMode = 8;
            g_api_FreePrimitives(D_us_801B5570);
            D_us_801B5570 = -1;
            D_us_801B554C = 6;
            self->step = 4;
            return;
        }

        case 7: {
            u8 b0, b1;
            Primitive* p;
            u16 tmp;

            if (D_us_801B5538 != 0) {
                D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);
                goto opcode_loop;
            }

            b0 = ((u8*)D_us_801B553C)[0];
            b1 = ((u8*)D_us_801B553C)[1];
            p = D_us_801B5568;
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);

            D_us_801B5540 = b0;
            D_us_801B5544 = b1;
            p = p->next;
            p->y1 = b1;
            p->y0 = b1;
            tmp = D_us_801B5544 + 0x48;
            p->y3 = tmp;
            p->y2 = tmp;
            p = p->next;
            p->y0 = D_us_801B5544 - 1;
            p->u0 = 0xF6;
            D_us_801B554C = 0x18;
            self->step = 5;
            self->step_s = 0;
            return;
        }

        case 8:
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            D_us_801B554C = 0x18;
            self->step = 6;
            return;

        case 9:
            if (D_us_801B5538 == 0) {
                goto play_sfx_opcode;
            }
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);
            goto opcode_loop;

        case 10:
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            if (!g_api_func_80131F68()) {
                goto rewind_and_return;
            }
            goto opcode_loop;

        case 11: {
            bool result;
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            result = g_api_func_80131F68();
            if (!result) {
                goto rewind_and_return;
            }
            if (result != 1) {
                goto opcode_loop;
            }
            goto rewind_and_return;
        }

        case 12: {
            u8* c = (u8*)D_us_801B553C;
            u32 packed = ((((c[0] << 4) | c[1]) << 4 | c[2]) << 4) | c[3];
            D_us_801B553C = (Entity*)(c + 4);
            func_us_801A4A4C((u8*)packed);
            goto opcode_loop;
        }

        case 13:
            goto opcode_loop;

        case 14: {
            /* LOW CONFIDENCE: this opcode indexes what looks like a
             * script-level jump table via a raw address literally
             * reconstructed from 4 nibble bytes in the script stream
             * ($t1 used directly as a base register with no lui/relocation
             * in the original asm). Translated as literally as possible;
             * needs WSL to confirm. */
            u8* c = (u8*)D_us_801B553C;
            u16 idx = ((((c[0] << 4) | c[1]) << 4 | c[2]) << 4) | c[3];
            u8* afterIdx = c + 4;
            u8* target = afterIdx + (*(u16*)(u32)idx) * 4;
            u16 packed2 = ((((target[0] << 4) | target[1]) << 4 | target[2]) << 4) | target[3];
            D_us_801B553C = (Entity*)(packed2 + 0x100000);
            goto opcode_loop;
        }

        case 15: {
            u8* c = (u8*)D_us_801B553C;
            u16 packed = ((((c[0] << 4) | c[1]) << 4 | c[2]) << 4) | c[3];
            D_us_801B553C = (Entity*)(packed + 0x100000);
            goto opcode_loop;
        }

        case 16: {
            u8 bit = *(u8*)D_us_801B553C;
            if (!((D_us_801B55E8 >> bit) & 1)) {
                goto op_wait_decrement;
            }
            D_us_801B55E8 &= ~(1u << bit);
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            goto opcode_loop;
        }

        case 17: {
            u8 bit = *(u8*)D_us_801B553C;
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            D_us_801B55E8 |= (1u << bit);
            goto opcode_loop;
        }

        case 18:
            D_us_801B5578 = 0;
            goto opcode_loop;

        case 19: {
            /* LOW CONFIDENCE: tp/abr args to LoadTPage were not clearly
             * observable in the register trace (a1/a2 not set within this
             * opcode's own instructions); using 0/0 pending WSL check. */
            if (D_us_801B5538 != 0) {
                D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 5);
                goto opcode_loop;
            }
            {
                u8* c = (u8*)D_us_801B553C;
                u8 b0 = c[0], b1 = c[1], b2 = c[2], b3 = c[3], b4 = c[4];
                u32 pix = (((((b0 << 4) | b1) << 4 | b2) << 4) | b3) + 0x100000;
                u16 x = D_us_80180B84[b4];
                D_us_801B553C = (Entity*)(c + 5);
                LoadTPage((u_long*)pix, 0, 0, x, 0x100, 0x30, 0x48);
                goto opcode_loop;
            }
        }

        case 20:
        play_sfx_opcode: {
            u8 b0 = ((u8*)D_us_801B553C)[0];
            u8 b1 = ((u8*)D_us_801B553C)[1];
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 2);
            g_api_PlaySfx(b1 | (b0 << 4));
            goto opcode_loop;
        }

        case 21:
            D_us_801B55E8 = 0;
            D_us_801B5538 = 0;
            D_us_801B55EC = 0;
            goto opcode_loop;

        case 22: {
            u8 bit = *(u8*)D_us_801B553C;
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            D_us_801B55E8 &= ~(1u << bit);
            goto opcode_loop;
        }

        case 23:
            return;

        case 24: {
            u8 bit = *(u8*)D_us_801B553C;
            if (!((D_us_801B55E8 >> bit) & 1)) {
                goto op_wait_decrement;
            }
            D_us_801B553C = (Entity*)((u8*)D_us_801B553C + 1);
            goto opcode_loop;
        }

        default: {
            /* opcode >= 0x19 and not otherwise handled: treated as an
             * implicit scroll/draw command using the opcode byte's own
             * nibbles as parameters. */
            if (D_us_801B5538 != 0) {
                goto opcode_loop;
            }
            D_us_801B5552 = D_us_801B5553;
            if (op == 0x20) {
                D_us_801B5546 += 2;
                return;
            }
            {
                RECT rect;
                s32 y;
                rect.x = ((op & 0xF) * 2) + 0x380;
                rect.y = (s16)(((op & 0xF0) >> 1) + 0xF0);
                rect.w = 2;
                rect.h = 8;
                y = (s16)(D_us_801B554A * 12 + 0x180);
                MoveImage(&rect, D_us_801B5546, y);
                D_us_801B5546 += 2;
                return;
            }
        }
        }

    rewind_and_return:
        D_us_801B553C = (Entity*)((u8*)D_us_801B553C - 1);
        return;

    op_wait_decrement:
        D_us_801B553C = (Entity*)((u8*)D_us_801B553C - 1);
        return;
    }

    case 2:
        func_us_801A4D3C(2);
        if (D_us_801B554C < 6) {
            return;
        }
        self->step--;
        return;

    case 3: {
        Primitive* p = D_us_801B5568;
        p->x2 -= 4;
        p->x0 = p->x2;
        p->x3 += 4;
        p->x1 = p->x3;
        p->y1 -= 6;
        p->y0 = p->y1;
        p->y3 += 6;
        p->y2 = p->y3;

        D_us_801B554C--;
        if (D_us_801B554C != 0) {
            return;
        }
        self->step = 1;

        p = &g_PrimBuf[D_us_801B5570];
        if (p == NULL) {
            return;
        }
        p->drawMode = 0;
        for (;;) {
            p = p->next;
            if (p == NULL) {
                return;
            }
            p->drawMode = 0;
        }
    }

    case 4: {
        Primitive* p = D_us_801B5568;
        s16 x1;

        p->x2 += 4;
        p->x0 = p->x2;
        p->x3 -= 4;
        p->x1 = p->x3;
        p->y1 += 6;
        p->y0 = p->y1;
        p->y3 -= 6;
        p->y2 = p->y3;

        x1 = p->x1;
        if (!(x1 < D_us_801B5540 - 2)) {
            p->x3 = p->x1 = D_us_801B5540 - 3;
        }

        D_us_801B554C--;
        if (D_us_801B554C != 0) {
            return;
        }
        self->step = 1;
        return;
    }

    case 5:
        switch (self->step_s) {
        case 1:
            goto step5_s1;
        case 0:
            goto step5_s0;
        case 2:
            goto step5_s2;
        default:
            return;
        }

    step5_s0: {
        /* MEDIUM-LOW CONFIDENCE: the PrimitiveType argument to
         * g_api_AllocPrimitives here wasn't set within this opcode's own
         * visible instructions (likely leftover from earlier code this
         * trace didn't cover); PRIM_GT4 is a guess pending WSL check. */
        s16 primIndex = g_api_AllocPrimitives(PRIM_GT4, 0x48);
        D_us_801B55EC = 1;
        if (primIndex == -1) {
            goto destroy_and_return;
        }
        D_us_801B556C = primIndex;
        {
            Primitive* p = &g_PrimBuf[primIndex];
            s32 i = 0;
            if (p != NULL) {
                do {
                    p->r1 = p->r0 = 0x7F;
                    p->g1 = p->g0 = 0;
                    p->b1 = p->b0 = 0;
                    p->x1 = p->x0 = 0xF7;
                    p->priority = 0x1FE;
                    p->drawMode = 0;
                    p->y1 = p->y0 = i + D_us_801B5544;
                    p->x3 = 0xF70;
                    p->x2 = D_us_80180B8C[i];
                    i++;
                } while ((p = p->next) != NULL);
            }
        }
        goto step5_tail;
    }

    step5_s1: {
        Primitive* p = &g_PrimBuf[D_us_801B556C];
        bool stillShrinking = false;
        if (p != NULL) {
            do {
                s32 tmp;
                p->x3 -= p->x2;
                p->x2 += 2;
                tmp = p->x3;
                if (tmp < 0) {
                    tmp += 0xF;
                }
                p->x0 = tmp >> 4;
                if (p->x0 < 5) {
                    p->x0 = 4;
                } else {
                    stillShrinking = true;
                }
            } while ((p = p->next) != NULL);
        }
        if (stillShrinking) {
            return;
        }
        {
            Primitive* p2;
            g_api_FreePrimitives(D_us_801B556C);
            D_us_801B556C = -1;
            p2 = D_us_801B5568->next;
            p2->drawMode = 0x11;
            p2 = p2->next;
            p2->drawMode = 0x51;
        }
        goto step5_tail;
    }

    step5_s2: {
        Primitive* p = D_us_801B5568->next;
        bool active = false;
        if (p->b0 < 0x7F) {
            p->b1 = p->b0 = p->b1 + 8;
            active = true;
        } else {
            p->b1 = p->b0 = 0x7F;
        }
        p = p->next;
        if (p->r0 < 0x10) {
            p->b0 = p->g0 = p->r0 = 0x10;
        } else {
            p->b0 = p->g0 = p->r0 = p->b0 - 0xF;
            active = true;
        }
        if (active) {
            return;
        }
        self->step = 1;
        return;
    }

    step5_tail:
        self->step_s++;
        return;

    case 6: {
        Primitive* p = D_us_801B5568->next;
        D_us_801B554C--;
        if ((s16)D_us_801B554C >= 12) {
            u8 origU0;
            p = p->next;
            origU0 = p->u0;
            p->u0 = (u8)(origU0 + 0xEC);
            if (!(D_us_801B554C & 1)) {
                return;
            }
            p->u0 = (u8)(origU0 - 0x15);
            return;
        }
        if ((s16)D_us_801B554C != 0) {
            p->y3 = p->y2 = p->y3 - 6;
        } else {
            self->step_s = 1;
        }
        p = p->next;
        p->drawMode = 8;
        return;
    }

    case 7:
        DestroyEntity(self);
        g_CastleFlags[0x62] = 1;
        g_CastleFlags[0x85] = 1;
        g_CutsceneHasControl = 0;
        g_Settings.D_8003CB04 |= 0x300;
        return;
    }

    return;

destroy_and_return:
    DestroyEntity(self);
    return;
}

// Nudges g_unkGraphicsStruct.unkC by +-1/frame towards a target derived from
// `x`, snapping straight to the target once within 1 of it. The comparison
// narrows the counter to its low 16 bits (matches the asm's lhu + sign-extend
// on an otherwise s32 field) while the actual increment/decrement/assign
// always touches the full s32.
// NOTE: field corrected from an earlier wrong guess (BottomCornerTextTimer) —
// g_unkGraphicsStruct's real base is 0x800973F8 (it starts with `primIndex`,
// one word before the first field shown in game.h's own comment), so offset
// 0x14 actually lands on `unkC`, one field earlier than assumed.
void func_us_801A5F28(s32 x) {
    s16 target;
    s16 diff;

    target = 0x180 - x;
    diff = target - (s16)g_unkGraphicsStruct.unkC;

    if (diff >= 2) {
        g_unkGraphicsStruct.unkC++;
    } else if (diff < -1) {
        g_unkGraphicsStruct.unkC--;
    } else {
        g_unkGraphicsStruct.unkC = target;
    }
}

// Ending-cutscene camera/player lock sequencer (mirrors func_us_801A4494's
// music-stop step machine but drives the player instead): step 0 inits;
// step 1 is a 3-part sub-machine that scans g_Entities_160 for an entityId
// 3 marker, waits for it to change type, then calls SetStep(2); step 2
// force-locks player input and freezes the demo timer; step 3 forces the
// player to walk toward x=0x140 (or just idles pad input if in wolf form)
// until close enough, then advances; step 4 waits for a D_us_801B55E8 flag
// bit before advancing; step 5 waits for the camera nudge (func_us_801A5F28)
// to settle at x=0x80, then unpauses and destroys self.
void func_us_801A5F88(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180478);
            return;

        case 1:
            switch (self->step_s) {
                case 0: {
                    Entity* e = g_Entities_160;
                    Entity* end = (Entity*)((u8*)g_Entities_160 + 0x1780);

                    for (; e < end; e++) {
                        if (e->entityId == 3) {
                            *(Entity**)&self->ext = e;
                            self->step_s++;
                        }
                    }
                    return;
                }

                case 1: {
                    Entity* target = *(Entity**)&self->ext;

                    if (target->entityId == 3) {
                        return;
                    }
                    self->step_s++;
                    D_us_801806DC = 0;
                    D_us_801B55E8 |= 0x8000;
                    return;
                }

                case 2:
                    SetStep(2);
                    return;

                default:
                    return;
            }

        case 2:
            g_PauseAllowed = 0;
            g_unkGraphicsStruct.pauseEnemies = 1;
            g_Player.padSim = 0x2000;
            if (g_Player.status & PLAYER_STATUS_WOLF_FORM) {
                g_Player.padSim = 2;
            }
            g_Player.demo_timer = 1;
            self->step++;
            return;

        case 3: {
            if (g_Player.status & PLAYER_STATUS_WOLF_FORM) {
                g_Player.padSim = 0;
                if (g_Timer & 1) {
                    g_Player.padSim = 2;
                }
            } else {
                s16 absX;
                s32 arg;

                g_Player.padSim = 0x2000;
                absX = PLAYER.posX.i.hi + g_Tilemap.scrollX.i.hi;

                if (absX >= 0x121) {
                    if (!(D_us_801B55E8 & 1)) {
                        D_us_801B55E8 |= 1;
                    }
                }

                if (absX >= 0x141) {
                    g_Player.padSim = 0;
                    PLAYER.posX.i.hi = 0x140 - g_Tilemap.scrollX.i.hi;
                    self->step++;
                }

                arg = (absX < 0x100) ? 0x100 : absX;
                func_us_801A5F28(arg);
            }
            g_Player.demo_timer = 1;
            return;
        }

        case 4:
            func_us_801A5F28(0x140);
            if (D_us_801B55E8 & 2) {
                self->step++;
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            return;

        case 5:
            func_us_801A5F28(0x100);
            if (g_unkGraphicsStruct.unkC == 0x80) {
                g_PauseAllowed = 1;
                if (g_unkGraphicsStruct.pauseEnemies != 0) {
                    g_unkGraphicsStruct.pauseEnemies = 0;
                }
                DestroyEntity(self);
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            return;

        default:
            return;
    }
}

// Final death-cutscene animation chain: if D_us_801B5538 is set and self
// hasn't reached step 0xD yet, jumps straight there. Steps 0-2 wait on
// D_us_801B55E8 flag bits set elsewhere (probably by a sibling audio/script
// function); steps 3-12 each play one AnimateEntity segment and advance to
// the next step once its own flag bit appears; step 13 waits for its anim to
// finish, then launches self (velocityX) into step 14, which slides self
// until past x=0x111 and destroys it, flagging completion via bit 1.
void func_us_801A62B4(Entity* self) {
    if (D_us_801B5538 && self->step < 0xD) {
        SetStep(0xD);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180460);
            self->animSet = -0x7FFC;
            self->animCurFrame = 1;
            self->unk5A = 0x50;
            self->palette = 0x220;
            self->hitboxState = 0;
            self->posX.i.hi = 0x1C0 - g_Tilemap.scrollX.i.hi;
            return;

        case 1:
            if (D_us_801B55E8 & 4) {
                self->step++;
            }
            return;

        case 2:
            self->animCurFrame = 2;
            if (D_us_801B55E8 & 8) {
                self->step++;
            }
            return;

        case 3:
            AnimateEntity(D_us_80180C24, self);
            if (D_us_801B55E8 & 0x10) {
                SetStep(4);
            }
            return;

        case 4:
            AnimateEntity(D_us_80180C48, self);
            if (D_us_801B55E8 & 0x20) {
                SetStep(5);
            }
            return;

        case 5:
            AnimateEntity(D_us_80180C5C, self);
            if (D_us_801B55E8 & 0x40) {
                SetStep(6);
            }
            return;

        case 6:
            AnimateEntity(D_us_80180C6C, self);
            if (D_us_801B55E8 & 0x80) {
                SetStep(7);
            }
            return;

        case 7:
            AnimateEntity(D_us_80180C74, self);
            if (D_us_801B55E8 & 0x100) {
                SetStep(8);
            }
            return;

        case 8:
            AnimateEntity(D_us_80180C84, self);
            if (D_us_801B55E8 & 0x200) {
                SetStep(9);
            }
            return;

        case 9:
            AnimateEntity(D_us_80180C90, self);
            if (D_us_801B55E8 & 0x400) {
                SetStep(0xA);
            }
            return;

        case 10:
            AnimateEntity(D_us_80180C98, self);
            if (D_us_801B55E8 & 0x800) {
                SetStep(0xB);
            }
            return;

        case 11:
            AnimateEntity(D_us_80180CA8, self);
            if (D_us_801B55E8 & 0x1000) {
                SetStep(0xC);
            }
            return;

        case 12:
            AnimateEntity(D_us_80180CC0, self);
            if (D_us_801B55E8 & 0x2000) {
                SetStep(0xD);
            }
            return;

        case 13:
            if (AnimateEntity(D_us_80180CCC, self) == 0) {
                self->velocityX = 0xC000;
                SetStep(0xE);
            }
            return;

        case 14:
            AnimateEntity(D_us_80180CD4, self);
            MoveEntity();
            if (self->posX.i.hi >= 0x111) {
                D_us_801B55E8 |= 2;
                DestroyEntity(self);
            }
            return;

        default:
            return;
    }
}

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
