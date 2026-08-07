// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo0.h"

// Olrox's own idle "spring" (pos, vel) pair used by the leg/wing positioning
// helpers below; meaning of the pos value itself not yet identified.
#define ENTITY_EXT90(e) ((s16*)((u8*)(e) + 0x90))

// ext+0x80/+0x84 each hold a *pointer* to one of Olrox's linked body-part
// structs (same "obj"-style struct as ENTITY_EXT90's family: +0xC/+0x10 angle
// fields, +0x18 next-part pointer, +0x1C/+0x20 base position, +0x28/+0x2C
// rotated output position).
#define ENTITY_EXT80(e) (*(s16**)((u8*)(e) + 0x80))
#define ENTITY_EXT84(e) (*(s16**)((u8*)(e) + 0x84))

// TODO: exact type of self->ext+0x10 (offset 0x8C) not identified; raw-cast
// pointer whose fields at +0x2A/+0x2E feed self's own posX/posY integer parts.
// Declared here (not just before its first use) so it precedes every caller,
// including func_us_801B5470 earlier in this file - a real "used prior to
// declaration" compile error was masked until now by the pre-existing ❌ BO0
// residual noise.
extern u16 D_us_80180708[];

extern EInit g_EInitOlroxAfterImage;

// Steps fromAngle toward toAngle by at most maxDelta units, taking the
// shortest rotational direction, wrapped to a 12-bit angle (0x1000 units/circle).
s16 func_us_801AD26C(s32 maxDelta, s16 fromAngle, s16 toAngle) {
    s16 delta;
    s16 negMax;

    fromAngle &= 0xFFF;
    toAngle &= 0xFFF;
    delta = toAngle - fromAngle;

    if (delta >= 0x801) {
        delta -= 0x1000;
    }
    negMax = -maxDelta;
    if (delta < -0x800) {
        delta += 0x1000;
    }
    if (delta < negMax) {
        delta = negMax;
    }
    if (maxDelta < delta) {
        delta = maxDelta;
    }
    return (fromAngle + delta) & 0xFFF;
}

// Returns whether the tile at (x, y) is solid.
s32 func_us_801AD2F0(s32 x, s32 y) {
    Collider collider;

    g_api_CheckCollision((s16)x, (s16)y, &collider, 0);
    return collider.effects & EFFECT_SOLID;
}

INCLUDE_RODATA("boss/bo0/nonmatchings/2D26C", D_us_801A9344);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AD338);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AE858);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AF31C);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AF604);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AF8C0);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801AFAF4);

// Fading teleport after-image clone; fades out over ~64 frames then destroys itself
void EntityOlroxAfterImage(Entity* self) {
    if (!self->step) {
        InitializeEntity(g_EInitOlroxAfterImage);
        self->palette = 0x217;
        self->drawFlags = ENTITY_OPACITY;
        self->opacity = 0x80;
        self->hitboxState = 0;
        self->blendMode = 0x30;
        self->zPriority -= 2;
        self->animCurFrame = self->params;
    }
    self->opacity -= 2;
    if (!self->opacity) {
        DestroyEntity(self);
    }
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B001C);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B053C);

extern s16 D_us_80180DC4[];
extern s16 D_us_80180EE0[];
extern s32 D_us_801CD6E8[];

// Builds an interpolation table by combining two small s16 lookup tables
// (D_us_80180DC4/D_us_80180EE0) into interleaved slots of D_us_801CD6E8.
void func_us_801B088C(void) {
    s16* p1 = D_us_80180DC4;
    s16* p2 = D_us_80180EE0;
    s32* even = D_us_801CD6E8;
    s32* odd = D_us_801CD6E8 - 1;
    s32 i;

    for (i = 0; i < 0x42; i++) {
        *odd = *odd + (((-0x40 + *p2) - (-0x10 + *p1)) << 9);
        p1++;
        p2++;
        odd += 2;

        *even = *even + (((-0x70 + *p2) - (-0x27 + *p1)) << 9);
        p1++;
        p2++;
        even += 2;
    }
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B0930);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B13A8);

