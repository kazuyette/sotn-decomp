// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef BO2_H
#define BO2_H

#include <stage.h>

#define OVL_EXPORT(x) BO2_##x

// g_EInitParticle (e_misc.h/e_collect.h) and g_EInitInteractable (e_life_up.h/
// e_stage_name_us.h) and g_EInitCommon (e_room_bg.h/e_red_door.h) all need a
// SECOND raw name too: bo2's own still-unmigrated boss-specific code
// (22610.c/337D0.c, Minotaur/Werewolf AI, not part of this checkpoint) has its
// own local `extern EInit D_us_XXXXXXXX;`-style raw references to these exact
// addresses (confirmed by grepping asm/us/boss/bo2/nonmatchings/{22610,337D0}
// for each candidate address BEFORE writing this file -- see DECOMP_LOG.md,
// this is the same class of bug bo1 hit with D_us_80180AA4/20F60.c). Real
// definitions live in e_init.c under the raw name; alias the friendly name to
// each here so e_misc.c/e_collect.c/e_life_up.c/e_stage_name.c/e_red_door.c/
// e_room_bg.c (all migrated in this checkpoint, all include this header)
// resolve to the same one real symbol.
extern EInit D_us_801804D0;
#define g_EInitParticle D_us_801804D0

extern EInit D_us_801804E8;
#define g_EInitInteractable D_us_801804E8

extern EInit D_us_8018050C;
#define g_EInitCommon D_us_8018050C

// The remaining EInit constants below have NO raw-name conflict (checked the
// same way, against 22610.c/337D0.c and the still-unmigrated e_misc.c stubs
// via `INCLUDE_ASM` stub content) -- declared directly under their real
// literal names.
extern EInit g_EInitLockCamera;    // {0,0,0,0,1} @ D_us_80180500
extern EInit g_EInitDamageNum;     // {0,0,0,0,3} @ D_us_80180518

// g_EInitUnkId13 (e_misc.h) -- ALSO needs a raw-name alias: e_misc.c's own
// frozen asm (staying unmigrated this checkpoint) references D_us_801804F4
// directly. Missed on the first pass (only 22610.c/337D0.c were checked;
// e_misc.c being left untouched doesn't mean its own raw references stop
// mattering -- same lesson as g_EInitParticle/g_EInitInteractable/
// g_EInitCommon above, just caught one build later).
extern EInit D_us_801804F4;
#define g_EInitUnkId13 D_us_801804F4

// EInitObtainable (e_particles.h), OVL_EXPORT-wrapped so always per-overlay
// safe regardless of raw-name usage elsewhere.
extern EInit OVL_EXPORT(EInitObtainable); // {3,0,0,0,1} @ D_us_801804C4

// Entity dispatch table (see e_init.c), 41 entries (entityId 1-41), real
// VRAM address 0x80180414 confirmed via CreateEntityFromLayout's real
// disassembly (entry 0 == EntityBreakable == 0x801A24DC, matching
// config/symbols.us.bobo2.txt). Kept under its friendly OVL_EXPORT name
// (nothing currently needs it externed from this header -- create_entity.c
// stays raw INCLUDE_ASM this checkpoint, see e_init.c) for when
// create_entity.c is properly migrated in a future checkpoint.
extern PfnEntityUpdate OVL_EXPORT(EntityUpdates)[];

// pStObjLayoutHorizontal/Vertical and the dispatch table's own "-1" base are
// declared directly under their RAW names in e_init.c (D_us_8018026C,
// D_us_80180340, D_us_80180410), NOT OVL_EXPORT-wrapped: create_entity.c
// itself stays raw INCLUDE_ASM this checkpoint (see e_init.c for why --
// several of its own internal helper addresses turned out to be real,
// still-needed symbols, not harmless splat artifacts as first assumed), so
// its own frozen asm references these three addresses by their original
// raw splat names directly. No extern needed here for the layout tables
// (nothing in this header's own migrated file set uses the friendly
// OBJ_LAYOUT_HORIZONTAL/VERTICAL macro this checkpoint) but D_us_80180410
// IS now used from real C (e_misc.c's hand-decompiled func_us_801AF910,
// which does its own dispatch-table lookup the same way
// CreateEntityFromLayout does), so it needs a real extern here.
extern PfnEntityUpdate D_us_80180410;

