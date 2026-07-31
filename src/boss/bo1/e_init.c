// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo1.h"

void func_us_801A0F60(Entity* self);
void EntityExplosion(Entity* self);
void EntityPrizeDrop(Entity* self);
void EntityDamageDisplay(Entity* self);
void OVL_EXPORT(EntityRedDoor)(Entity* self);
void EntityIntenseExplosion(Entity* self);
void EntitySoulStealOrb(Entity* self);
void EntityRoomForeground(Entity* self);
void EntityStageNamePopup(Entity* self);
void EntityEquipItemDrop(Entity* self);
void EntityRelicOrb(Entity* self);
void EntityHeartDrop(Entity* self);
void EntityEnemyBlood(Entity* self);
void EntityMessageBox(Entity* self);
void EntityDummy(Entity* self);
void OVL_EXPORT(EntityBackgroundBlock)(Entity* self);
void OVL_EXPORT(EntityLockCamera)(Entity* self);
void EntityUnkId13(Entity* self);
void EntityExplosionVariants(Entity* self);
void EntityGreyPuff(Entity* self);
void func_us_801A1334(Entity* self);
void func_us_801A10A8(Entity* self);
void func_us_801A1878(Entity* self);
void func_us_801A2F2C(Entity* self);
void func_us_801A3480(Entity* self);
void func_us_801A38EC(Entity* self);
void func_us_801A2774(Entity* self);
void func_us_801A2BC4(Entity* self);
void func_us_801A2CC4(Entity* self);
void func_us_801A2D90(Entity* self);
void func_us_801A4394(Entity* self);
void func_us_801A45D0(Entity* self);
void func_us_801A518C(Entity* self);
void func_us_801A493C(Entity* self);
void func_us_801A4AF4(Entity* self);
void EntityLifeUpSpawn(Entity* self);