// Resets both linked body-part phase counters (childPalette) and starts a
// new step. Confirmed equivalent to our earlier raw-offset translation:
// ET_B0_Unk's declared field order compiles parent to +0x18 and
// childPalette to +0x24 relative to unk80, matching byte-for-byte.
void func_us_801B1590(u8 step) {
    ET_B0_Unk* temp = (ET_B0_Unk*)g_CurrentEntity->ext.b0Unk.unk80;

    g_CurrentEntity->step = step;
    g_CurrentEntity->step_s = 0;

    temp->childPalette = 0;
    temp = (ET_B0_Unk*)temp->parent;
    temp->childPalette = 0;
}

// Moves *value toward target by at most step units per call; returns true
// (1) once it reaches (or lands within step of) target, 0 while still moving.
s32 func_us_801B15BC(s16* value, s32 target, s32 step) {
    s16 current = *value;
    s16 t = (s16)target;
    s16 s = (s16)step;
    s16 diff;

    diff = current - t;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff < s) {
        *value = t;
        return 1;
    }
    if (t < current) {
        *value = current - s;
    }
    if (*value < t) {
        *value += s;
    }
    return 0;
}

// Spring-style approach: nudges the (pos, vel) pair pointed to by pv toward
// target by accelerating vel (in steps of ~maxAccel/16, min 1) clamped near
// +-maxAccel, then integrating pos by vel. Returns 1 once the remaining
// distance is smaller than the current velocity (this step reaches/overshoots
// target), 0 while still approaching.
s32 func_us_801B163C(s16* pv, s32 target, s32 maxAccel) {
    s16 vel = pv[1];
    s16 limit = (s16)maxAccel;
    s32 accelStep = (limit >= 0) ? limit : (limit + 0xF);
    s32 diff = target - pv[0];
    s32 t;

    accelStep >>= 4;
    if (accelStep == 0) {
        accelStep = 1;
    }

    if (diff > 0) {
        if (vel < limit) {
            vel += accelStep;
        } else {
            vel -= accelStep;
        }
    } else {
        if (vel > -limit) {
            vel -= accelStep;
        } else {
            vel += accelStep;
        }
    }

    t = (diff < 0) ? -diff : diff;
    if (t < ((vel < 0) ? -vel : vel)) {
        pv[0] = (s16)target;
        pv[1] = (s16)diff;
        return 1;
    }
    pv[0] += vel;
    pv[1] = vel;
    return 0;
}

// Nudges two (pos, vel) pairs at byte offsets 0xC/0xE and 0x10/0x12 of p
// toward target1 and target2 respectively, stepping by exactly +-limit per
// call and snapping once within limit. Returns 1 only once BOTH pairs have
// snapped onto their targets.
s32 func_us_801B171C(s16* p, s32 target1, s32 target2, s32 limit) {
    s32 targets[2];
    s32 notDone = 0;
    s32 i;

    targets[0] = target1;
    targets[1] = target2;

    for (i = 0; i < 2; i++) {
        s16* pv = p + 6 + i * 2;
        s16 pos = pv[0];
        s32 diff = targets[i] - pos;
        s32 absDiff = diff;

        if (absDiff < 0) {
            absDiff = -absDiff;
        }

        if (absDiff < limit) {
            pv[0] = (s16)targets[i];
            pv[1] = (s16)diff;
        } else if (diff <= 0) {
            pv[0] = pos - (s16)limit;
            pv[1] = -(s16)limit;
            notDone = 1;
        } else {
            pv[0] = pos + (s16)limit;
            pv[1] = (s16)limit;
            notDone = 1;
        }
    }

    return notDone ^ 1;
}

void func_us_801B17BC(s16* obj) {
    s32 result;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            result = func_us_801B171C(obj, -0x400, 0x200, 0x18);
            break;
        case 1:
            result = func_us_801B171C(obj, -0x400, -0x40, 0x20);
            break;
        case 2:
            result = func_us_801B171C(obj, -0x300, -0x40, 0x20);
            break;
        default:
            return;
    }

    if (result) {
        (*((u8*)obj + 0x24))++;
    }
}

