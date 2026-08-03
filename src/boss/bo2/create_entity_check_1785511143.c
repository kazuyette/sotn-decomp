// SPDX-License-Identifier: AGPL-3.0-or-later
#include "common.h"
#include "bo2.h"

// This file mirrors src/st/create_entity.h's generic room-layout/entity-
// creation machinery, but is hand-written rather than #include-ing that
// shared header: cross-checking every function here against its real
// disassembly (asm/us/boss/bo2/nonmatchings/create_entity/*.s) confirmed the
// logic is byte-for-byte identical to the shared header, EXCEPT that this
// overlay's own local storage for the four `static` scratch variables
// (g_LayoutObjHorizontal/g_LayoutObjVertical/g_LayoutObjPosHorizontal/
// g_LayoutObjPosVertical) landed at fixed raw addresses that bo2's own
// 22610.c/337D0.c do NOT reference directly -- so there was no forcing
// function to migrate this file to the shared header the way other bo2
// files were. Kept as its own translation unit for now; a future session
// could still fold it into the shared header as a normal #include if that
// turns out clean.
//
// IMPORTANT gotcha resolved this session: what earlier looked like "8
// mysterious func_us_ stub functions" (task tracked as create_entity.c's own
// investigation item) turned out to be pure splat function-boundary
// mis-splits, not real distinct functions:
//   - func_us_801A9808 / func_us_801A9854 are literally the first
//     instruction of FindFirstEntityAbove / FindFirstEntityBelow
//     respectively (each function loads its own LAYOUT_OBJ_START/END
//     sentinel constant into a register before the sign-extension of its
//     posY argument runs, and splat's boundary heuristic drew the line one
//     instruction too early).
//   - func_us_801A98AC / func_us_801A99A8 / func_us_801A9ABC /
//     func_us_801A9C34 / func_us_801A9CE8 / func_us_801A9D5C are each
//     exactly the missing first instruction (a compiler-hoisted `lui` or
//     the stack-frame `addiu sp,sp,-0x20`) of CreateEntitiesAbove /
//     CreateEntitiesBelow / InitRoomEntities / UpdateRoomPosition /
//     CreateEntityFromCurrentEntity / CreateEntityFromEntity respectively.
// Confirmed via: (1) address adjacency (each stub's address + its single
// instruction's size == the following named function's start address), (2)
// call-site cross-reference (nothing outside this file ever calls the first
// six stub names; UpdateRoomPosition itself calls `func_us_801A98AC`/
// `func_us_801A99A8` where the shared header calls CreateEntitiesAbove/
// Below directly, proving those raw names ARE those functions, just
// referenced by their true first-instruction address). Writing the complete,
// correct C body for each real function (including its previously-orphaned
// first instruction) makes the compiler emit that instruction at exactly
// the address the stub used to squat on, so no separate stub definition is
// needed at all -- see bo2.h for how func_us_801A9CE8/801A9D5C (the two of
// these eight addresses that ARE referenced from 22610.c/337D0.c, by their
// raw names) are aliased back onto the real function names via macros.

#define LAYOUT_OBJ_START 0xfffe
#define LAYOUT_OBJ_END 0xffff

#define LAYOUT_OBJ_POSITION_FORWARD 0
#define LAYOUT_OBJ_POSITION_BACKWARD 1

// Indices into a LayoutEntity when addressed as a flat u16[5].
#define LAYOUT_OBJ_POS_X 0
#define LAYOUT_OBJ_POS_Y 1
#define LAYOUT_OBJ_ID 2
#define LAYOUT_OBJ_SLOT 3

#define DECREMENT_AND_CHECK(x) (--(x) != (u8) -1)

void CreateEntityFromLayout(Entity* entity, LayoutEntity* initDesc) {
    Tilemap* tilemap = &g_Tilemap;

    DestroyEntity(entity);
    entity->entityId = initDesc->entityId & 0x3FF;
    entity->pfnUpdate = (&D_us_80180410)[entity->entityId];
    entity->posX.i.hi = (s16) initDesc->posX - tilemap->scrollX.i.hi;
    entity->posY.i.hi = (s16) initDesc->posY - tilemap->scrollY.i.hi;
    entity->params = initDesc->params;
    entity->entityRoomIndex = initDesc->entityRoomIndex >> 8;
    entity->unk68 = (initDesc->entityId >> 0xA) & 7;
}

