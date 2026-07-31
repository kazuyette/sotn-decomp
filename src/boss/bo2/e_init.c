// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo2.h"

// Forward declarations for entity dispatch table below. Real functions
// migrated this checkpoint use their real OVL_EXPORT/shared name; boss-
// specific ones (22610.c, 337D0.c, and e_misc.c's func_us_801AF744) stay as
// raw INCLUDE_ASM stubs -- not touched this checkpoint -- so are declared
// here under their existing raw splat names.
void OVL_EXPORT(EntityRedDoor)(Entity* self);
void OVL_EXPORT(EntityBackgroundBlock)(Entity* self);
void OVL_EXPORT(EntityLockCamera)(Entity* self);

// Real shared functions defined in e_misc.h/e_life_up.h -- not included here
// since e_misc.c stays unmigrated this checkpoint (has embedded boss-specific
// code, deferred) and e_life_up.h's own static definition already provides
// these for real elsewhere in the link; forward-declared here only so the
// dispatch table below type-checks.
void EntityUnkId13(Entity* self);
void EntityExplosionVariants(Entity* self);
void EntityGreyPuff(Entity* self);
void EntityLifeUpSpawn(Entity* self);

void func_us_801AF744(Entity* self); // e_misc.c, boss-specific, deferred
void func_us_801A3E04(Entity* self); // 22610.c
void func_us_801A269C(Entity* self); // 22610.c
void func_us_801A3818(Entity* self); // 22610.c
void func_us_801A3AC8(Entity* self); // 22610.c
void func_us_801A3BB4(Entity* self); // 22610.c
void func_us_801B385C(Entity* self); // 337D0.c
void func_us_801B4F5C(Entity* self); // 337D0.c
void func_us_801B503C(Entity* self); // 337D0.c
void func_us_801B5218(Entity* self); // 337D0.c
void func_us_801B52FC(Entity* self); // 337D0.c
void func_us_801B56C0(Entity* self); // 337D0.c
void func_us_801A3CF0(Entity* self); // 22610.c
void func_us_801A4D4C(Entity* self); // 22610.c
void func_us_801A4EE0(Entity* self); // 22610.c
void func_us_801A5DCC(Entity* self); // 22610.c
void func_us_801A6EF8(Entity* self); // 22610.c
void func_us_801A7340(Entity* self); // 22610.c
void func_us_801A460C(Entity* self); // 22610.c
void func_us_801A4BA4(Entity* self); // 22610.c

static LayoutEntity D_us_80181C86[]; // forward decl, defined below with the other LayoutEntity blobs

// D_us_80180410: real ROM value 0x80181C86 (a pointer into the vertical
// LayoutEntity table's own content -- see D_us_80181C86 below), placed
// immediately before BO2_EntityUpdates below so it lands at the correct
// address (0x410, 4 bytes before 0x414). This is CreateEntityFromLayout's
// own "-1" index folding: `PfnEntityUpdates[entityId - 1]` compiles to a
// base of (table - 4) + entityId*4, and the compiler/linker need a real
// symbol at that exact base address. entityId is never 0 in practice so
// this value is never actually dereferenced as a real dispatch entry --
// its only job is to occupy the correct 4 bytes byte-exactly. Needed under
// this raw name directly (not aliased) because create_entity.c itself
// (still raw INCLUDE_ASM -- see note below) and 22610.c/e_misc.c's frozen
// asm all reference it by this literal name.
PfnEntityUpdate D_us_80180410 = (PfnEntityUpdate)D_us_80181C86;

