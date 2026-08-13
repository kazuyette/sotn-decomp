// SPDX-License-Identifier: AGPL-3.0-or-later
#include "dre.h"

u8 gfx_stage_name_jp_sm[] = {
#include "gen/gfx_stage_name_jp_sm.h"
};

u8 gfx_stage_name_jp_lg[] = {
#include "gen/gfx_stage_name_jp_lg.h"
};

u8 gfx_succubus_1[] = {
#include "gen/gfx_succubus_1.h"
};

u8 gfx_succubus_2[] = {
#include "gen/gfx_succubus_2.h"
};

u8 gfx_succubus_3[] = {
#include "gen/gfx_succubus_3.h"
};

u8 gfx_succubus_4[] = {
#include "gen/gfx_succubus_4.h"
};

u8 gfx_succubus_5[] = {
#include "gen/gfx_succubus_5.h"
};

u8 gfx_succubus_6[] = {
#include "gen/gfx_succubus_6.h"
};

u8 gfx_succubus_7[] = {
#include "gen/gfx_succubus_7.h"
};

u8 gfx_succubus_lisa[] = {
#include "gen/gfx_succubus_lisa.h"
};

u8 gfx_portrait_alucard[] = {
#include "gen/gfx_portrait_alucard.h"
};

u8 gfx_portrait_lisa[] = {
#include "gen/gfx_portrait_lisa.h"
};

u8 gfx_portrait_succubus[] = {
#include "gen/gfx_portrait_succubus.h"
};

#ifdef VERSION_US
u8 gfx_stage_name_en[] = {
#include "gen/gfx_stage_name_en.h"
};
#endif

u16 pal_2E0[] = {
#include "gen/pal_2E0.h"
};

u16 pal_succubus[] = {
#include "gen/pal_succubus.h"
};

u16 pal_portrait_alucard[] = {
#include "gen/pal_portrait_alucard.h"
};

u16 pal_portrait_lisa[] = {
#include "gen/pal_portrait_lisa.h"
};

u16 pal_portrait_succubus[] = {
#include "gen/pal_portrait_succubus.h"
};


__asm__(
    ".global D_hd_80181F4C\n"
    "D_hd_80181F4C = gfx_stage_name_jp_sm + 0x250\n"
    ".global D_hd_801824F0\n"
    "D_hd_801824F0 = gfx_stage_name_jp_sm + 0x7F4\n"
    ".global D_hd_80182F2C\n"
    "D_hd_80182F2C = gfx_stage_name_jp_sm + 0x1230\n"
    ".global D_hd_801839F8\n"
    "D_hd_801839F8 = gfx_stage_name_jp_sm + 0x1CFC\n"
    ".global D_hd_80184698\n"
    "D_hd_80184698 = gfx_stage_name_jp_sm + 0x299C\n"
    ".global D_hd_801852EC\n"
    "D_hd_801852EC = gfx_stage_name_jp_sm + 0x35F0\n"
    ".global D_hd_80185E6C\n"
    "D_hd_80185E6C = gfx_stage_name_jp_sm + 0x4170\n"
    ".global D_hd_80186A1C\n"
    "D_hd_80186A1C = gfx_stage_name_jp_sm + 0x4D20\n"
    ".global D_hd_801874DC\n"
    "D_hd_801874DC = gfx_stage_name_jp_sm + 0x57E0\n"
    ".global D_hd_8018A858\n"
    "D_hd_8018A858 = gfx_stage_name_jp_sm + 0x8B5C\n"
    ".global D_hd_8018AB18\n"
    "D_hd_8018AB18 = gfx_stage_name_jp_sm + 0x8E1C\n"
    ".global D_hd_8018ABB8\n"
    "D_hd_8018ABB8 = gfx_stage_name_jp_sm + 0x8EBC\n"
    ".global D_hd_8018ADB8\n"
    "D_hd_8018ADB8 = gfx_stage_name_jp_sm + 0x90BC\n"
    ".global D_hd_8018AFB8\n"
    "D_hd_8018AFB8 = gfx_stage_name_jp_sm + 0x92BC\n"
    ".global func_80188218\n"
    "func_80188218 = 0x80196B40\n"
    ".global func_8018861C\n"
    "func_8018861C = 0x80196F44\n"
    ".global func_80188514\n"
    "func_80188514 = 0x80196E3C\n"
    ".global func_8018A634\n"
    "func_8018A634 = 0x80198F5C\n"
    ".global func_8018A7AC\n"
    "func_8018A7AC = 0x801990D4\n"
);