void func_us_801B1864(s16* obj) {
    s16 v;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            if (func_us_801B171C(obj, -0x40, 0x40, 0x1C)) {
                (*((u8*)obj + 0x24))++;
            }
            v = *ENTITY_EXT90(g_CurrentEntity);
            if (v < -0xFF) {
                return;
            }
            *ENTITY_EXT90(g_CurrentEntity) = v - 0xC;
            return;
        case 1:
            if (func_us_801B171C(obj, -0x100, 0x300, 0x18)) {
                (*((u8*)obj + 0x24))++;
            }
            v = *ENTITY_EXT90(g_CurrentEntity);
            if (v < 0x100) {
                *ENTITY_EXT90(g_CurrentEntity) = v + 8;
            }
            return;
        default:
            return;
    }
}

void func_us_801B1950(s16* obj) {
    s32 result;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            result = func_us_801B171C(obj, -0x200, 0x300, 0x18);
            break;
        case 1:
            result = func_us_801B171C(obj, 0, 0x280, 0x1C);
            break;
        case 2:
            result = func_us_801B171C(obj, 0x80, 0x180, 0x18);
            break;
        default:
            return;
    }

    if (result) {
        (*((u8*)obj + 0x24))++;
    }
}

void func_us_801B19FC(s16* obj) {
    s32 result;
    s16 v;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            result = func_us_801B171C(obj, -0x80, 0x80, 0x1C);
            if (result) {
                (*((u8*)obj + 0x24))++;
            }
            v = *ENTITY_EXT90(g_CurrentEntity);
            if (v < -0xFF) {
                return;
            }
            *ENTITY_EXT90(g_CurrentEntity) = v - 0x10;
            return;
        case 1:
            result = func_us_801B171C(obj, -0x100, 0, 0x18);
            if (result) {
                (*((u8*)obj + 0x24))++;
            }
            return;
        case 2:
            result = func_us_801B171C(obj, -0x280, -0xC0, 0x14);
            if (result) {
                (*((u8*)obj + 0x24))++;
            }
            v = *ENTITY_EXT90(g_CurrentEntity);
            if (v < 0x100) {
                *ENTITY_EXT90(g_CurrentEntity) = v + 0x10;
            }
            return;
        default:
            return;
    }
}

s32 func_us_801B1B30(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 first;

    switch (*((u8*)obj + 0x24)) {
        case 0: {
            s32 sum = func_us_801B171C(obj, -0x180, 0, 0x20);
            sum += func_us_801B171C(next, 0x80, 0x280, 0x20);
            if (sum == 2) {
                (*((u8*)obj + 0x24))++;
            }
            return 0;
        }
        case 1:
            first = func_us_801B171C(obj, -0x200, -0x100, 0x40);
            func_us_801B171C(next, -0x100, 0x280, 0x40);
            if (first == 0) {
                return 0;
            }
            (*((u8*)obj + 0x24))++;
            return 0;
        case 2:
            first = func_us_801B171C(obj, -0x280, -0x200, 0x60);
            func_us_801B171C(next, -0x180, 0x300, 0x40);
            return (first != 0) ? 1 : 0;
        default:
            return 0;
    }
}

s32 func_us_801B1C60(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 result;

    func_us_801B163C(ENTITY_EXT90(g_CurrentEntity), 0, 0xC);
    result = func_us_801B171C(obj, -0x1C0, 0x280, 0x10);
    result += func_us_801B171C(next, -0x380, 0x140, 0x10);
    return (result ^ 2) < 1;
}

s32 func_us_801B1CE0(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 acc = func_us_801B163C(ENTITY_EXT90(g_CurrentEntity), 0x180, 0x40);

    switch (*((u8*)obj + 0x24)) {
        case 0:
            func_us_801B171C(obj, -0x60, 0x340, 0x18);
            if (func_us_801B171C(next, -0x500, 0x80, 0x20) == 0) {
                return 0;
            }
            (*((u8*)obj + 0x24))++;
            return 0;
        case 1:
            acc += func_us_801B171C(obj, -0x40, 0x340, 0x18);
            acc += func_us_801B171C(next, -0x3C0, 0xE0, 0x20);
            return (acc == 3) ? 1 : 0;
        default:
            return 0;
    }
}