// func_us_801A9D5C / func_us_801A9CE8: RESOLVED (task #45) -- these are not
// separate functions at all. create_entity.c's own asm turned out to have
// its function boundaries mis-drawn by splat in 8 places; these two raw
// addresses are simply the true first instruction of CreateEntityFromEntity
// and CreateEntityFromCurrentEntity respectively (each is a compiler-hoisted
// `addiu sp,sp,-0x20` stack-frame instruction that splat sliced into its own
// bogus "function"). create_entity.c now contains the complete, correct
// bodies for both real functions (which naturally compile to start at these
// exact addresses); 22610.c/337D0.c's existing call sites use the raw names
// below, so they're aliased back onto the real functions here rather than
// updating every call site.
void CreateEntityFromEntity(u16 entityId, Entity* source, Entity* entity);
void CreateEntityFromCurrentEntity(u16 entityId, Entity* entity);
#define func_us_801A9D5C(entityId, source, dest) \
    CreateEntityFromEntity(entityId, source, dest)
#define func_us_801A9CE8(entityId, entity) \
    CreateEntityFromCurrentEntity(entityId, entity)

// Shared boss-AI scratch state used across 22610.c AND 337D0.c (Minotaur's
// two overlay halves reference the same block). Being pieced together
// function-by-function -- flat externs for now (no struct yet) since the
// full layout/semantics are still ambiguous (see DECOMP_LOG.md). Each field
// below is only as trustworthy as the function that revealed it; do not
// assume gaps between named addresses are unused.
extern s16 D_us_801B6846; // = D_us_801B6848 + 0x14 (an angle bias?)
extern s16 D_us_801B6848; // shared text Y-anchor coordinate, see below
extern s16 D_us_801B684A; // = 2 (a state/sub-state value)
extern s16 D_us_801B684C; // = 2 (a state/sub-state value)
extern s16 D_us_801B684E; // index/timer, reset to 0; also used as a small
                           // signed index (-2..1 range observed) into the
                           // pointer-ish region at D_us_801B6850/54/58/5C
extern s16 D_us_801B6850; // part of the same block, role TBD
extern s16 D_us_801B6852; // reset to 0 independently of 0x6850 -- NOTE this
                           // means 0x6850/0x6852 are two u16 fields, NOT one
                           // u32/pointer spanning both (corrects an earlier,
                           // pre-verification hypothesis)
extern u16 D_us_801B6854;   // CORRECTED (func_us_801A56D4 reads it via lhu,
                            // a plain 16-bit value, not a pointer) -- an
                            // earlier pass wrongly guessed this was part of
                            // a 4-pointer array before checking a real read
// CONFIRMED (func_us_801A5CC8 references this directly by name as a real
// 5-element array, indexed 0..4): a 5-slot array of pointers, base at
// D_us_801B6858 (NOT D_us_801B6850 as an earlier, pre-verification pass
// guessed -- D_us_801B6850/6852 are separate scalar u16 fields, see above).
// func_us_801A56D4's `(&D_us_801B684E + index*4 + 0xA)` computed-address
// form is exactly `&D_us_801B6858[index]` for index in 0..4 (0xA ==
// offsetof(D_us_801B6858) - offsetof(D_us_801B684E)).
// CORRECTED type (func_us_801A54B4 shows these slots are populated via
// `g_api.AllocPrimitives` + `&g_PrimBuf[primIndex]`, and every field write
// in func_us_801A56D4/5CC8 lines up exactly with real `Primitive` struct
// offsets -- x0/y0/u0/v0/clut/u1/v1/tpage/priority/drawMode -- not Entity
// fields as an earlier pass assumed from offset-width coincidence alone).
extern Primitive* D_us_801B6858[5];
extern u8 D_us_801B6856;  // = 0
extern u8 D_us_801B6857;  // = 8
extern s16 D_us_801B687C; // = 1 (a state flag)
extern s16 D_us_801B687E; // = 0 (a counter)
// CORRECTED (func_us_801A593C proves this is a byte-pointer script cursor
// for a small timestamped bytecode VM, not a plain s32 -- an earlier pass
// typed it from func_us_801A5910 alone, which merely biases and stores a
// pointer-shaped value into it without revealing its real pointee type).
extern u8* D_us_801B6880; // script cursor: func_us_801A5910 seeds it as
                          // (arg0 + 0x100000), likely an overlay-relative
                          // to absolute address fixup
extern s32 D_us_801B683C; // = 1 (a flag/state), used by func_us_801A5BE8
extern s32 D_us_801B6870; // tracked primIndex-like value, -1 == unused
extern s32 D_us_801B6874; // tracked primIndex-like value, -1 == unused