void CreateEntityWhenInVerticalRange(LayoutEntity* obj) {
    s16 yClose;
    s16 yFar;
    s16 posY;
    Entity* entity;
    Tilemap* tilemap = &g_Tilemap;

    posY = tilemap->scrollY.i.hi;
    yClose = posY - 0x40;
    yFar = posY + 0x120;
    if (yClose < 0) {
        yClose = 0;
    }

    posY = obj->posY;
    if (posY < yClose || posY > yFar) {
        return;
    }

    switch (obj->entityId & 0xE000) {
        case 0x0:
            entity = &g_Entities_64[obj->entityRoomIndex & 0xFF];
            if (!entity->entityId) {
                CreateEntityFromLayout(entity, obj);
            }
            break;
        case 0xA000:
            entity = &g_Entities_64[obj->entityRoomIndex & 0xFF];
            CreateEntityFromLayout(entity, obj);
            break;
        case 0x8000:
            break;
    }
}

void CreateEntityWhenInHorizontalRange(LayoutEntity* obj) {
    s16 xClose;
    s16 xFar;
    s16 posX;
    Entity* entity;
    Tilemap* tilemap = &g_Tilemap;

    posX = tilemap->scrollX.i.hi;
    xClose = posX - 0x40;
    xFar = posX + 0x140;
    if (xClose < 0) {
        xClose = 0;
    }

    posX = obj->posX;
    if (posX < xClose || posX > xFar) {
        return;
    }

    switch (obj->entityId & 0xE000) {
        case 0x0:
            entity = &g_Entities_64[obj->entityRoomIndex & 0xFF];
            if (!entity->entityId) {
                CreateEntityFromLayout(entity, obj);
            }
            break;
        case 0xA000:
            entity = &g_Entities_64[obj->entityRoomIndex & 0xFF];
            CreateEntityFromLayout(entity, obj);
            break;
        case 0x8000:
            break;
    }
}

void FindFirstEntityToTheRight(s16 posX) {
    while (1) {
        u16* layoutEntity = func_us_801B5C2C;
        if (layoutEntity[LAYOUT_OBJ_POS_X] != LAYOUT_OBJ_START &&
            layoutEntity[LAYOUT_OBJ_POS_X] >= posX) {
            break;
        }
        func_us_801B5C2C += sizeof(LayoutEntity) / sizeof(u16);
    }
}

void FindFirstEntityToTheLeft(s16 posX) {
    while (1) {
        u16* layoutEntity = func_us_801B5C2C;
        if (layoutEntity[LAYOUT_OBJ_POS_X] != LAYOUT_OBJ_END &&
            (layoutEntity[LAYOUT_OBJ_POS_X] <= posX ||
             layoutEntity[LAYOUT_OBJ_POS_X] == LAYOUT_OBJ_START)) {
            break;
        }
        func_us_801B5C2C -= sizeof(LayoutEntity) / sizeof(u16);
    }
}

void CreateEntitiesToTheRight(s16 posX) {
    const s32 expected = false;
    u16* layoutObjHorizontal;
    u8 flag;

    if (D_us_801B5C34) {
        FindFirstEntityToTheRight(posX - g_ScrollDeltaX);
        D_us_801B5C34 = LAYOUT_OBJ_POSITION_FORWARD;
    }

    while (1) {
        layoutObjHorizontal = func_us_801B5C2C;
        if (layoutObjHorizontal[LAYOUT_OBJ_POS_X] == LAYOUT_OBJ_END ||
            posX < layoutObjHorizontal[LAYOUT_OBJ_POS_X]) {
            break;
        }

        flag = (layoutObjHorizontal[LAYOUT_OBJ_SLOT] >> 8);
        if (DECREMENT_AND_CHECK(flag)) {
            if ((g_unkGraphicsStruct.D_80097428[flag >> 5] &
                 (1 << (flag & 0x1F))) == expected) {
                CreateEntityWhenInVerticalRange((LayoutEntity*) func_us_801B5C2C);
            }
        } else {
            CreateEntityWhenInVerticalRange((LayoutEntity*) func_us_801B5C2C);
        }

        func_us_801B5C2C += sizeof(LayoutEntity) / sizeof(u16);
    }
}

void CreateEntitiesToTheLeft(s16 posX) {
    const s32 expected = false;
    u16* layoutObjHorizontal;
    u8 flag;

    if (posX < 0) {
        posX = 0;
    }

    if (!D_us_801B5C34) {
        FindFirstEntityToTheLeft(posX - g_ScrollDeltaX);
        D_us_801B5C34 = LAYOUT_OBJ_POSITION_BACKWARD;
    }

    while (1) {
        layoutObjHorizontal = func_us_801B5C2C;
        if (layoutObjHorizontal[LAYOUT_OBJ_POS_X] == LAYOUT_OBJ_START) {
            return;
        }

        if (posX > layoutObjHorizontal[LAYOUT_OBJ_POS_X]) {
            return;
        }

        flag = (layoutObjHorizontal[LAYOUT_OBJ_SLOT] >> 8);
        if (DECREMENT_AND_CHECK(flag)) {
            if ((g_unkGraphicsStruct.D_80097428[flag >> 5] &
                 (1 << (flag & 0x1F))) == expected) {
                CreateEntityWhenInVerticalRange((LayoutEntity*) func_us_801B5C2C);
            }
        } else {
            CreateEntityWhenInVerticalRange((LayoutEntity*) func_us_801B5C2C);
        }

        func_us_801B5C2C -= sizeof(LayoutEntity) / sizeof(u16);
    }
}