// Nudges Olrox's own idle spring plus two linked sub-part position pairs
// (obj and *(obj+0x18)) toward fixed targets; returns true once all three
// have settled. The 4 sibling functions below share this exact shape, only
// differing in the target/step constants passed to func_us_801B171C.
s32 func_us_801B1DDC(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 result;

    func_us_801B163C(ENTITY_EXT90(g_CurrentEntity), 0, 0xC);
    result = func_us_801B171C(obj, -0x2C0, 0x2C0, 0x28);
    result += func_us_801B171C(next, -0x100, 0x3C0, 0x38);
    return (result ^ 2) < 1;
}

s32 func_us_801B1E5C(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 result;

    func_us_801B163C(ENTITY_EXT90(g_CurrentEntity), 0, 0xC);
    result = func_us_801B171C(obj, -0x40, 0x40, 0x60);
    result += func_us_801B171C(next, -0x200, 0x280, 0x50);
    return (result ^ 2) < 1;
}

s32 func_us_801B1EDC(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 result;

    func_us_801B163C(ENTITY_EXT90(g_CurrentEntity), 0, 0x10);
    result = func_us_801B171C(obj, -0x200, 0x200, 0x60);
    result += func_us_801B171C(next, -0x200, 0x180, 0x40);
    return (result ^ 2) < 1;
}

s32 func_us_801B1F5C(s16* obj) {
    s16* next = *(s16**)((u8*)obj + 0x18);
    s32 sum;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            sum = func_us_801B171C(obj, -0x80, 0x80, 0x20);
            sum += func_us_801B171C(next, 0, 0x180, 0xC);
            if (sum == 2) {
                (*((u8*)obj + 0x24))++;
            }
            return 0;
        case 1:
            sum = func_us_801B171C(obj, -0x300, 0, 0x28);
            sum += func_us_801B171C(next, -0x80, 0x100, 0xC);
            return (sum == 2) ? 1 : 0;
        default:
            return 0;
    }
}

void func_us_801B2044(s16* obj) {
    s32 result;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            result = func_us_801B171C(obj, -0x400, 0x200, 8);
            break;
        case 1:
            result = func_us_801B171C(obj, -0x400, -0x40, 8);
            break;
        case 2:
            result = func_us_801B171C(obj, -0x300, -0x40, 0xC);
            break;
        default:
            return;
    }

    if (result) {
        (*((u8*)obj + 0x24))++;
    }
}

// obj+0x24 is a small phase counter shared by this family of positioning
// helpers; advances it once the corresponding func_us_801B171C move settles.
void func_us_801B20F4(s16* obj) {
    s32 result;

    switch (*((u8*)obj + 0x24)) {
        case 0:
            result = func_us_801B171C(obj, -0x40, 0x40, 0x8);
            break;
        case 1:
            result = func_us_801B171C(obj, -0x100, 0x380, 0x14);
            break;
        default:
            return;
    }

    if (result) {
        (*((u8*)obj + 0x24))++;
    }
}

// Debug-only tile-boundary print (format string has an unused "%x" - matches
// the original asm, which never wires up an arg to FntPrint here either).
// Defined locally (not extern) since this func's .s file was the only place
// this rodata string lived; replacing its last INCLUDE_ASM orphans it otherwise.
static const char D_us_801A94C4[] = "hit_kind %x\n";

s16 func_us_801B2178(s16* obj) {
    u16 tileX = *(u16*)((u8*)&g_Tilemap + 0xE);
    u16 objField = *(u16*)((u8*)obj + 0x22);
    s16 sum = (s16)(tileX + objField);
    s16 result = 0;
    u16 flag;

    if (sum >= 0x1CC) {
        result = 0x1CC - sum;
        flag = 1;
    } else {
        flag = 0;
    }
    *(u16*)((u8*)obj + 0x26) = flag;
    FntPrint(D_us_801A94C4);
    return result;
}