extern u16 g_pads_0_tapped;
extern u16 g_pads_1_tapped;

// These three are outside bo2's usual 0x8018xxxx/0x801Axxxx/0x801Bxxxx
// overlay range -- likely core/main-executable game state (possibly
// weapon or servant-slot related) rather than bo2-private data. Only
// referenced so far from func_us_801A75B4; declared minimally (address-of
// use only, so exact type doesn't affect correctness here).
extern u8 D_8007C6B8;
extern s16 D_8007C6E8;
extern s16 D_80077A88;

extern EInit D_us_801804DC; // used by func_us_801A75B4
extern u16 D_us_801B6844; // part of the shared boss-AI state block, used by func_us_801A56D4
extern s32 D_us_801B6840; // saved Entity* (as s32), set by func_us_801A54B4
extern s32 D_us_801B6878; // primIndex from g_api.AllocPrimitives, or 0/-1 on failure
extern Primitive* D_us_801B686C; // 6th primitive-chain slot, set by func_us_801A54B4 (NOT
                                  // part of the 5-element D_us_801B6858 array used elsewhere)

extern u8* D_us_80180C20[]; // per-stringId bitmap-font string table, used by func_us_801A578C

// Entity offset 0x90 (ext+0x14): a "step transition ready"/turn-passing byte
// flag, referenced on `self` AND on neighboring entities (self-1, self+1)
// by several 22610.c functions (func_us_801A3818, and the still-deferred
// func_us_801A269C). Not yet a named struct field (no single function's
// ext layout fully explains it), so accessed uniformly via this macro
// rather than inventing a premature/inconsistent struct per call site.
#define ENTITY_UNK90(e) (*((u8*) (e) + 0x90))

// Entity offset 0x7C/0x80/0x88 (ext+0/+4/+0xC): a boss-flash-overlay entity's
// own primitive pointer + two counters, used by func_us_801A460C. ext+0x7C
// matches ET_Werewolf.prim/ET_Minotaur's padding, ext+0x80 matches
// ET_Werewolf/ET_Minotaur's `timer`; ext+0x88 isn't named in either struct
// (declared as padding in both) despite being genuinely read/written here,
// so all three are accessed via raw macros -- same rationale as
// ENTITY_UNK90 -- rather than committing to one boss variant's ext member.
#define ENTITY_UNK7C(e) (*(Primitive**) ((u8*) (e) + 0x7C))
#define ENTITY_UNK80(e) (*(u16*) ((u8*) (e) + 0x80))
#define ENTITY_UNK88(e) (*(u16*) ((u8*) (e) + 0x88))

// Boolean gate checked by func_us_801A460C's fade-out (step_s==1) and
// particle-spawn logic; likely a "boss defeat sequence still running" flag
// set elsewhere (possibly in the deferred func_us_801A269C conductor).
extern s32 D_us_80180AB0;

// func_us_801A4EE0's cases 1/3 poke halfwords at g_Tilemap+0x1F6.."+0x2F6 --
// far past the small Tilemap typedef's real size (0x54 bytes), landing in
// g_BgLayers[] territory instead (confirmed via config/symbols.us.txt: no
// named symbol exists in that whole address range). Rather than guess an
// unverified BgLayer-array index/field split, access these the same way
// ENTITY_UNK90 handles an unnamed Entity field: a raw offset macro.
#define TILEMAP_RAW16(off) (*(u16*) ((u8*) &g_Tilemap + (off)))

extern u8 D_us_80180A00[]; // per-animCurFrame frame-record index, used by func_us_801A3818
extern u8 D_us_80180964[]; // 4-byte hitbox records (offX,offY,width,height -- first
                           // two read as signed s8, last two as plain u8), used by
                           // func_us_801A3818
extern EInit D_us_8018053C; // used by func_us_801A3818

extern s32 D_us_801B68EC; // shared boss-AI bitmask, tested for various bits
                          // (0x4/0x8/0x20/0x80/0x100) as step-advance triggers
extern u8 D_us_80180C28[]; // AnimateEntity frames, used by func_us_801A7340
extern u8 D_us_80180C30[]; // AnimateEntity frames, used by func_us_801A7340
extern u8 D_us_80180C50[]; // AnimateEntity frames, used by func_us_801A7340

extern Entity g_Entities_64[]; // sub-pool used by the bytecode VM in func_us_801A593C