// Room layout data for InitRoomEntities/CreateEntity* (create_entity.h),
// transcribed byte-for-byte from disks/us/BOSS/BO1/BO1.BIN (12 unique
// LayoutEntity blobs, contiguous at D_us_80181BCC-D_us_80181D84: 6 for
// horizontal followed immediately by 6 for vertical). Each list is
// sentinel-terminated ({LAYOUT_OBJ_START,LAYOUT_OBJ_START,0,0,0} head,
// {LAYOUT_OBJ_END,LAYOUT_OBJ_END,0,0,0} tail), found via terminator-scanning
// the real ROM bytes, same technique as bo3's e_init.c. Kept as raw byte
// blobs (not typed LayoutEntity[]) since only byte-exact values matter here.
u8 D_us_80181BCC[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181BE0[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
    0x41, 0x01, 0x16, 0x00, 0x80, 0x00, 0x01, 0x00, 0x07, 0x00, 0x41, 0x01,
    0x16, 0x00, 0x81, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x18, 0x00,
    0x10, 0x00, 0x00, 0x00, 0xF7, 0x01, 0x41, 0x00, 0x16, 0x00, 0x81, 0x00,
    0x00, 0x00, 0xF7, 0x01, 0x41, 0x00, 0x16, 0x00, 0x80, 0x00, 0x01, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181C26[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
    0x30, 0x00, 0x08, 0x00, 0x80, 0x00, 0x01, 0x00, 0x80, 0x00, 0xC0, 0x00,
    0x08, 0x00, 0x88, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181C4E[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181C6C[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x02,
    0x61, 0x00, 0x05, 0x00, 0x92, 0x00, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181C8A[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF8, 0x00,
    0x61, 0x00, 0x05, 0x00, 0x92, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181CA8[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181CBC[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7, 0x01,
    0x41, 0x00, 0x16, 0x00, 0x81, 0x00, 0x00, 0x00, 0xF7, 0x01, 0x41, 0x00,
    0x16, 0x00, 0x80, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x18, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x07, 0x00, 0x41, 0x01, 0x16, 0x00, 0x80, 0x00,
    0x01, 0x00, 0x07, 0x00, 0x41, 0x01, 0x16, 0x00, 0x81, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181D02[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
    0x30, 0x00, 0x08, 0x00, 0x80, 0x00, 0x01, 0x00, 0x80, 0x00, 0xC0, 0x00,
    0x08, 0x00, 0x88, 0x00, 0x02, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181D2A[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181D48[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x02,
    0x61, 0x00, 0x05, 0x00, 0x92, 0x00, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

u8 D_us_80181D66[] = {
    0xFE, 0xFF, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xF8, 0x00,
    0x61, 0x00, 0x05, 0x00, 0x92, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// BO1_pStObjLayoutHorizontal: per-objLayoutId pointer table, transcribed
// from D_us_80180850. Confirmed against InitRoomEntities' real disassembly
// (indexed by objLayoutId*4). 53 entries -- see splat.us.bobo1.yaml for why
// this doesn't match the vertical table's length.
//
// Kept under the raw name (not the friendly OVL_EXPORT one): matches bo3's
// same treatment of D_us_80180280, cheap insurance in case some not-yet
// migrated file (e_collect.c) ends up referencing it by raw name too.
LayoutEntity* D_us_80180850[53] = {
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BE0, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181C26, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181C26, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181C8A, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181C6C,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181BCC,
    (LayoutEntity*)D_us_80181BCC, (LayoutEntity*)D_us_80181C4E,
    (LayoutEntity*)D_us_80181C4E,
};

// BO1_pStObjLayoutVertical: same as above but 52 entries, transcribed from
// D_us_80180924. Ends exactly where the entity dispatch table begins
// (D_us_801809F4, see below) -- confirmed by direct ROM reading that the
// bytes right after this table's 52nd entry are func_us_801A0F60 (dispatch
// slot 1), not a 53rd room entry.
LayoutEntity* D_us_80180924[52] = {
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CBC, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181D02, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181D02, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181D66, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181D48,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181CA8,
    (LayoutEntity*)D_us_80181CA8, (LayoutEntity*)D_us_80181D2A,
};

// g_EInitCommon / g_EInitLockCamera / EInitObtainable / g_EInitInteractable /
// g_EInitDamageNum, confirmed byte-for-byte against bo1's real compiled
// data (disks/us/BOSS/BO1/BO1.BIN, offsets computed as VRAM - 0x80180000):
// {0,0,0,0,3} / {0,0,0,0,1} / {3,0,0,0,1} / {0,0,0,0,5} / {0,0,0,0,3} --
// all exactly matching the values already confirmed on bo3 for these same
// 5 constants (see bo1.h for which ones are kept under raw names due to
// still-unmigrated files referencing them).
EInit D_us_80180AE0 = {0x0000, 0x0000, 0x0000, 0x0000, 0x0003};
EInit g_EInitLockCamera = {0x0000, 0x0000, 0x0000, 0x0000, 0x0001};
EInit D_us_80180A98 = {0x0003, 0x0000, 0x0000, 0x0000, 0x0001};
EInit D_us_80180ABC = {0x0000, 0x0000, 0x0000, 0x0000, 0x0005};
EInit g_EInitDamageNum = {0x0000, 0x0000, 0x0000, 0x0000, 0x0003};

// g_EInitParticle / D_us_80180AA4, confirmed against ROM -- {3,0,0,0,2}.
// TWO different callers need TWO different literal names for this exact same
// data: e_misc.h and e_collect.h both extern it as the friendly name
// `g_EInitParticle` (not OVL_EXPORT-wrapped, same convention as every other
// stage's e_init.c e.g. src/st/are/e_init.c), while 20F60.c (Granfaloon's
// frozen boss-specific behavior code from an earlier checkpoint, does not
// include bo1.h) has its own local `extern u16 D_us_80180AA4[];` and calls
// InitializeEntity(D_us_80180AA4) directly by raw name. A single real
// definition can only carry one literal name, so it's defined here under the
// RAW name and bo1.h aliases the friendly name to it (`#define
// g_EInitParticle D_us_80180AA4`) -- same pattern as g_EInitCommon/
// g_EInitInteractable above. (An earlier pass tried the opposite: defining it
// as g_EInitParticle with no raw alias, which built fine until the next WSL
// attempt hit "undefined reference to D_us_80180AA4" from 20F60.c -- that
// file was never touched by this checkpoint's migration work, so its raw
// reference has to keep working.)
EInit D_us_80180AA4 = {0x0003, 0x0000, 0x0000, 0x0000, 0x0002};

// animFrames table for BackgroundBlockInit, transcribed byte-for-byte from
// D_us_80180B84 (falls inside the still-unsplit data blob with no linkable
// symbol on its own -- defined here matching the real bytes exactly, same
// pattern as bo3's D_us_8018060C/D_us_80180610).
//
// Correction: an earlier pass of this checkpoint mistakenly transcribed
// this as {0x06, 0x01, 0xFA, 0x01, 0x00, 0x00} -- those are actually the
// TABLE's own animSet/zPriority fields (see BackgroundBlockInit below),
// not D_us_80180B84's real bytes. Re-extracted directly from the ROM
// (4 bytes, ending exactly where the table starts at 0xB88): the real
// content is {0x40, 0x01, 0xFF, 0x00} (a single frame/duration pair
// terminated by 0xFF, same encoding as bo3's animFrames arrays).
u8 D_us_80180B84[] = {0x40, 0x01, 0xFF, 0x00};

// BackgroundBlockInit, transcribed byte-for-byte from D_us_80180B88.
// Only 1 entry defined: the second candidate slot's raw bytes
// (animSet=zPriority=unk5A=0x2020, palette=0x2050 -- all fields sharing
// one suspicious repeating value) don't look like real ObjInit2 data,
// suggesting bo1 only ever uses self->params==0 for E_BACKGROUND_BLOCK.
// Not independently confirmed against a real InitRoomEntities layout
// walk -- flag if a real WSL build ever indexes past entry 0.
ObjInit2 OVL_EXPORT(BackgroundBlockInit)[] = {
    {0x0006, 0x01FA, 0, 0, 0x0000, 0x0010, 0x0000, 0x00000000, D_us_80180B84},
};

// Exact order + real addresses resolved directly from disks/us/BOSS/BO1/
// BO1.BIN's rodata (file offset 0x9F4 = VRAM 0x801809F4), since the table
// lives inside the still-unidentified head data blob and splat never named
// it -- same ROM-extraction technique used throughout bo3. 38 entries
// (0x00-0x25). See bo1.h for the EntityIDs enum documenting each slot.
//
// NAMED `D_us_801809F4` (not the friendlier OVL_EXPORT name) on purpose:
// create_entity.c and e_misc.c are still raw, un-migrated INCLUDE_ASM
// stubs whose frozen .s text already references this table by this raw
// name (confirmed via grep, 4 references) -- renaming would break the
// real WSL link, same lesson learned on bo3's D_us_80180424.
PfnEntityUpdate D_us_801809F4[] = {
    (PfnEntityUpdate) 0x80181D2A, // 0x00 E_NONE (data ptr, never actually called)
    func_us_801A0F60,        // 0x01
    EntityExplosion,          // 0x02 E_EXPLOSION
    EntityPrizeDrop,          // 0x03 E_PRIZE_DROP
    EntityDamageDisplay,      // 0x04 E_DAMAGE_DISPLAY
    OVL_EXPORT(EntityRedDoor), // 0x05 E_RED_DOOR
    EntityIntenseExplosion,   // 0x06 E_INTENSE_EXPLOSION
    EntitySoulStealOrb,       // 0x07 E_SOUL_STEAL_ORB
    EntityRoomForeground,     // 0x08 E_ROOM_FOREGROUND
    EntityStageNamePopup,     // 0x09 E_STAGE_NAME_POPUP
    EntityEquipItemDrop,      // 0x0A E_EQUIP_ITEM_DROP
    EntityRelicOrb,           // 0x0B E_RELIC_ORB
    EntityHeartDrop,          // 0x0C E_HEART_DROP
    EntityEnemyBlood,         // 0x0D E_ENEMY_BLOOD
    EntityMessageBox,         // 0x0E E_MESSAGE_BOX
    EntityDummy,               // 0x0F
    EntityDummy,               // 0x10
    OVL_EXPORT(EntityBackgroundBlock), // 0x11 E_BACKGROUND_BLOCK
    OVL_EXPORT(EntityLockCamera),      // 0x12 E_LOCK_CAMERA
    EntityUnkId13,             // 0x13
    EntityExplosionVariants,   // 0x14
    EntityGreyPuff,            // 0x15
    func_us_801A1334,        // 0x16
    func_us_801A10A8,        // 0x17
    func_us_801A1878,        // 0x18
    func_us_801A2F2C,        // 0x19
    func_us_801A3480,        // 0x1A
    func_us_801A38EC,        // 0x1B
    func_us_801A2774,        // 0x1C
    func_us_801A2BC4,        // 0x1D
    func_us_801A2CC4,        // 0x1E
    func_us_801A2D90,        // 0x1F
    func_us_801A4394,        // 0x20
    func_us_801A45D0,        // 0x21
    func_us_801A518C,        // 0x22
    func_us_801A493C,        // 0x23
    func_us_801A4AF4,        // 0x24
    EntityLifeUpSpawn,         // 0x25 E_LIFE_UP_SPAWN
};

// NOTE: 2nd checkpoint (this pass) migrated the remaining 4 candidate files
// -- create_entity.c, e_misc.c, e_red_door.c, e_life_up.c -- all turned out
// to be 100% shared after real structural verification against the asm
// (contrary to the bo3 precedent where e_life_up.c was boss-specific; here
// it matched byte-for-byte, as did e_red_door.c's func_us_801A7D80, which
// turned out to just be EntityRedDoor itself -- confirmed via its
// g_eRedDoorUV table matching the shared header's hardcoded values exactly).
// Only e_collect.c remains un-migrated: genuinely new shared-engine work
// never done on any overlay before, not a migration -- see project memory.