// Drives Olrox's linked ring/body-part chain: computes two rotated offsets
// (rcos/rsin at scale 672 and 448) for the currently-selected sub-part
// (self->ext+0x80 or +0x84, chosen by self->ext+0x88), writes the combined
// result into g_CurrentEntity's own posX/posY, then repeats a similar
// (single-angle) computation for that sub-part's linked "next" part, feeding
// only that part's own internal state (not g_CurrentEntity's position again).
void func_us_801B21F0(void) {
    s16* part;
    s16* next;
    s32 s0, v0, v1, a0;
    s32 s2, s3, s4, s5;

    {
        s16* part80 = ENTITY_EXT80(g_CurrentEntity);
        *(s32*)((u8*)part80 + 0x1C) = *(s32*)((u8*)g_CurrentEntity + 0xBC);
        *(s32*)((u8*)part80 + 0x20) = *(s32*)((u8*)g_CurrentEntity + 0xC0);
    }
    {
        s16* part84 = ENTITY_EXT84(g_CurrentEntity);
        *(s32*)((u8*)part84 + 0x1C) = *(s32*)((u8*)g_CurrentEntity + 0x178);
        *(s32*)((u8*)part84 + 0x20) = *(s32*)((u8*)g_CurrentEntity + 0x17C);
    }

    if (*((u8*)g_CurrentEntity + 0x88) == 0) {
        part = ENTITY_EXT80(g_CurrentEntity);
    } else {
        part = ENTITY_EXT84(g_CurrentEntity);
    }

    s4 = *(s32*)((u8*)part + 0x1C);
    s2 = *(s32*)((u8*)part + 0x20);

    if (g_CurrentEntity->facingLeft != 0) {
        v0 = 0xC00 - *(u16*)((u8*)part + 0x10);
    } else {
        v0 = *(u16*)((u8*)part + 0x10) + 0xC00;
    }
    s0 = (s16)v0;
    v1 = rcos(s0) * 672;
    s3 = s4 - v1;
    v1 = rsin(s0) * 672;
    s2 = s2 + v1;
    *(s32*)((u8*)part + 0x28) = s3;
    *(s32*)((u8*)part + 0x2C) = s2;

    if (g_CurrentEntity->facingLeft != 0) {
        v0 = 0xC00 - *(u16*)((u8*)part + 0xC);
    } else {
        v0 = *(u16*)((u8*)part + 0xC) + 0xC00;
    }
    s0 = (s16)v0;
    v1 = rcos(s0) * 448;
    s5 = s3 - v1;
    a0 = s4 - s5;
    v1 = rsin(s0) * 448;
    *(s32*)((u8*)part + 0x14) = a0;
    if (g_CurrentEntity->facingLeft == 0) {
        *(s32*)((u8*)part + 0x14) = -a0;
    }
    s2 = s2 + v1;

    g_CurrentEntity->posX.val = s5;
    g_CurrentEntity->posY.val = s2 + (s32)0xFFFC0000;

    next = *(s16**)((u8*)part + 0x18);

    if (g_CurrentEntity->facingLeft != 0) {
        v0 = 0xC00 - *(u16*)((u8*)next + 0xC);
    } else {
        v0 = *(u16*)((u8*)next + 0xC) + 0xC00;
    }
    s0 = (s16)v0;
    v1 = rcos(s0) * 448;
    s3 = s5 + v1;
    v1 = rsin(s0) * 448;
    s2 = s2 - v1;
    *(s32*)((u8*)next + 0x28) = s3;
    *(s32*)((u8*)next + 0x2C) = s2;

    if (g_CurrentEntity->facingLeft != 0) {
        v0 = 0xC00 - *(u16*)((u8*)next + 0x10);
    } else {
        v0 = *(u16*)((u8*)next + 0x10) + 0xC00;
    }
    s0 = (s16)v0;
    v1 = rcos(s0) * 672;
    s4 = s3 + v1;
    v1 = rsin(s0) * 672;
    s2 = s2 - v1;
    *(s32*)((u8*)next + 0x1C) = s4;
    *(s32*)((u8*)next + 0x20) = s2;

    v1 = s5 - s4;
    *(s32*)((u8*)next + 0x14) = v1;
    if (g_CurrentEntity->facingLeft == 0) {
        *(s32*)((u8*)next + 0x14) = -v1;
    }
}