// func_us_801A6EF8 -- the boss-defeat cutscene controller (locks the
// player via g_Player.padSim/demo_timer, checks g_Status.wornEquipment[1]
// == ITEM_AXE_LORD_ARMOR to skip a redundant animation copy onto
// g_Entities[0] -- this is the true-ending/Richter-path armor check,
// consistent with bo2 (Minotaur/Werewolf) canonically being a
// true-ending-relevant fight).
extern s32 D_us_80180B74; // one-shot "cutscene started" flag
extern u8 D_us_80180C8C[]; // AnimateEntity frames
extern u8 D_us_80180C98[]; // AnimateEntity frames
extern s32 D_us_80180AAC; // shared "cutscene done" flag (also read by the
                          // deferred func_us_801A269C, .Lus_801A296C block)
extern s16 PLAYER_posX_i_hi;
extern s16 D_us_801B6848; // shared text Y-anchor coordinate (used by func_us_801A578C);
                          // CORRECTED from an earlier guess ("base angle?") in func_us_801A5460's
                          // comment -- that guess was never verified against a real use site

// Paired angle/magnitude table used by func_us_801A3AC8 (indexed by
// self->params). D_us_80180A60 sits exactly 4 bytes after D_us_80180A5C in
// the real data, and both are indexed with the same `params * 8` byte
// stride (`params * 2` as s32[] index) -- i.e. this is really one struct
// array of {s32 first; s32 second;} entries, expressed here as two parallel
// flat arrays to avoid inventing an unverified struct layout/symbol.
extern s32 D_us_80180A5C[];
extern s32 D_us_80180A60[];

extern EInit D_us_80180548; // used by func_us_801A3AC8
extern EInit D_us_80180554; // used by func_us_801A3BB4
extern u8 D_us_80180834[];  // AnimateEntity frames, used by func_us_801A3BB4
extern u8 D_us_80180864[];  // AnimateEntity frames, used by func_us_801A3CF0
extern u8 D_us_80180B44[];  // AnimateEntity frames, used by func_us_801A4BA4

extern EInit D_us_80180524; // used by func_us_801A4D4C
extern u16 D_us_80180B6C[]; // scaleX/scaleY lookup, indexed by self->params, used by func_us_801A4D4C
extern u8 D_us_80180B60[];  // AnimateEntity frames, used by func_us_801A4D4C
extern char D_us_801A1F54[]; // FntPrint format string (debug frame-stepper display), used by func_us_801A4D4C
extern char D_us_801A1F60[]; // FntPrint format string (debug frame-stepper display), used by func_us_801A4EE0

// func_us_801A3E04 -- the boss-defeat/ending-trigger conductor: debug
// strings confirm this ("set_step %x\n" on self->step, "boss_go %x\n" on
// D_us_80180AAC). Operates on self+1/self+4 (the two visible boss-part
// entities, same offset idiom as func_us_801A4EE0's self+1/self+2) plus two
// raw Entity-sized data blocks (D_80078618/+0xBC) used as pseudo-entities
// for the ending's credits/staff-roll trigger via func_us_801A9CE8.
extern char D_us_801A1EF4[];
extern char D_us_801A1F04[];
extern Entity D_80078618[];

// A per-boss-configuration lookup table (D_us_80180AB4[self->0x8C]) giving
// each of the two boss-part entities an AnimateEntity-style {frames*, unk}
// pair (self+4 reads entry[0], self+1 reads entry[8]); some cases re-read
// the unk4/unk6 or unkC/unkE pair together as one 32-bit "tag" value rather
// than the individual u16 (confirmed via two different load widths at the
// same table in func_us_801A3E04's cases 5 vs 6/7) -- both access forms are
// kept since neither can be dropped without losing an observed behavior.
typedef struct {
    s32 unk0;
    u16 unk4;
    u16 unk6;
    s32 unk8;
    u16 unkC;
    u16 unkE;
} BossPartInitEntry; /* size 0x10 */
extern BossPartInitEntry D_us_80180AB4[];

// Entity offsets 0x84/0x8C/0x8D/0x8E (ext+8/ext+0x10/+0x11/+0x12): more
// unnamed per-boss-part scratch fields read/written by
// func_us_801A3E04 -- same ENTITY_UNK90-style rationale (shared ambiguous
// ext layout between the two boss variants).
#define ENTITY_UNK84(e) (*(s32*) ((u8*) (e) + 0x84))
#define ENTITY_UNK8C(e) (*(u8*) ((u8*) (e) + 0x8C))
#define ENTITY_UNK8D(e) (*(u8*) ((u8*) (e) + 0x8D))
#define ENTITY_UNK8E(e) (*(u8*) ((u8*) (e) + 0x8E))
extern u16 g_pads_1_pressed;