// Entity dispatch table, 41 entries (entityId 1-41), confirmed against real
// disassembly of CreateEntityFromLayout.s (D_us_80180410 above == real table
// start - 4) and cross-checked entry-by-entry against
// config/symbols.us.bobo2.txt real addresses (e.g. entry 0 == 0x801A24DC ==
// EntityBreakable). No raw-name conflict found for the table itself, kept
// under its friendly OVL_EXPORT name for when create_entity.c is properly
// migrated in a future checkpoint.
PfnEntityUpdate OVL_EXPORT(EntityUpdates)[] = {
    EntityBreakable,                 // 0x01
    EntityExplosion,                 // 0x02
    EntityPrizeDrop,                 // 0x03
    EntityDamageDisplay,              // 0x04
    OVL_EXPORT(EntityRedDoor),       // 0x05
    func_us_801AF744,                // 0x06 (e_misc.c, boss-specific)
    EntitySoulStealOrb,               // 0x07
    EntityRoomForeground,             // 0x08
    EntityStageNamePopup,             // 0x09
    EntityEquipItemDrop,              // 0x0A
    EntityRelicOrb,                   // 0x0B
    EntityHeartDrop,                  // 0x0C
    EntityEnemyBlood,                 // 0x0D
    EntityMessageBox,                 // 0x0E
    EntityDummy,                      // 0x0F
    EntityDummy,                      // 0x10
    OVL_EXPORT(EntityBackgroundBlock), // 0x11
    OVL_EXPORT(EntityLockCamera),     // 0x12
    EntityUnkId13,                    // 0x13
    EntityExplosionVariants,          // 0x14
    EntityGreyPuff,                   // 0x15
    func_us_801A3E04,                 // 0x16
    func_us_801A269C,                 // 0x17
    func_us_801A3818,                 // 0x18
    func_us_801A3AC8,                 // 0x19
    func_us_801A3BB4,                 // 0x1A
    func_us_801B385C,                 // 0x1B
    func_us_801B4F5C,                 // 0x1C
    func_us_801B503C,                 // 0x1D
    func_us_801B5218,                 // 0x1E
    func_us_801B52FC,                 // 0x1F
    func_us_801B56C0,                 // 0x20
    func_us_801A3CF0,                 // 0x21
    func_us_801A4D4C,                 // 0x22
    func_us_801A4EE0,                 // 0x23
    EntityLifeUpSpawn,                 // 0x24
    func_us_801A5DCC,                 // 0x25
    func_us_801A6EF8,                 // 0x26
    func_us_801A7340,                 // 0x27
    func_us_801A460C,                 // 0x28
    func_us_801A4BA4,                 // 0x29
};

// OBJ_LAYOUT_HORIZONTAL/VERTICAL (create_entity.h), 53 entries each, real
// addresses confirmed via CreateEntityFromLayout.s (D_us_80180410, objLayoutId
// dispatch base) and InitRoomEntities.s (D_us_8018026C horizontal read first
// into g_LayoutObjHorizontal, D_us_80180340 vertical read second into
// g_LayoutObjVertical -- matches create_entity.h's read order exactly).
// Symmetric this time: both tables span exactly 0x26C-0x340 and 0x340-0x414,
// (0x414 independently confirmed as the dispatch table's real start via
// EntityBreakable match above), so there's no bo1-style asymmetry/ambiguity.