// Walks up to 2 linked body-parts starting at self->ext+0x80's chain, applying
// the same two-angle rcos/rsin position update as func_us_801B21F0 to each
// part in turn - using g_CurrentEntity's CURRENT posX/posY as a fixed
// reference each iteration (not fed back into g_CurrentEntity, unlike 21F0).
void func_us_801B24CC(void) {
    s16* part;
    s32 count;
    s16 angle;
    s32 v0, v1;
    s32 s2, s3, s4, newPos;

    {
        s16* part80 = ENTITY_EXT80(g_CurrentEntity);
        *(s32*)((u8*)part80 + 0x1C) = *(s32*)((u8*)g_CurrentEntity + 0xBC);
        *(s32*)((u8*)part80 + 0x20) = *(s32*)((u8*)g_CurrentEntity + 0xC0);
    }
    count = 0;
    {
        s16* part84 = ENTITY_EXT84(g_CurrentEntity);
        *(s32*)((u8*)part84 + 0x1C) = *(s32*)((u8*)g_CurrentEntity + 0x178);
        *(s32*)((u8*)part84 + 0x20) = *(s32*)((u8*)g_CurrentEntity + 0x17C);
    }

    part = ENTITY_EXT80(g_CurrentEntity);

    do {
        s4 = g_CurrentEntity->posX.val;
        s3 = g_CurrentEntity->posY.val;

        if (g_CurrentEntity->facingLeft != 0) {
            v0 = 0xC00 - *(u16*)((u8*)part + 0xC);
        } else {
            v0 = *(u16*)((u8*)part + 0xC) + 0xC00;
        }
        angle = (s16)v0;
        v1 = rcos(angle) * 448;
        s2 = s4 + v1;
        v1 = rsin(angle) * 448;
        s3 = s3 - v1;
        *(s32*)((u8*)part + 0x28) = s2;
        *(s32*)((u8*)part + 0x2C) = s3;

        if (g_CurrentEntity->facingLeft != 0) {
            v0 = 0xC00 - *(u16*)((u8*)part + 0x10);
        } else {
            v0 = *(u16*)((u8*)part + 0x10) + 0xC00;
        }
        angle = (s16)v0;
        v1 = rcos(angle) * 672;
        newPos = s2 + v1;
        v1 = rsin(angle) * 672;
        s3 = s3 - v1;
        *(s32*)((u8*)part + 0x1C) = newPos;
        *(s32*)((u8*)part + 0x20) = s3;
        v0 = newPos - s4;
        *(s32*)((u8*)part + 0x14) = v0;

        count++;
        if (g_CurrentEntity->facingLeft != 0) {
            *(s32*)((u8*)part + 0x14) = -v0;
        }

        part = *(s16**)((u8*)part + 0x18);
    } while (count < 2);
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B2690);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B30AC);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B365C);

void func_us_801B5470(Entity* self) {
    if (self->flags & 0x100) {
        self->facingLeft = (self - 1)->facingLeft;
        self->hitboxState = 0;
    }

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180708);
            self->hitboxWidth = 0xC;
            self->hitboxHeight = 4;
            self->hitboxOffX = -0xC;
            self->animCurFrame = 0;
            break;
        case 1: {
            void* ext8C = *(void**)((u8*)self + 0x8C);
            self->posX.val = *(s32*)((u8*)ext8C + 0x1C);
            self->posY.val = *(s32*)((u8*)ext8C + 0x20);
            break;
        }
        default:
            break;
    }
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B551C);

// Same "approach within step, else snap by step" idiom as func_us_801B15BC,
// inlined here over 8 stride-4 (value, ...) slots against 8 individual s16
// targets. May share the same GCC-scheduling residual as func_us_801B15BC.
s32 func_us_801B5D6C(s16* values, s16* targets, s32 step) {
    s32 allDone = 1;
    s16 limit = (s16)step;
    s32 i;

    for (i = 0; i < 8; i++) {
        s16 current = *values;
        s16 target = *targets;
        s16 diff = current - target;

        if (diff < 0) {
            diff = -diff;
        }
        if (diff < limit) {
            *values = target;
        } else {
            if (target < current) {
                allDone = 0;
                *values = current - limit;
            }
            if (*values < target) {
                *values += limit;
            }
        }
        values += 2;
        targets++;
    }
    return allDone;
}

// Runs func_us_801B163C over 8 (pos, vel) pairs against 8 individual s16
// targets, all sharing the same accel limit. Returns true only if every one
// of the 8 has settled onto its target.
s32 func_us_801B5E08(s16* pairs, s16* targets, s32 maxAccel) {
    s32 allDone = 1;
    s16 limit = (s16)maxAccel;
    s32 i;

    for (i = 0; i < 8; i++) {
        if (!func_us_801B163C(pairs, *targets, limit)) {
            allDone = 0;
        }
        pairs += 2;
        targets++;
    }
    return allDone;
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B5E8C);