// func_us_801A5DCC -- the boss-intro cutscene controller and its own small
// bytecode VM (see the function's own comment for the opcode list).
//
// D_us_801B6840 doubles as a raw script-byte cursor while this function's
// own intro script (D_us_80181DC5/CCC below) is active: func_us_801A54B4
// (step 0) just does `D_us_801B6840 = (s32) self` generically with
// whichever pointer it's handed, so passing it one of these two raw script
// tables (cast to Entity*, matching the parameter type func_us_801A54B4
// happens to be declared with) turns D_us_801B6840 into a pointer into that
// table for the rest of the script's run -- a second, independent instance
// of the same s32-holds-a-pointer trick already used by
// D_us_801B6880/func_us_801A5910 for the OTHER (g_Entities_64) VM.
#define BOSS_SCRIPT_CURSOR (*(u8**) &D_us_801B6840)

// The two static intro scripts read via BOSS_SCRIPT_CURSOR, selected in
// step 0 by self->params (0 == normal ending, nonzero == true/Richter-armor
// ending). Raw byte arrays -- opcode-encoded, not a named struct.
extern u8 D_us_80181DC5[];
extern u8 D_us_80181CCC[];

// "true-ending path already started" one-shot flag, checked in step 0
// (alongside g_CastleFlags[0xB4]/g_PlayableCharacter/g_DemoMode) and set at
// the very end of the script (step 7's params==0 branch... no, see the
// function body: it's actually the OTHER branch's counterpart) -- gates
// against re-entering the true-ending script a second time.
extern s32 D_us_80180B78;

// Set once func_us_801A54B4 succeeds (step 0); armed as a "check for the
// real ending condition" gate at the top of every call (see the function's
// own comment) alongside D_us_801B683C.
extern s32 D_us_801B68F0;

// FntPrint debug format string, used by func_us_801A5DCC to print the
// script cursor value every call.
extern char D_us_801A1F84[];

// Lookup tables used by func_us_801A5DCC's opcode 5 (prim UV/position setup
// for a boss-intro sprite) and opcode 19/step 5 (LoadTPage x-coordinate and
// a 72-tile HUD gauge's per-tile x2 gradient value, respectively). Same
// "byte-value indexes a small table" pattern seen elsewhere in this file
// (D_us_80180B6C, D_us_80180A5C/60) -- flat arrays, no struct invented.
extern u8 D_us_80180B7C[];
extern u8 D_us_80180B80[];
extern u16 D_us_80180B84[];
extern u16 D_us_80180B88[];
extern u16 D_us_80180B8C[];
extern u16 D_us_80180B90[];

// func_us_801A269C -- the main boss "conductor" entity (E_UNK_17), driving
// the whole Minotaur/Werewolf fight through a large self->step state
// machine (steps named after their raw values: 0/1 setup+intro pose,
// 2/0x22 idle/aggro cycling, 7/8/0xB/0xC/0xE/0x10/0x12/0x14/0x16/0x18/0x21
// individual attack patterns, 0x40 defeat animation, 0xFF a hidden debug
// menu gated behind pad input). Same "self - 3" neighboring-entity idiom
// already seen in func_us_801A3E04 (bo2.h ENTITY_UNK84 etc.) turns out to
// be THIS conductor's own reference point when read from other entities;
// here, within the conductor's own code, `self - 3` is instead read AS a
// second copy of itself (likely the other boss variant, Minotaur vs
// Werewolf, sharing this same conductor function at a fixed slot offset).
// Ends every call (except the few paths that return directly from
// mid-switch) by looking up self->step in `D_us_80180908` to index a
// 4-byte hitbox record in `D_us_80180880` (offX/offY as signed bytes,
// width/height as plain bytes -- same record shape as func_us_801A3818's
// own hitbox table), then calls func_us_801A2610() (the room-bounds
// bouncer, already decompiled at the top of this file).
extern EInit D_us_80180530;
extern u16 D_us_80180536; // a second post-intro palette value