// Real first instruction lives at 0x801A9808 (previously the bogus stub
// "func_us_801A9808"); see file header comment.
void FindFirstEntityAbove(s16 posY) {
    while (1) {
        u16* ptr = &D_us_801B5C30[LAYOUT_OBJ_POS_Y];
        if (*ptr != LAYOUT_OBJ_START && *ptr >= posY) {
            break;
        }
        D_us_801B5C30 += sizeof(LayoutEntity) / sizeof(u16);
    }
}

// Real first instruction lives at 0x801A9854 (previously the bogus stub
// "func_us_801A9854"); see file header comment.
void FindFirstEntityBelow(s16 posY) {
    while (1) {
        u16* ptr = &D_us_801B5C30[LAYOUT_OBJ_POS_Y];
        if (*ptr != LAYOUT_OBJ_END &&
            (*ptr <= posY || *ptr == LAYOUT_OBJ_START)) {
            break;
        }
        D_us_801B5C30 -= sizeof(LayoutEntity) / sizeof(u16);
    }
}

// Real first instruction lives at 0x801A98AC (previously the bogus stub
// "func_us_801A98AC"); see file header comment.
void CreateEntitiesAbove(s16 posY) {
    const s32 expected = false;
    u16* layout;
    u8 flag;

    if (D_us_801B5C38) {
        FindFirstEntityAbove(posY - g_ScrollDeltaY);
        D_us_801B5C38 = LAYOUT_OBJ_POSITION_FORWARD;
    }

    while (1) {
        layout = &D_us_801B5C30[LAYOUT_OBJ_POS_Y];

        if (layout[LAYOUT_OBJ_POS_X] == LAYOUT_OBJ_END ||
            posY < layout[LAYOUT_OBJ_POS_X]) {
            break;
        }

        flag = layout[LAYOUT_OBJ_ID] >> 8;
        if (DECREMENT_AND_CHECK(flag)) {
            if ((g_unkGraphicsStruct.D_80097428[flag >> 5] &
                 (1 << (flag & 0x1F))) == expected) {
                CreateEntityWhenInHorizontalRange((LayoutEntity*) D_us_801B5C30);
            }
        } else {
            CreateEntityWhenInHorizontalRange((LayoutEntity*) D_us_801B5C30);
        }

        D_us_801B5C30 += sizeof(LayoutEntity) / sizeof(u16);
    }
}

// Real first instruction lives at 0x801A99A8 (previously the bogus stub
// "func_us_801A99A8"); see file header comment.
void CreateEntitiesBelow(s16 posY) {
    const s32 expected = false;
    u16* layout;
    u8 flag;

    if (posY < 0) {
        posY = 0;
    }

    if (!D_us_801B5C38) {
        FindFirstEntityBelow(posY - g_ScrollDeltaY);
        D_us_801B5C38 = LAYOUT_OBJ_POSITION_BACKWARD;
    }

    while (1) {
        layout = &D_us_801B5C30[LAYOUT_OBJ_POS_Y];
        if (layout[LAYOUT_OBJ_POS_X] == LAYOUT_OBJ_START ||
            (posY > layout[LAYOUT_OBJ_POS_X])) {
            break;
        }
        flag = layout[LAYOUT_OBJ_ID] >> 8;
        if (DECREMENT_AND_CHECK(flag)) {
            if ((g_unkGraphicsStruct.D_80097428[flag >> 5] &
                 (1 << (flag & 0x1F))) == expected) {
                CreateEntityWhenInHorizontalRange((LayoutEntity*) D_us_801B5C30);
            }
        } else {
            CreateEntityWhenInHorizontalRange((LayoutEntity*) D_us_801B5C30);
        }

        D_us_801B5C30 -= sizeof(LayoutEntity) / sizeof(u16);
    }
}