extern s32 D_us_801CE5B0;
extern s16* D_us_801812D0[];
extern s16 D_us_80181288[];
extern s16 D_us_80181298[];
extern s16 D_us_801812A8[];
extern s16 D_us_801812B8[];

// obj+0x88 is an array of 8 (pos,vel) pairs (func_us_801B5E08/5D6C's family);
// obj+0x80 is a 0/1 toggle selecting which of 2 D_us_801812D0 target-tables
// to settle toward, flipped once the current one settles.
void func_us_801B619C(Entity* self) {
    Entity* parent = self - 4;
    s16 v0, v1;
    s32 result;

    self->facingLeft = parent->facingLeft;
    self->palette = parent->palette;
    if (D_us_801CE5B0 == 0) {
        self->posX.val = parent->posX.val;
        self->posY.val = parent->posY.val;
        if (self->facingLeft == 0) {
            self->posX.i.hi += 4;
        } else {
            self->posX.i.hi -= 4;
        }
    }

    if (self->flags & 0x100) {
        self->hitboxState = 0;
    }

    if (self->step < 10) {
        switch (self->step) {
            case 0: {
                Primitive* prim;
                s16* q;
                s32 i;
                s8 c1, c2;

                InitializeEntity(D_us_80180708);
                self->hitboxState = 0;
                self->animCurFrame = 0;
                self->flags |= 0x8000000;

                result = g_api_AllocPrimitives(4, 8);
                if ((s16)result == -1) {
                    self->step = 0;
                    break;
                }

                self->flags |= 0x800000;
                self->primIndex = result;
                prim = &g_PrimBuf[result];
                *(Primitive**)((u8*)self + 0x7C) = prim;

                c1 = -0x80;
                c2 = -0x75;
                for (i = 0; i < 8; i++) {
                    prim->u2 = (u8)c1;
                    prim->u0 = (u8)c1;
                    prim->tpage = 0x14;
                    prim->clut = 0x20B;
                    prim->u3 = (u8)c2;
                    prim->u1 = (u8)c2;
                    prim->v1 = 0x50;
                    prim->v0 = 0x50;
                    prim->v3 = 0x60;
                    prim->v2 = 0x60;
                    prim->priority = 0xA1;
                    prim->drawMode = 2;
                    prim = prim->next;
                    c1 += 0xB;
                    c2 += 0xB;
                }

                q = (s16*)((u8*)self + 0x88);
                i = 7;
                do {
                    *q = -0x180;
                    i--;
                    q += 2;
                } while (i >= 0);
                *(s16*)((u8*)self + 0x84) = 0;
                break;
            }
            case 1:
                if (D_us_801CE5B0 == 0) {
                    self->step++;
                }
                break;
            case 2:
                v0 = *(s16*)((u8*)self + 0x84);
                if (v0 < 0x1000) {
                    *(s16*)((u8*)self + 0x84) = v0 + 0x200;
                } else {
                    *(s16*)((u8*)self + 0x84) = 0x1000;
                    self->step++;
                }
                break;
            case 3:
                result = func_us_801B5E08((s16*)((u8*)self + 0x88),
                                          D_us_801812D0[*((u8*)self + 0x80)], 0x20);
                result &= 0xFF;
                if (result) {
                    *((u8*)self + 0x80) ^= 1;
                }
                break;
            case 4:
                func_us_801B5E08((s16*)((u8*)self + 0x88), D_us_80181288, 0x20);
                break;
            case 6:
                result = func_us_801B5E08((s16*)((u8*)self + 0x88), D_us_80181298, 0x60);
                result &= 0xFF;
                if (result) {
                    *((u8*)self + 0x80) ^= 1;
                }
                break;
            case 7:
                func_us_801B5E08((s16*)((u8*)self + 0x88), D_us_801812A8, 0x80);
                break;
            case 8:
                result = func_us_801B5E08((s16*)((u8*)self + 0x88), D_us_801812B8, 0x60);
                result &= 0xFF;
                if (result) {
                    *((u8*)self + 0x80) ^= 1;
                }
                break;
            case 9:
                result = func_us_801B5D6C((s16*)((u8*)self + 0x88),
                                          D_us_801812D0[*((u8*)self + 0x80)], 0x40);
                result &= 0xFF;
                if (result) {
                    *((u8*)self + 0x80) ^= 1;
                }
                break;
            default:
                break;
        }
    }

    func_us_801B5E8C();

    if ((self->palette & 0x8000) && *(Primitive**)((u8*)self + 0x7C) != NULL) {
        Primitive* p = *(Primitive**)((u8*)self + 0x7C);
        do {
            p->clut = self->palette & 0xFFF;
            p = p->next;
        } while (p != NULL);
    }

    if (*((u8*)self - 0x255) != 0 && *(Primitive**)((u8*)self + 0x7C) != NULL) {
        Primitive* p = *(Primitive**)((u8*)self + 0x7C);
        do {
            p->tpage = 0x15;
            p->clut = 0x219;
            p = p->next;
        } while (p != NULL);
    }
}