// AnimateEntity frame tables / UnkCollisionFunc hit tables used only by
// func_us_801A269C's many attack-pattern steps. Declared as plain byte
// arrays (same convention as every other AnimateEntity table in this
// file) since AnimateEntity/UnkCollisionFunc2/UnkCollisionFunc3 are
// themselves called without a prototype elsewhere in this file already
// (implicit declaration; tolerated by this project's PSX compiler
// invocation, see func_us_801A3818).
extern u8 D_us_801806C8[];
extern u8 D_us_801806D0[];
extern u8 D_us_801806E0[];
extern u8 D_us_801806F4[];
extern u8 D_us_80180704[];
extern u8 D_us_80180714[];
extern u8 D_us_80180724[];
extern u8 D_us_80180734[];
extern u8 D_us_8018073C[];
extern u8 D_us_80180750[];
extern u8 D_us_80180770[];
extern u8 D_us_8018077C[];
extern u8 D_us_80180790[];
extern u8 D_us_80180798[];
extern u8 D_us_801807AC[];
extern u8 D_us_801807B4[];
extern u8 D_us_801807D0[];
extern u8 D_us_801807F4[];
extern u8 D_us_801807FC[];
extern u8 D_us_80180808[];
extern u8 D_us_80180810[];
extern u8 D_us_80180818[];
extern u8 D_us_80180824[];
extern u8 D_us_80180850[];
extern u8 D_us_80180858[];

// Per-self->step hitbox-record lookup used by func_us_801A269C's shared
// epilogue: D_us_80180908[step] gives a byte index into D_us_80180880,
// a table of 4-byte records (offX, offY as signed bytes; width, height as
// plain bytes) -- same shape as func_us_801A3818's own D_us_80180964.
extern u8 D_us_80180908[];
extern u8 D_us_80180880[];

extern char D_us_801A1E68[]; // FntPrint "step" debug format string
extern char D_us_801A1E78[]; // FntPrint format string (arg: ENTITY_UNK88(self))
extern char D_us_801A1E8C[]; // FntPrint format string, debug menu (step==0xFF)

extern Entity g_Entities_160[];
extern Entity g_Entities_224[];

extern s32 PLAYER_velocityX;
extern u16 PLAYER_facingLeft;

// Entity offset 0x82/0x8F/0x9C (ext+6/+0x13/+0x20): more unnamed per-
// conductor scratch fields, same ENTITY_UNK90-style rationale as the rest
// of this shared ambiguous ext layout.
#define ENTITY_UNK82(e) (*(s16*) ((u8*) (e) + 0x82))
#define ENTITY_UNK8F(e) (*(u8*) ((u8*) (e) + 0x8F))
#define ENTITY_UNK9C(e) (*(s32*) ((u8*) (e) + 0x9C))

// Entity IDs for bo2 (Minotaur/Werewolf), in exact order of the real dispatch
// table at VRAM 0x80180414 in disks/us/BOSS/BO2/BO2.BIN -- resolved directly
// from the ROM/real disassembly, same technique used throughout bo1/bo3.
// 41 entries. Slots whose function is boss-specific are named E_UNK_XX with
// the real function comment alongside.
typedef enum {
    /* 0x01 */ E_BREAKABLE = 1,
    /* 0x02 */ E_EXPLOSION,
    /* 0x03 */ E_PRIZE_DROP,
    /* 0x04 */ E_DAMAGE_DISPLAY,
    /* 0x05 */ E_RED_DOOR, // confirmed = shared EntityRedDoor (func_us_801A9E50, e_red_door.c)
    /* 0x06 */ E_UNK_06, // func_us_801AF744 (e_misc.c) - boss-specific, deferred (not migrated this checkpoint)
    /* 0x07 */ E_SOUL_STEAL_ORB,
    /* 0x08 */ E_ROOM_FOREGROUND,
    /* 0x09 */ E_STAGE_NAME_POPUP,
    /* 0x0A */ E_EQUIP_ITEM_DROP,
    /* 0x0B */ E_RELIC_ORB,
    /* 0x0C */ E_HEART_DROP,
    /* 0x0D */ E_ENEMY_BLOOD,
    /* 0x0E */ E_MESSAGE_BOX,
    /* 0x0F */ E_DUMMY_0F,
    /* 0x10 */ E_DUMMY_10,
    /* 0x11 */ E_BACKGROUND_BLOCK, // confirmed = shared EntityBackgroundBlock (func_us_801A2258, e_room_bg.c)
    /* 0x12 */ E_LOCK_CAMERA, // confirmed = shared EntityLockCamera (func_us_801A2320, e_lock_camera.c)
    /* 0x13 */ E_UNK_ID13,
    /* 0x14 */ E_EXPLOSION_VARIANTS,
    /* 0x15 */ E_GREY_PUFF,
    /* 0x16 */ E_UNK_16, // func_us_801A3E04 (22610.c, boss-specific)
    /* 0x17 */ E_UNK_17, // func_us_801A269C (22610.c, boss-specific)
    /* 0x18 */ E_UNK_18, // func_us_801A3818 (22610.c, boss-specific)
    /* 0x19 */ E_UNK_19, // func_us_801A3AC8 (22610.c, boss-specific)
    /* 0x1A */ E_UNK_1A, // func_us_801A3BB4 (22610.c, boss-specific)
    /* 0x1B */ E_UNK_1B, // func_us_801B385C (337D0.c, boss-specific)
    /* 0x1C */ E_UNK_1C, // func_us_801B4F5C (337D0.c, boss-specific)
    /* 0x1D */ E_UNK_1D, // func_us_801B503C (337D0.c, boss-specific)
    /* 0x1E */ E_UNK_1E, // func_us_801B5218 (337D0.c, boss-specific)
    /* 0x1F */ E_UNK_1F, // func_us_801B52FC (337D0.c, boss-specific)
    /* 0x20 */ E_UNK_20, // func_us_801B56C0 (337D0.c, boss-specific)
    /* 0x21 */ E_UNK_21, // func_us_801A3CF0 (22610.c, boss-specific)
    /* 0x22 */ E_UNK_22, // func_us_801A4D4C (22610.c, boss-specific)
    /* 0x23 */ E_UNK_23, // func_us_801A4EE0 (22610.c, boss-specific)
    /* 0x24 */ E_LIFE_UP_SPAWN,
    /* 0x25 */ E_UNK_25, // func_us_801A5DCC (22610.c, boss-specific)
    /* 0x26 */ E_UNK_26, // func_us_801A6EF8 (22610.c, boss-specific)
    /* 0x27 */ E_UNK_27, // func_us_801A7340 (22610.c, boss-specific)
    /* 0x28 */ E_UNK_28, // func_us_801A460C (22610.c, boss-specific)
    /* 0x29 */ E_UNK_29, // func_us_801A4BA4 (22610.c, boss-specific)
} EntityIDs;

