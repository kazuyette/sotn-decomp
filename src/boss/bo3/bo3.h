// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef BO3_H
#define BO3_H

#include <stage.h>

#define OVL_EXPORT(x) BO3_##x

// st_common.h's ReplaceBreakableWithItemDrop()/EntityExplosionSpawn() helpers
// assign these three generic names directly to entity->pfnUpdate (bypassing
// the ID dispatch table) — but Scylla has its own custom-named implementations
// for these same conceptual entity types (already decompiled in 2AFCC.c under
// their real func_us_ names). Without these aliases, migrating st_common.c
// fails at link time with "undefined reference to EntityExplosion" etc.
// (confirmed via a real WSL build). Safe: same signature, same real function.
extern void func_us_801ABDB0(Entity* self);
extern void func_us_801AB53C(Entity* self);
extern void func_us_801ABF50(Entity* self);
#define EntityExplosion func_us_801ABDB0
#define EntityPrizeDrop func_us_801AB53C
#define EntityEquipItemDrop func_us_801ABF50

// Entity IDs for bo3 (Scylla), in exact order of the real dispatch table at
// asm/us/boss/bo3/data/0.data.s (glabel D_us_80180424). Slots whose function
// is Scylla-specific (not yet mapped to a shared/common name) are named
// E_UNK_XX with the real function comment alongside, matching this project's
// convention for undocumented per-boss entity slots (see src/boss/bo0/bo0.h).
typedef enum {
    /* 0x00 */ E_NONE, // D_us_80181E58 (data ptr, not a function)
    /* 0x01 */ E_BREAKABLE,
    /* 0x02 */ E_EXPLOSION,
    /* 0x03 */ E_PRIZE_DROP,
    /* 0x04 */ E_DAMAGE_DISPLAY,
    /* 0x05 */ E_RED_DOOR,
    /* 0x06 */ E_INTENSE_EXPLOSION,
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
    /* 0x11 */ E_BACKGROUND_BLOCK,
    /* 0x12 */ E_LOCK_CAMERA,
    /* 0x13 */ E_UNK_ID13,
    /* 0x14 */ E_EXPLOSION_VARIANTS,
    /* 0x15 */ E_GREY_PUFF,
    /* 0x16 */ E_DUMMY_16,
    /* 0x17 */ E_UNK_17, // func_us_801A1EE4 - splash/debris chunk
    /* 0x18 */ E_DUMMY_18,
    /* 0x19 */ E_DUMMY_19,
    /* 0x1A */ E_DUMMY_1A,
    /* 0x1B */ E_UNK_1B, // func_us_801A27EC
    /* 0x1C */ E_UNK_1C, // func_us_8019D1D4
    /* 0x1D */ E_UNK_1D, // func_us_8019D51C
    /* 0x1E */ E_DUMMY_1E,
    /* 0x1F */ E_DUMMY_1F,
    /* 0x20 */ E_DUMMY_20,
    /* 0x21 */ E_DUMMY_21,
    /* 0x22 */ E_DUMMY_22,
    /* 0x23 */ E_DUMMY_23,
    /* 0x24 */ E_DUMMY_24,
    /* 0x25 */ E_DUMMY_25,
    /* 0x26 */ E_DUMMY_26,
    /* 0x27 */ E_DUMMY_27,
    /* 0x28 */ E_DUMMY_28,
    /* 0x29 */ E_DUMMY_29,
    /* 0x2A */ E_DUMMY_2A,
    /* 0x2B */ E_DUMMY_2B,
    /* 0x2C */ E_DUMMY_2C,
    /* 0x2D */ E_UNK_2D, // func_us_801A07CC - water level rise/fall controller
    /* 0x2E */ E_UNK_2E, // func_us_801A0A80 (deferred)
    /* 0x2F */ E_UNK_2F, // func_us_801A1120 - splash/debris chunk
    /* 0x30 */ E_UNK_30, // func_us_801A16E4 - water ripple
    /* 0x31 */ E_UNK_31, // func_us_801A2AEC (deferred) - multi-part death sequence
    /* 0x32 */ E_UNK_32, // func_us_801A3CD8
    /* 0x33 */ E_UNK_33, // func_us_801A365C (deferred)
    /* 0x34 */ E_UNK_34, // func_us_801A3EE0 - rotating heraldic disc projectile
    /* 0x35 */ E_UNK_35, // func_us_801A42A8 - acid-spit trail
    /* 0x36 */ E_UNK_36, // func_us_801A4680
    /* 0x37 */ E_UNK_37, // func_us_801A4988
    /* 0x38 */ E_UNK_38, // func_us_801A4C0C
    /* 0x39 */ E_UNK_39, // func_us_801A4E24
    /* 0x3A */ E_UNK_3A, // func_us_801A5338 (deferred) - body-chain spawner
    /* 0x3B */ E_UNK_3B, // func_us_801A57A4
    /* 0x3C */ E_UNK_3C, // func_us_801A5948 (deferred)
    /* 0x3D */ E_UNK_3D, // func_us_801A5F0C - falling debris/shatter
    /* 0x3E */ E_UNK_3E, // func_us_801A1BE8 - rising/falling platform
    /* 0x3F */ E_UNK_3F, // func_us_801A61F0 - falling debris chunk
    /* 0x40 */ E_LIFE_UP_SPAWN,
    /* 0x41 */ E_UNK_41, // func_us_801A6370 - boss-defeat music sequencer
    /* 0x42 */ E_UNK_42, // func_us_801A6568 - boss-defeat music sequencer (sibling)
    /* 0x43 */ E_UNK_43, // func_us_8019D160
} EntityIDs;