// Real first instruction lives at 0x801A9ABC (previously the bogus stub
// "func_us_801A9ABC"); see file header comment.
void InitRoomEntities(s32 objLayoutId) {
    Tilemap* tilemap = &g_Tilemap;
    s16 rightEdge;
    s16 temp;
    s16 i;

    func_us_801B5C2C = (u16*) D_us_8018026C[objLayoutId];
    D_us_801B5C30 = (u16*) D_us_80180340[objLayoutId];

    if (*func_us_801B5C2C != LAYOUT_OBJ_START) {
        func_us_801B5C2C++;
        temp = Random() & 0xFF;
        for (i = 0; 1; i++) {
            temp -= *func_us_801B5C2C++;
            if (temp < 0) {
                func_us_801B5C2C = (u16*) ((func_us_801B5C2C[LAYOUT_OBJ_POS_Y] << 16) +
                                        func_us_801B5C2C[LAYOUT_OBJ_POS_X]);
                break;
            }
            func_us_801B5C2C += 2;
        }
        D_us_801B5C30 += i * 2 + 2;
        D_us_801B5C30 = (u16*) ((D_us_801B5C30[LAYOUT_OBJ_POS_Y] << 0x10) +
                                D_us_801B5C30[LAYOUT_OBJ_POS_X]);
    }

    temp = tilemap->scrollX.i.hi;
    i = temp - 64;
    rightEdge = temp + 320;

    if (i < 0) {
        i = 0;
    }

    D_us_801B5C34 = LAYOUT_OBJ_POSITION_FORWARD;
    D_us_801B5C38 = LAYOUT_OBJ_POSITION_FORWARD;
    FindFirstEntityToTheRight(i);
    CreateEntitiesToTheRight(rightEdge);
    temp = tilemap->scrollY.i.hi;
    i = temp + 288;
    FindFirstEntityAbove(i);
}

// Real first instruction lives at 0x801A9C34 (previously the bogus stub
// "func_us_801A9C34"); see file header comment.
void UpdateRoomPosition(void) {
    Tilemap* tilemap = &g_Tilemap;
    s16 tmp;

    if (g_ScrollDeltaX != 0) {
        tmp = tilemap->scrollX.i.hi;
        if (g_ScrollDeltaX > 0) {
            tmp += 320;
            CreateEntitiesToTheRight(tmp);
        } else {
            tmp -= 64;
            CreateEntitiesToTheLeft(tmp);
        }
    }

    if (g_ScrollDeltaY != 0) {
        tmp = tilemap->scrollY.i.hi;
        if (g_ScrollDeltaY > 0) {
            tmp += 288;
            CreateEntitiesAbove(tmp);
        } else {
            tmp -= 64;
            CreateEntitiesBelow(tmp);
        }
    }
}

// Real first instruction lives at 0x801A9CE8 (previously the bogus stub
// "func_us_801A9CE8"); 22610.c/337D0.c call this exact address by its raw
// name, aliased back onto this real name via a macro in bo2.h.
void CreateEntityFromCurrentEntity(u16 entityId, Entity* entity) {
    DestroyEntity(entity);
    entity->entityId = entityId;
    entity->pfnUpdate = (&D_us_80180410)[entityId];
    entity->posX.i.hi = g_CurrentEntity->posX.i.hi;
    entity->posY.i.hi = g_CurrentEntity->posY.i.hi;
}

// Real first instruction lives at 0x801A9D5C (previously the bogus stub
// "func_us_801A9D5C"); 22610.c/337D0.c call this exact address by its raw
// name, aliased back onto this real name via a macro in bo2.h.
void CreateEntityFromEntity(u16 entityId, Entity* source, Entity* entity) {
    DestroyEntity(entity);
    entity->entityId = entityId;
    entity->pfnUpdate = (&D_us_80180410)[entityId];
    entity->posX.i.hi = source->posX.i.hi;
    entity->posY.i.hi = source->posY.i.hi;
}

// D_us_80180000 (bo2's AbbreviatedOverlay struct, still raw asm data in
// asm/us/boss/bo2/data/0.data.s -- a future session can port it to a proper
// header.c like bo4/bo6/mar/rbo0 already have) references InitRoomEntities
// and UpdateRoomPosition by their raw pre-mis-split addresses
// (func_us_801A9ABC / func_us_801A9C34; see this file's header comment).
// Unlike the 22610.c/337D0.c call sites, that's a plain .word in an asm file,
// not C -- the #define aliases in bo2.h can't reach it, since nothing
// preprocesses that file. Give the linker an extra symbol at the same
// address instead, so the raw table entry resolves without duplicating code.
__asm__(".global func_us_801A9ABC\n"
        ".set func_us_801A9ABC, InitRoomEntities\n"
        ".global func_us_801A9C34\n"
        ".set func_us_801A9C34, UpdateRoomPosition\n");
