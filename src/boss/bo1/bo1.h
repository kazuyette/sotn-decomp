// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef BO1_H
#define BO1_H

#include <stage.h>

#define OVL_EXPORT(x) BO1_##x

// Entity dispatch table (see e_init.c) is still referenced by its old raw
// splat-assigned name from create_entity.c/e_misc.c's frozen INCLUDE_ASM
// text (create_entity.c and e_misc.c are not migrated yet) — kept raw,
// aliased here, same pattern proven on bo3 (see bo3.h/e_init.c history in
// project memory).
#define BO1_EntityUpdates D_us_801809F4

// BO1_pStObjLayoutHorizontal/Vertical (create_entity.h's OBJ_LAYOUT_HORIZONTAL/
// VERTICAL), confirmed against InitRoomEntities' real disassembly and against
// direct ROM reading (see e_init.c for the length-asymmetry note: horizontal
// has 53 entries, vertical only 52). Kept raw, same reasoning as
// BO1_EntityUpdates above — cheap insurance against e_collect.c (still
// un-migrated) referencing either by raw name.
#define BO1_pStObjLayoutHorizontal D_us_80180850
#define BO1_pStObjLayoutVertical D_us_80180924

// g_EInitParticle (used by e_misc.h/e_collect.h) and D_us_80180AA4 (used
// directly by 20F60.c's own local extern, which does not include this
// header) are the exact same EInit data. Real definition lives in e_init.c
// under the raw name; alias the friendly name to it here so e_misc.c/
// e_collect.c resolve to that one symbol too.
extern EInit D_us_80180AA4;
#define g_EInitParticle D_us_80180AA4

// g_EInitCommon (used by e_room_bg.h), confirmed against bo1's real
// compiled bytes (D_us_80180AE0) — {0,0,0,0,3}, exactly matching bo3's
// value for the same constant. Kept under its raw name: still referenced
// that way from e_misc.c's frozen asm.
extern EInit D_us_80180AE0;
#define g_EInitCommon D_us_80180AE0

// g_EInitLockCamera (used by entity_lock_camera.h), confirmed against
// D_us_80180AD4 — {0,0,0,0,1}, matching bo3. No raw-name conflict found in
// the still-unmigrated files, declared directly.
extern EInit g_EInitLockCamera;

// EInitObtainable (used by EntitySoulStealOrb/EntityEnemyBlood in
// e_particles.h), confirmed against D_us_80180A98 — {3,0,0,0,1}, matching
// bo3's BO3_EInitObtainable value. Kept under its raw name: referenced 4x
// from e_misc.c's frozen asm (likely more particle-spawning call sites).
extern EInit D_us_80180A98;
#define BO1_EInitObtainable D_us_80180A98

// g_EInitInteractable (used by EntityStageNamePopup in e_stage_name.h),
// confirmed against D_us_80180ABC — {0,0,0,0,5}, matching bo3. Kept under
// its raw name: referenced from e_red_door.c's frozen asm.
extern EInit D_us_80180ABC;
#define g_EInitInteractable D_us_80180ABC

// g_EInitDamageNum (used by EntityDamageDisplay in st_collision.h),
// confirmed against D_us_80180AEC — {0,0,0,0,3}, matching bo3. No raw-name
// conflict found, declared directly.
extern EInit g_EInitDamageNum;

// Entity IDs for bo1 (Granfaloon), in exact order of the real dispatch
// table at file offset 0x9F4 (VRAM 0x801809F4) in disks/us/BOSS/BO1/BO1.BIN
// -- resolved directly from the ROM (same technique used throughout bo3),
// since the table lives inside the still-unidentified 133 KB head data
// blob and splat never named it. 38 entries (0x00-0x25). Slots whose
// function is Granfaloon-specific are named E_UNK_XX with the real
// function comment alongside, matching this project's convention (see
// src/boss/bo0/bo0.h, src/boss/bo3/bo3.h).
typedef enum {
    /* 0x00 */ E_NONE, // data ptr (0x80181D2A), not a function, never called
    /* 0x01 */ E_UNK_01, // func_us_801A0F60 - Granfaloon debris/breakable trigger
    /* 0x02 */ E_EXPLOSION,
    /* 0x03 */ E_PRIZE_DROP,
    /* 0x04 */ E_DAMAGE_DISPLAY,
    /* 0x05 */ E_RED_DOOR, // confirmed = shared EntityRedDoor (e_red_door.c)
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
    /* 0x16 */ E_UNK_16, // func_us_801A1334 - Granfaloon body state machine
    /* 0x17 */ E_UNK_17, // func_us_801A10A8 - Granfaloon debris effect
    /* 0x18 */ E_UNK_18, // func_us_801A1878 - Granfaloon combat "conductor"
    /* 0x19 */ E_UNK_19, // func_us_801A2F2C
    /* 0x1A */ E_UNK_1A, // func_us_801A3480 - tentacle segment
    /* 0x1B */ E_UNK_1B, // func_us_801A38EC - eye/chain quad renderer
    /* 0x1C */ E_UNK_1C, // func_us_801A2774
    /* 0x1D */ E_UNK_1D, // func_us_801A2BC4
    /* 0x1E */ E_UNK_1E, // func_us_801A2CC4
    /* 0x1F */ E_UNK_1F, // func_us_801A2D90
    /* 0x20 */ E_UNK_20, // func_us_801A4394
    /* 0x21 */ E_UNK_21, // func_us_801A45D0
    /* 0x22 */ E_UNK_22, // func_us_801A518C - shared debris/spark library
    /* 0x23 */ E_UNK_23, // func_us_801A493C
    /* 0x24 */ E_UNK_24, // func_us_801A4AF4 - chain/rope ribbon renderer
    /* 0x25 */ E_LIFE_UP_SPAWN,
} EntityIDs;

#endif // BO1_H