// 337D0.c (func_us_801B37D0 onward, second half of the Minotaur/Werewolf
// overlay). func_us_801B37D0 itself is a room-bounds bouncer variant of
// func_us_801A2610 (8px margin, 0x1A8/0x58 clamp values instead of
// 0x10px/0x1A0/0x60) -- needs no new externs.

// func_us_801B4F5C -- a hitbox-tracker linked to the entity right before it
// (Entity* prev = self - 1;), exact same idiom as func_us_801A3818 but
// tracking E_UNK_1B (func_us_801B385C, this overlay half's own conductor)
// via entityId == 0x1B instead of 0x17.
extern EInit D_us_8018056C;
extern u8 D_us_80181A88[]; // per-animCurFrame frame-record index (same
                           // pattern as D_us_80180A00/func_us_801A3818)
extern u8 D_us_80181A3C[]; // 4-byte hitbox records (offX,offY,width,height),
                           // same pattern as D_us_80180964/func_us_801A3818

// func_us_801B5218 -- a flash/pulse effect entity: fades opacity down by 8
// each frame (self-destructing at 0), cycles animCurFrame between 0 and
// self->params via an ENTITY_UNK80 frame counter.
extern EInit D_us_80180560;
extern u16 D_us_80180566; // a palette-related base value, combined with a
                          // fixed -0x7FF7 bias into self->palette at init

// func_us_801B503C -- self-1.
// Reads a linked Entity* from ext+0x7C (a THIRD, distinct use of this same
// ext offset: func_us_801A460C uses it for a Primitive*, func_us_801B52FC
// below uses it for a Primitive* too, but THIS function dereferences it
// with Entity-sized field offsets (+8 velocityX, +0x34 flags) that don't
// fit inside a 0x24-byte Primitive -- confirmed by direct offset math, not
// assumed. Kept as a local raw cast rather than reusing the ENTITY_UNK7C
// macro to avoid mixing incompatible pointee types under one name.
extern EInit D_us_80180578;

// func_us_801B52FC -- allocates 3 chained PRIM_GT4 prims and (when
// self->params==0) two E_UNK_1F helper entities via func_us_801A9D5C.
// Stores its own prim chain head at ext+0x7C using the SAME ENTITY_UNK7C
// macro as func_us_801A460C (genuinely a Primitive* here, unlike
// func_us_801B503C's unrelated reuse of the same ext offset above).
extern EInit D_us_80180584;

