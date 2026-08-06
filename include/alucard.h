// SPDX-License-Identifier: AGPL-3.0-or-later
#ifndef ALUCARD_H
#define ALUCARD_H

// Entity IDs for entities created/used by Alucard's (DRA) overlay.
// Moved out of src/dra/dra.h (see GitHub issue #2982) so that stage/boss
// overlays can reference these entity IDs (e.g. entity->entityId checks)
// without pulling in dra.h's much heavier dependency chain
// (disk.h/game.h/weapon.h/items.h and DRA-specific declarations).
enum AluEntities {
    E_NONE,                         // EntityNull
    E_FACTORY,                      // EntityFactory
    E_SMOKE_PUFF,                   // EntitySmokePuff
    E_GRAVITY_BOOT_BEAM,            // EntityGravityBootBeam
    E_KNIFE,                        // EntitySubwpnKnife
    E_UNK_5,                        // func_8011E4BC
    E_UNK_6,                        // EntityDiveKickAttack
    E_UNK_7,                        // EntityGiantSpinningCross
    E_CRASH_CROSS_ROTATING,         // EntitySubwpnCrashCross
    E_SUBWPN_CRASH_CROSS_PARTICLES, // EntitySubwpnCrashCrossParticles
    E_SUBWPN_AXE,                   // EntitySubwpnThrownAxe
    E_BLINK_WHITE,                  // EntityPlayerBlinkWhite
    E_SUBWPN_VIBHUTI,               // EntitySubwpnThrownVibhuti
    E_UNK_13,                       // func_8011E0E4
    E_UNK_14,                       // func_8011EDA0
    E_UNK_15,                       // EntityUnarmedAttack
    E_UNK_16,                       // func_8011EDA8
    E_SUBWPN_AGUNEA,                // EntitySubwpnAgunea
    E_SUBWPN_AGUNEA_HIT_ENEMY,      // EntityAguneaHitEnemy
    E_UNK_19,                       // EntityNumberMovesToHpMeter
    E_SUBWPN_REBOUND_STONE,         // EntitySubwpnReboundStone
    E_UNK_21,                       // EntityLevelUpAnimation
    E_SUBWPN_HOLY_WATER,            // EntitySubwpnHolyWater
    E_SUBWPN_HOLY_WATER_FLAME,      // EntitySubwpnHolyWaterFlame
    E_UNK_24,                       // EntityUnkId24
    E_HELLFIRE,                     // EntityHellfire
    E_HELLFIRE_SMALL_FIREBALL,      // EntityHellfireNormalFireball
    E_HELLFIRE_BIG_FIREBALL,        // EntityHellfireBigFireball
    E_UNK_28,                       // EntityExpandingCircle
    E_UNK_29,                       // func_80127CC8
    E_HIT_BY_LIGHTNING,             // EntityHitByLightning
    E_PLAYER_OUTLINE,               // EntityPlayerOutline
    E_UNK_32,                       // EntityPlayerDissolves
    E_HIT_BY_ICE,                   // EntityHitByIce
    E_MIST,                         // EntityMist
    E_WING_SMASH_TRAIL,             // EntityWingSmashTrail
    E_UNK_36,                       // func_8011B480
    E_UNK_37,                       // EntityGuardText
    E_UNK_38,                       // EntityTransparentWhiteCircle
    E_UNK_39,                       // EntityPlayerPinkEffect
    E_HOLYWATER_BREAK_GLASS,        // EntitySubwpnHolyWaterBreakGlass
    E_SUBWPN_STOPWATCH,             // EntityStopWatch
    E_SUBWPN_STOPWATCH_CIRCLE,      // EntityStopWatchExpandingCircle
    E_SUBWPN_BIBLE,                 // EntitySubwpnBible
    E_SUBWPN_BIBLE_TRAIL,           // EntitySubwpnBibleTrail
    E_BAT_FIREBALL,                 // EntityBatFireball
    E_UNK_46,                       // func_80123B40
    E_UNK_47,                       // func_80119F70
    E_UNK_48,                       // UnknownEntId48
    E_UNK_49,                       // UnknownEntId49
    E_UNK_50,                       // func_80123A60
    E_SMALL_RISING_HEART,           // EntitySmallRisingHeart
    E_BAT_ECHO,                     // EntityBatEcho
    E_UNK_53,                       // func_8011B530
    E_UNK_54,                       // func_8011F074
    E_UNK_55,                       // func_80130264
    E_UNK_56,                       // func_8012F894
    E_UNK_57,                       // func_80130618
    E_UNK_58,                       // func_801309B4
    E_UNK_59,                       // func_80130E94
    E_UNK_60,                       // func_8013136C
    E_UNK_61,                       // func_80129864
    E_UNK_62,                       // EntityNull
    E_SUMMON_SPIRIT,                // EntitySummonSpirit
    E_UNK_64,                       // func_80123F78
    E_TELEPORT,                     // EntityTeleport
    E_SLEEP_ZZZ,                    // EntityPlayerSleepZ
    E_UNK_67,                       // EntityNull
    NUM_ENTITIES,

    E_UNK_238 = 238, // ???
    E_UNK_254 = 254, // ???

};

#endif