// LayoutEntity content blobs pointed to by the tables below. 10 unique blobs
// (bo2 has far fewer real room-layout variants than bo1), all transcribed
// byte-for-byte from the ROM, terminator-scanned (sentinel-terminated
// LayoutEntity lists: {0xFFFE,0xFFFE,0,0,0} header, {0xFFFF,0xFFFF,0,0,0}
// tail -- same convention as bo1/bo3).
static LayoutEntity D_us_80181AEC[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181B00[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0051, 0x003E, 0x0022, 0x0080, 0x0000},
    {0x0080, 0x0080, 0x0025, 0x0088, 0x0000},
    {0x008E, 0x0052, 0x0022, 0x0081, 0x0001},
    {0x00C8, 0x0058, 0x0022, 0x0082, 0x0002},
    {0x0100, 0x0080, 0x0016, 0x0010, 0x0000},
    {0x0100, 0x0080, 0x0023, 0x0090, 0x0000},
    {0x0100, 0x0080, 0x0023, 0x0094, 0x0001},
    {0x0138, 0x0058, 0x0022, 0x0083, 0x0002},
    {0x0172, 0x0052, 0x0022, 0x0084, 0x0001},
    {0x01B0, 0x003E, 0x0022, 0x0085, 0x0000},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181B78[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0480, 0x0080, 0x0009, 0x0080, 0x0000},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181B96[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0000, 0x0038, 0x0008, 0x0080, 0x0000},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181BB4[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0080, 0x0030, 0x0008, 0x0080, 0x0001},
    {0x0080, 0x00C0, 0x0008, 0x0088, 0x0002},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181BDC[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181BF0[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0051, 0x003E, 0x0022, 0x0080, 0x0000},
    {0x01B0, 0x003E, 0x0022, 0x0085, 0x0000},
    {0x008E, 0x0052, 0x0022, 0x0081, 0x0001},
    {0x0172, 0x0052, 0x0022, 0x0084, 0x0001},
    {0x00C8, 0x0058, 0x0022, 0x0082, 0x0002},
    {0x0138, 0x0058, 0x0022, 0x0083, 0x0002},
    {0x0080, 0x0080, 0x0025, 0x0088, 0x0000},
    {0x0100, 0x0080, 0x0016, 0x0010, 0x0000},
    {0x0100, 0x0080, 0x0023, 0x0090, 0x0000},
    {0x0100, 0x0080, 0x0023, 0x0094, 0x0001},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181C68[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0480, 0x0080, 0x0009, 0x0080, 0x0000},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181C86[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0000, 0x0038, 0x0008, 0x0080, 0x0000},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};
static LayoutEntity D_us_80181CA4[] = {
    {0xFFFE, 0xFFFE, 0, 0, 0},
    {0x0080, 0x0030, 0x0008, 0x0080, 0x0001},
    {0x0080, 0x00C0, 0x0008, 0x0088, 0x0002},
    {0xFFFF, 0xFFFF, 0, 0, 0},
};

// pStObjLayoutHorizontal/Vertical -- declared under their RAW names, not
// OVL_EXPORT-wrapped: create_entity.c is being kept as raw INCLUDE_ASM this
// checkpoint (see note above D_us_80180410), so its own frozen
// InitRoomEntities.s references these two tables by their original raw
// splat names directly (D_us_8018026C, D_us_80180340), and nothing else
// currently needs the friendly OBJ_LAYOUT_HORIZONTAL/VERTICAL macro name.
// Values re-verified via a Python re-read of the ROM after the first
// transcription (done by hand, into this same file) turned out to have two
// real bugs: horizontal was one entry short with BB4/B96 misplaced, and
// vertical had its last 4 entries reversed (CA4,CA4,C86,C86 written as
// C86,C86,CA4,CA4). Regenerated programmatically this time instead of by
// hand to eliminate transcription risk.
// Vertical is 52 entries, NOT 53: the real 53rd address (0x410) is
// D_us_80180410 above (the dispatch table's own -1 addressing base) --
// same asymmetric pattern as bo1, not the false "53/53 symmetric" claim
// from the first draft of this file.
LayoutEntity* D_us_8018026C[53] = {
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181B00, D_us_80181B78, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC,
    D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181AEC, D_us_80181BB4,
    D_us_80181BB4, D_us_80181B96, D_us_80181B96,
};
LayoutEntity* D_us_80180340[52] = {
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BF0, D_us_80181C68, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC,
    D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181BDC, D_us_80181CA4,
    D_us_80181CA4, D_us_80181C86,
};

// EInitBreakable (e_breakable.h), confirmed against D_us_801804B8 --
// {3,0,0,0,0} -- via EntityBreakable's real InitializeEntity call site.
EInit BO2_EInitBreakable = {0x0003, 0x0000, 0x0000, 0x0000, 0x0000};

// EInitObtainable (e_particles.h), confirmed against D_us_801804C4 --
// {3,0,0,0,1}, matching bo1/bo3's value for the same constant.
EInit BO2_EInitObtainable = {0x0003, 0x0000, 0x0000, 0x0000, 0x0001};

// g_EInitParticle (e_misc.h/e_collect.h) -- confirmed against ROM,
// {3,0,0,0,2}, matching bo1/bo3. Defined under the RAW name because
// 22610.c's frozen func_us_801A3CF0/func_us_801A4BA4 reference
// D_us_801804D0 directly (see bo2.h for the alias).
EInit D_us_801804D0 = {0x0003, 0x0000, 0x0000, 0x0000, 0x0002};

// g_EInitInteractable (e_life_up.h/e_stage_name_us.h) -- confirmed against
// ROM, {0,0,0,0,5}, matching bo1/bo3. Defined under the RAW name because
// 22610.c's func_us_801A3E04/func_us_801A460C and 337D0.c's func_us_801B56C0
// reference D_us_801804E8 directly (see bo2.h for the alias).
EInit D_us_801804E8 = {0x0000, 0x0000, 0x0000, 0x0000, 0x0005};

// g_EInitUnkId13 (e_misc.h) -- confirmed against ROM, {0,0,0,0,2}, matching
// bo1. Defined under the RAW name because e_misc.c's OWN frozen asm (it
// stays unmigrated this checkpoint) references D_us_801804F4 directly --
// missed on the first pass, which only checked 22610.c/337D0.c and assumed
// e_misc.c (being left untouched) couldn't conflict. Backwards: leaving a
// file untouched means its raw references still need to resolve too.
EInit D_us_801804F4 = {0x0000, 0x0000, 0x0000, 0x0000, 0x0002};

// g_EInitLockCamera (entity_lock_camera.h) -- confirmed against
// D_us_80180500 -- {0,0,0,0,1}, matching bo1/bo3. No raw-name conflict found.
EInit g_EInitLockCamera = {0x0000, 0x0000, 0x0000, 0x0000, 0x0001};

// g_EInitCommon (e_room_bg.h/e_red_door.h) -- confirmed against ROM,
// {0,0,0,0,3}, matching bo1/bo3. Defined under the RAW name because
// 22610.c's func_us_801A7340 references D_us_8018050C directly (see bo2.h
// for the alias).
EInit D_us_8018050C = {0x0000, 0x0000, 0x0000, 0x0000, 0x0003};

// g_EInitDamageNum (collision.h/EntityDamageDisplay) -- confirmed against
// D_us_80180518 -- {0,0,0,0,3}, matching bo1/bo3. No raw-name conflict found.
EInit g_EInitDamageNum = {0x0000, 0x0000, 0x0000, 0x0000, 0x0003};

// RedDoorTiles[2][8] -- confirmed byte-for-byte against the real ROM data at
// D_us_801805C8 (referenced by func_us_801A9E50/EntityRedDoor via
// `params << 4` row-stride addressing, matching [2][8] u16 layout exactly).
u16 OVL_EXPORT(RedDoorTiles)[][8] = {
    {0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x04FA, 0x04FA, 0x04FA, 0x04FA, 0x0000, 0x0000, 0x0000, 0x0000},
};

// BackgroundBlockInit's animFrames data (AnimateEntityFrame{duration,pose}
// pairs, sentinel-terminated {0,0}), transcribed byte-for-byte from
// D_us_80180590/D_us_80180594 -- confirmed via BackgroundBlockInit's own
// animFrames pointer fields (offset 0x10) matching these two addresses
// exactly.
static u8 D_us_80180590[] = {0x10, 0x0B, 0x00, 0x00};
static u8 D_us_80180594[] = {0x08, 0x0C, 0x08, 0x0D, 0x08, 0x0E,
                             0x08, 0x0F, 0x08, 0x10, 0x00, 0x00};

// BackgroundBlockInit (e_room_bg.h), 2 real entries -- confirmed via direct
// ROM reading: entry 2 onward reproduces RedDoorTiles' own bytes verbatim
// (D_us_801805C8, which begins exactly where a hypothetical entry 2 would
// start), confirming the table really only has 2 entries, same pattern as
// bo1's single-entry case. Field order/widths matched exactly against
// ObjInit2's real definition in game.h (u16,u16,u8,u8,u16,u16,u16,u32,ptr) --
// note drawFlags/blendMode are u16, NOT the byte-sized fields an earlier,
// incorrect reading of the raw asm's lbu/sb instructions suggested; the
// asm's byte-sized loads turned out to belong to facingLeft/unk5A (adjacent
// u8 fields), re-verified by re-reading the exact byte offsets directly.
ObjInit2 OVL_EXPORT(BackgroundBlockInit)[] = {
    {0x8001, 0x0069, 0x00, 0x00, 0x0000, 0x0000, 0x0000, 0x00000000, D_us_80180590},
    {0x8001, 0x0069, 0x00, 0x00, 0x0000, 0x0000, 0x0030, 0x00000000, D_us_80180594},
};