// MEDIUM CONFIDENCE: each s32 param packs two s16 values (target in the low
// half, accel limit in the high half) for one of Olrox's own 4 (pos, vel)
// pairs at ext+0x80/0x84/0x88/0x8C. Returns true only once all 4 settle.
s32 func_us_801B6520(s32 packed80, s32 packed84, s32 packed88, s32 packed8C) {
    s32 result;

    result = func_us_801B163C(ENTITY_EXT90(g_CurrentEntity) - 4, (s16)packed88,
                               (s16)(packed88 >> 16));
    result += func_us_801B163C(ENTITY_EXT90(g_CurrentEntity) - 2, (s16)packed8C,
                                (s16)(packed8C >> 16));
    result += func_us_801B163C(ENTITY_EXT90(g_CurrentEntity) - 8, (s16)packed80,
                                (s16)(packed80 >> 16));
    result += func_us_801B163C(ENTITY_EXT90(g_CurrentEntity) - 6, (s16)packed84,
                                (s16)(packed84 >> 16));
    return (result ^ 4) < 1;
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B65C0);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B6CA4);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B76E4);

void func_us_801B7BAC(Entity* self) {
    void* ext8C;
    u16 x, y;

    if (self->flags & 0x100) {
        self->hitboxState = 0;
    }

    if (self->step == 0) {
        InitializeEntity(D_us_80180708);
        self->hitboxOffY = 4;
        self->hitboxWidth = 4;
        self->animCurFrame = 0;
        self->hitboxHeight = 0x10;
    }

    ext8C = *(void**)((u8*)self + 0x8C);
    x = *(u16*)((u8*)ext8C + 0x2A);
    self->posX.i.hi = x;
    y = *(u16*)((u8*)ext8C + 0x2E);
    self->posY.i.hi = y;
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B7C44);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B7CC8);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B8794);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B888C);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B8970);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B8B64);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B8D8C);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801B9BEC);

void func_us_801BA030(s32 sfxId) {
    s16 dx = g_CurrentEntity->posX.i.hi - 0x80;
    s16 dy;
    s32 pan;
    s16 absDx;
    s32 vol;

    absDx = dx;
    if (absDx < 0) {
        absDx = -absDx;
    }
    pan = (absDx - 0x20) >> 5;
    if (pan >= 9) {
        pan = 8;
    } else if (pan < 0) {
        pan = 0;
    }
    if (dx < 0) {
        pan = -pan;
    }

    absDx = dx;
    if (absDx < 0) {
        absDx = -absDx;
    }
    vol = absDx - 0x60;

    dy = g_CurrentEntity->posY.i.hi - 0x80;
    if (dy < 0) {
        dy = -dy;
    }
    dy -= 0x70;
    if (dy > 0) {
        vol += dy;
    }

    vol = (vol < 0) ? 0 : (vol >> 1);
    vol = 0x58 - vol;
    if (vol > 0) {
        g_api_PlaySfxVolPan(sfxId, vol, pan);
    }
}

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801BA128);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801BA4AC);

INCLUDE_ASM("boss/bo0/nonmatchings/2D26C", func_us_801BA724);