// func_us_801B56C0 -- boss-variant twin of the regular Werewolf's
// EntityWerewolfDeathFlames (src/st/rare/e_werewolf.c). D_us_80181ACC is
// this variant's own death_flame_vector-equivalent: 4 SVECTORs (3 for
// gte_ldv3c/RTPT, the 4th for gte_ldv0/RTPS), same shape as that twin's
// static table. D_us_801A16F0 is an array of u8* (one pointer per sprite
// frame, indexed by a 0-0xD frame counter), each pointee read at +0x10
// for 4 bytes (u2/u0, v1/v0, u3/u1, v3/v2) -- byte-sized counterpart to
// e_werewolf's u16-based sprites_rare_4 (which reads at +8 instead).
extern SVECTOR D_us_80181ACC[4];
extern u8* D_us_801A16F0[];

// Declared explicitly (matches e_werewolf.c's own local forward-declare)
// since it returns a pointer -- an implicit `int` declaration would
// otherwise need a cast at every call site.
Primitive* FindFirstUnkPrim2(Primitive* prim, u8 index);

// create_entity.c's own private copies of the shared create_entity.h
// scratch statics (g_LayoutObjHorizontal/g_LayoutObjVertical/
// g_LayoutObjPosHorizontal/g_LayoutObjPosVertical) -- confirmed via
// FindFirstEntityAbove/Below and CreateEntitiesAbove/Below/ToTheRight/
// ToTheLeft's own asm, all cross-referencing these exact 4 addresses.
// func_us_801B5C2C sits immediately before the already-known
// D_us_801B683C-starting "shared boss-AI state block" above; it is the start
// of a huge all-zero-word BSS scratch block (0x801B5C2C..0x801B6930+, pure
// `nop` in the raw asm dump, never `jal`'d anywhere), but splat's classifier
// defaulted its very first word to a bogus "function" name since it had no
// data-symbol hint for that exact word (unlike the D_us_-prefixed words a
// little further into the same block, which it classified correctly).
// Defined as real `u16*` storage under that pre-existing splat name in
// 337D0.c (not renamed to a D_us_ name) since create_entity.c's own real
// disassembly already references it that way throughout -- see 337D0.c's
// comment on the definition itself for the full BSS-vs-code writeup.
extern u16* func_us_801B5C2C; // g_LayoutObjHorizontal equivalent
extern u16* D_us_801B5C30; // g_LayoutObjVertical equivalent
extern u8 D_us_801B5C34;   // g_LayoutObjPosHorizontal equivalent
extern u8 D_us_801B5C38;   // g_LayoutObjPosVertical equivalent

// Room layout tables (horizontal 53 entries, vertical 52 -- see e_init.c's
// own comment on the asymmetry), used by InitRoomEntities.
extern LayoutEntity* D_us_8018026C[53];
extern LayoutEntity* D_us_80180340[52];

// func_us_801B385C -- the second boss "conductor" (E_UNK_1B), this overlay
// half's own mirror of func_us_801A269C (22610.c): a large self->step state
// machine ending in the same "look up a per-pose hitbox record, then call
// this half's own room-bounds bouncer" epilogue pattern. Debug FntPrint
// strings for self->step / ENTITY_UNK88(self) / the step==0xFF menu.
extern char D_us_801A219C[];
extern char D_us_801A21AC[];
extern char D_us_801A21C0[];

// AnimateEntity frame tables / UnkCollisionFunc hit tables used only by
// func_us_801B385C's many attack-pattern steps -- same plain-byte-array
// convention as func_us_801A269C's own D_us_801806C8 etc.
extern u8 D_us_80181868[];
extern u8 D_us_80181858[];
extern u8 D_us_80181874[];
extern u8 D_us_80181850[];
extern u8 D_us_80181884[];
extern u8 D_us_801818A4[];
extern u8 D_us_801818AC[];
extern u8 D_us_801818BC[];
extern u8 D_us_801818D0[];
extern u8 D_us_801818D8[];
extern u8 D_us_801818E0[];
extern u8 D_us_801818EC[];
extern u8 D_us_80181930[];
extern u8 D_us_801818FC[];
extern u8 D_us_80181908[];

// Per-self->pose hitbox-record lookup used by func_us_801B385C's shared
// epilogue: D_us_801819F8[pose] gives a byte index into D_us_80181958, a
// table of 4-byte records (offX, offY as signed bytes; width, height as
// plain bytes) -- same shape as func_us_801A269C's own D_us_80180908/880
// pair.
extern u8 D_us_801819F8[];
extern u8 D_us_80181958[];

#endif // BO2_H