// pfn_entity_update.h / create_entity.h reference the dispatch table through
// the generic macro `PfnEntityUpdates`, which on PS1 expands to
// `OVL_EXPORT(EntityUpdates)` = `BO3_EntityUpdates`. Our real table stays
// named `D_us_80180424` (see e_init.c's comment for why: create_entity.c and
// e_misc.c still reference it under that raw name in frozen asm) — so alias
// the generic macro's expansion to the real name here, same #define-alias
// pattern used throughout this file. Must come before
// `#include "../../st/pfn_entity_update.h"` in any migrated .c file.
#define BO3_EntityUpdates D_us_80180424

// OBJ_LAYOUT_HORIZONTAL/VERTICAL (pfn_entity_update.h's
// OVL_EXPORT(pStObjLayoutHorizontal)/OVL_EXPORT(pStObjLayoutVertical)):
// bo3's Overlay struct instance (D_us_80180000, still an unidentified data
// blob) references the horizontal table by its raw name D_us_80180280 —
// confirmed via a real WSL build ("undefined reference to D_us_80180280").
// Kept raw + aliased, same pattern as BO3_EntityUpdates above.
#define BO3_pStObjLayoutHorizontal D_us_80180280
#define BO3_pStObjLayoutVertical D_us_80180354

// g_EInitCommon (used by e_room_bg.h) and g_EInitLockCamera (used by
// entity_lock_camera.h) confirmed against bo3's real compiled bytes
// (D_us_80180588 / D_us_8018057C in asm/us/boss/bo3/data/0.data.s) —
// {0,0,0,0,3} and {0,0,0,0,1} respectively, defined in e_init.c.
//
// g_EInitCommon is aliased to D_us_80180588's real name rather than declared
// directly: 1CEEC.c's own already-decompiled code references this same
// address as `D_us_80180588` (one of the 6 known baseline "conflicting
// types" symbols) — renaming it broke the real WSL link the first time
// around. Same fix pattern as EntityExplosion/EntityPrizeDrop above.
extern EInit D_us_80180588;
#define g_EInitCommon D_us_80180588
extern EInit g_EInitLockCamera;

// g_EInitInteractable and OVL_EXPORT(EInitObtainable), confirmed against real
// InitializeEntity call sites (EntityStageNamePopup -> D_us_80180564,
// EntitySoulStealOrb/EntityEnemyBlood -> D_us_80180540) — {0,0,0,0,5} and
// {0x0003,0,0,0,1} respectively, both exactly matching mar/rbo3's values.
// Same raw-name-alias treatment as g_EInitCommon: D_us_80180564 is already
// used ~30 times throughout 1CEEC.c, and D_us_80180540 is already used in
// 2AFCC.c — renaming either would break the real WSL link again.
extern EInit D_us_80180564;
#define g_EInitInteractable D_us_80180564
extern EInit D_us_80180540;
#define BO3_EInitObtainable D_us_80180540

// g_EInitDamageNum, confirmed against EntityDamageDisplay's real
// InitializeEntity(D_us_80180594) call — {0,0,0,0,3}. No existing raw-name
// reference found in 1CEEC.c/2AFCC.c, so declared directly (no alias needed).
extern EInit g_EInitDamageNum;

// g_EInitParticle / g_EInitUnkId13 deliberately NOT declared here yet — only
// needed by e_misc.c, which is NOT a migration candidate at all (it's
// genuinely mixed with boss-specific code, incl. EntityOlroxDrool — see
// project memory). No longer blocking anything in the current migration set.

#endif // BO3_H
