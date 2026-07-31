#include "bo2.h"

// Keeps g_CurrentEntity within the room's horizontal bounds, bouncing its
// posX.i.hi back to a fixed edge value when it would move off-screen in
// either direction (checked against velocityX's sign).
void func_us_801A2610(void) {
    Entity* entity = g_CurrentEntity;
    s16 scrollX = g_Tilemap.scrollX.i.hi;
    s32 worldX = entity->posX.i.hi + scrollX;
    s32 velocityX = entity->velocityX;

    if (velocityX > 0) {
        worldX += 0x10;
        if (worldX >= 0x1B1) {
            entity->posX.i.hi = 0x1A0 - scrollX;
            entity = g_CurrentEntity;
        }
        velocityX = entity->velocityX;
    }

    if (velocityX < 0) {
        worldX -= 0x10;
        if (worldX < 0x50) {
            entity->posX.i.hi = 0x60 - (u16)g_Tilemap.scrollX.i.hi;
        }
    }
}

// The main boss "conductor" entity (E_UNK_17) -- see bo2.h for the general
// shape of this state machine. `self - 3` is read repeatedly as a second,
// independent reference (posX.i.hi, flags, and its own ENTITY_UNK8F) --
// most likely the OTHER boss half (Minotaur vs Werewolf) sharing this same
// conductor function at a fixed slot offset, not a "neighboring part" in
// the sense func_us_801A3E04 uses self+1/self+4.
void func_us_801A269C(Entity* self) {
    Entity* other = self - 3;

    if (self->step != 0) {
        if ((self->flags & 0x100) && self->step < 0x40) {
            SetStep(0x40);
        }
    }

    FntPrint(D_us_801A1E68, self->step);
    FntPrint(D_us_801A1E78, ENTITY_UNK88(self));

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180530);
            self->hitboxState = 0;
            func_us_801A9CE8(0x18, self + 1);
            // fallthrough
        case 1:
            AnimateEntity(D_us_801806F4, self);
            GetSideToPlayer();
            self->facingLeft = !(GetSideToPlayer() & 1);
            UnkCollisionFunc3(D_us_801806D0);

            switch (self->step_s) {
                case 0:
                    self->palette = 0x815F;
                    self->blendMode = 0x50;
                    self->drawFlags |= 8;
                    self->opacity = 0;
                    self->step_s++;
                    // fallthrough
                case 1:
                    self->opacity += 4;
                    if (self->opacity == 0x80) {
                        self->palette = D_us_80180536;
                        self->blendMode = 0;
                        self->opacity = 0;
                        self->step_s++;
                    }
                    break;
                case 2:
                    self->opacity += 4;
                    if (self->opacity == 0x80) {
                        self->drawFlags = 0;
                        D_us_80180AB0 = 1;
                        self->step_s++;
                    }
                    break;
                case 3:
                    if (D_us_80180AAC) {
                        self->hitboxState = 3;
                        SetStep(2);
                    }
                    break;
                default:
                    break;
            }
            break;

        case 2: {
            if (self->step_s == 0) {
                if (ENTITY_UNK82(self) == 0) {
                    ENTITY_UNK82(self) = 0x20;
                }
                ENTITY_UNK8D(self) = 0;
                self->step_s++;
            }

            AnimateEntity(D_us_801806F4, self);
            self->facingLeft = !(GetSideToPlayer() & 1);

            if (ENTITY_UNK82(self) == 0) {
                ENTITY_UNK8D(self) = 1;
            } else {
                ENTITY_UNK82(self)--;
            }

            if (ENTITY_UNK84(self) != 0) {
                ENTITY_UNK8D(self) = 0;
                SetStep(7);
                break;
            }

            if (ENTITY_UNK8E(self) != 0) {
                s16 target = ENTITY_UNK88(self);
                ENTITY_UNK8E(self) = 0;
                ENTITY_UNK8D(self) = 0;
                SetStep(target);
                break;
            }

            {
                Entity* e0 = &g_Entities[0];
                s32 velX = PLAYER_velocityX;

                if (self->facingLeft) {
                    velX = -velX;
                }
                if (self->facingLeft == PLAYER_facingLeft) {
                    if (GetDistanceToPlayerX() < 0x49 && GetDistanceToPlayerX() < 0x60 &&
                        (g_Player.status & 0x400) && velX > 0) {
                        ENTITY_UNK8D(self) = 0;
                        SetStep(0x12);
                    }
                }
                if (self->facingLeft == e0->facingLeft) {
                    if (g_Player.status & 0x800) {
                        ENTITY_UNK8D(self) = 0;
                        SetStep(0x10);
                    }
                }
            }
            break;
        }

        case 7: {
            if (self->step_s == 0) {
                s16 selfX = self->posX.i.hi;
                s32 clamp;

                if (PLAYER_posX_i_hi < selfX) {
                    clamp = ENTITY_UNK84(self) + PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi;
                    if (clamp >= 0x191) {
                        clamp = 0x190;
                    }
                } else {
                    clamp = PLAYER_posX_i_hi - ENTITY_UNK84(self) + g_Tilemap.scrollX.i.hi;
                    if (clamp < 0x70) {
                        clamp = 0x70;
                    }
                }
                ENTITY_UNK84(self) = clamp;

                {
                    s32 diff = ENTITY_UNK84(self) - (self->posX.i.hi + g_Tilemap.scrollX.i.hi);
                    self->velocityX = (diff > 0) ? 0x10000 : -0x10000;
                }
                self->step_s++;
            }

            self->facingLeft = !(GetSideToPlayer() & 1);
            AnimateEntity(D_us_801806E0, self);
            UnkCollisionFunc2(D_us_801806C8);

            {
                s32 worldX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
                s32 diff = worldX - ENTITY_UNK84(self);
                s32 flag = (self->velocityX > 0) ? (diff > 0) : (diff < 0);

                if (flag) {
                    ENTITY_UNK84(self) = 0;
                    SetStep(2);
                }
            }
            break;
        }

        case 8:
            switch (self->step_s) {
                case 0:
                    self->velocityY = -0x68000;
                    self->velocityX = self->facingLeft ? 0x30000 : -0x30000;
                    ENTITY_UNK8F(self) = 0;
                    self->step_s++;
                    // fallthrough
                case 1:
                    AnimateEntity(D_us_80180704, self);
                    UnkCollisionFunc3(D_us_801806D0);
                    if (self->velocityY > 0) {
                        PlaySfxPositional(0x6C6);
                    }
                    SetSubStep(2);
                    break;
                case 2:
                    AnimateEntity(D_us_80180714, self);
                    if (UnkCollisionFunc3(D_us_801806D0) & 1) {
                        if ((other->flags & 0x100) || ENTITY_UNK8F(other) == 0) {
                            SetStep(7);
                        } else {
                            self->facingLeft = (other->posX.i.hi < self->posX.i.hi) ? 0 : 1;
                            SetSubStep(3);
                        }
                    }
                    break;
                case 3:
                    if (AnimateEntity(D_us_80180850, self) == 0) {
                        SetSubStep(4);
                    }
                    break;
                case 4:
                    if (ENTITY_UNK8F(self) != 0) {
                        if (AnimateEntity(D_us_80180858, self) == 0) {
                            SetStep(7);
                        }
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xB:
            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180724, self) != 0) {
                        self->step_s++;
                    }
                    break;
                case 1:
                    ENTITY_UNK8F(other) = 1;
                    ENTITY_UNK80(self) = 0x40;
                    PlaySfxPositional(0x788);
                    self->step_s++;
                    // fallthrough
                case 2: {
                    if (!(g_Timer & 3)) {
                        Entity* child = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (child != NULL) {
                            func_us_801A9D5C(0x1A, self, child);
                            child->posX.i.hi += self->facingLeft ? 0x26 : -0x26;
                            child->posY.i.hi -= 4;
                            child->rotate = -0x500;
                            child->facingLeft = self->facingLeft;
                            child->zPriority = self->zPriority + 1;
                        }
                    }
                    ENTITY_UNK80(self)--;
                    if (ENTITY_UNK80(self) == 0) {
                        SetStep(7);
                    }
                    break;
                }
                default:
                    break;
            }
            break;

        case 0xC:
            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_80180734, self) == 0) {
                        ENTITY_UNK8F(other) = 1;
                        SetSubStep(1);
                    }
                    break;
                case 1:
                    self->velocityX = self->facingLeft ? -0x48000 : 0x48000;
                    self->velocityY = 0;
                    PlaySfxPositional(0x615);
                    self->animCurFrame = 0x19;
                    self->step_s++;
                    // fallthrough
                case 2:
                    if (UnkCollisionFunc2(D_us_801806C8) & 0xF0) {
                        ENTITY_UNK80(self) = 1;
                        self->step_s++;
                    }
                    break;
                case 3:
                    AnimateEntity(D_us_8018073C, self);
                    ENTITY_UNK80(self)--;
                    if (ENTITY_UNK80(self) == 0) {
                        SetSubStep(4);
                    }
                    break;
                default:
                    SetStep(7);
                    break;
            }
            break;

        case 0xE: {
            if (AnimateEntity(D_us_80180750, self) == 0) {
                SetStep(7);
            }
            if (self->pose == 8) {
                s32 i;
                for (i = 0; i < 8; i++) {
                    Entity* child = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                    if (child != NULL) {
                        func_us_801A9D5C(0x19, self, child);
                        child->posY.i.hi += 0x28;
                        child->params = i;
                        child->facingLeft = self->facingLeft;
                    }
                }
            }
            break;
        }

        case 0x10:
            if (AnimateEntity(D_us_80180790, self) == 0) {
                SetStep(2);
            }
            break;

        case 0x12: {
            if (self->step_s == 0) {
                self->velocityX = 0;
                self->velocityY = 0;
                self->facingLeft = !(GetSideToPlayer() & 1);
                self->step_s++;
            }
            UnkCollisionFunc2(D_us_801806C8);
            if (AnimateEntity(D_us_80180798, self) == 0) {
                SetStep(2);
            }
            if (other->flags & 0x100) {
                s32 v = self->velocityX;
                v -= (v >= 0 ? v : v + 7) >> 3;
                self->velocityX = v;
                if (self->pose == 7) {
                    PlaySfxPositional(0x786);
                    self->velocityX = self->facingLeft ? 0x40000 : -0x40000;
                }
            }
            break;
        }

        case 0x14:
            if (AnimateEntity(D_us_801807AC, self) == 0) {
                SetStep(0x18);
            }
            break;

        case 0x16:
            switch (self->step_s) {
                case 0:
                    if (AnimateEntity(D_us_801807D0, self) == 0) {
                        SetSubStep(1);
                    }
                    if (self->pose == 5) {
                        PlaySfxPositional(0x789);
                    }
                    if (self->pose == 3) {
                        s32 atk = self->attack;
                        ENTITY_UNK90(self + 1) = 1;
                        ENTITY_UNK90(self) = 0;
                        ENTITY_UNK9C(self) = atk;
                        self->attack = atk * 2;
                    }
                    break;
                case 1:
                    self->velocityX = self->facingLeft ? -0x28000 : 0x28000;
                    self->velocityY = 0;
                    self->step_s++;
                    // fallthrough
                case 2:
                    AnimateEntity(D_us_801807F4, self);
                    if (UnkCollisionFunc2(D_us_801806C8) & 0xF0) {
                        ENTITY_UNK8F(other) = 1;
                        SetSubStep(3);
                        break;
                    }
                    if (GetDistanceToPlayerX() < 8) {
                        ENTITY_UNK8F(other) = 1;
                        PlaySfxPositional(0x787);
                        SetSubStep(3);
                    }
                    break;
                case 3:
                    if (AnimateEntity(D_us_801807FC, self) == 0) {
                        SetSubStep(4);
                        self->velocityX = 0;
                        self->velocityY = -0x70000;
                    }
                    break;
                case 4:
                    AnimateEntity(self->velocityY >= 0 ? D_us_80180810 : D_us_80180808, self);
                    if (UnkCollisionFunc3(D_us_801806D0) & 1) {
                        PlaySfxPositional(0x646);
                        self->attack = ENTITY_UNK9C(self);
                        ENTITY_UNK90(self + 1) = 0;
                        SetSubStep(5);
                    }
                    break;
                case 5:
                    AnimateEntity(D_us_80180818, self);
                    if (ENTITY_UNK90(self) != 0) {
                        SetStep(7);
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0x18:
            if (AnimateEntity(D_us_801807B4, self) == 0) {
                SetStep(0x16);
            }
            break;

        case 0x21:
            switch (self->step_s) {
                case 0:
                    self->animCurFrame = 0x2B;
                    self->step_s++;
                    break;
                case 1:
                    self->facingLeft ^= 1;
                    self->step_s++;
                    break;
                case 2:
                    self->animCurFrame = 2;
                    SetStep(7);
                    break;
                default:
                    break;
            }
            break;

        case 0x22:
            switch (self->step_s) {
                case 0:
                    if (other->posX.i.hi < self->posX.i.hi) {
                        ENTITY_UNK8F(self) = 0;
                        self->facingLeft = 0;
                    } else {
                        self->facingLeft = 1;
                    }
                    if (other->flags & 0x100) {
                        SetStep(0x12);
                        return;
                    }
                    self->step_s++;
                    // fallthrough
                case 1:
                    if (AnimateEntity(D_us_80180770, self) == 0) {
                        SetSubStep(2);
                    }
                    break;
                case 2:
                    if (ENTITY_UNK8F(self) != 0) {
                        PlaySfxPositional(0x786);
                        self->step_s++;
                    }
                    if (other->flags & 0x100) {
                        SetSubStep(3);
                        return;
                    }
                    break;
                case 3:
                    if (AnimateEntity(D_us_8018077C, self) == 0) {
                        SetStep(7);
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0x40:
            ENTITY_UNK8D(self) = 1;
            ENTITY_UNK8F(other) = 1;
            switch (self->step_s) {
                case 0:
                    self->hitboxState = 0;
                    self->velocityX = 0;
                    self->velocityY = 0;
                    self->step_s++;
                    // fallthrough
                case 1:
                    if (UnkCollisionFunc3(D_us_801806D0) & 1) {
                        ENTITY_UNK84(self) = -0x10;
                        ENTITY_UNK80(self) = 0;
                        PlaySfxPositional(0x654);
                        self->step_s++;
                    }
                    break;
                case 2:
                    if (AnimateEntity(D_us_80180824, self) == 0) {
                        self->step_s = 3;
                    }
                    ENTITY_UNK80(self) &= 1;
                    // fallthrough
                case 3: {
                    if (ENTITY_UNK80(self) & 1) {
                        Entity* child = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (child != NULL) {
                            func_us_801A9D5C(0x21, self, child);
                            child->posX.i.hi += (u16) ENTITY_UNK84(self);
                            child->posY.i.hi += 0x20;
                            child->zPriority += 1;
                            {
                                s32 mag = ENTITY_UNK84(self);
                                if (mag < 0) {
                                    mag = -mag;
                                }
                                mag += (s16) ENTITY_UNK80(self) >> 1;
                                child->rotate = mag;
                            }
                        }
                        ENTITY_UNK84(self) += 8;
                        if (ENTITY_UNK84(self) >= 0x11) {
                            ENTITY_UNK84(self) = -0x10;
                        }
                    }
                    {
                        u16 old = ENTITY_UNK80(self);
                        if (!(g_Timer & 0x1F)) {
                            PlaySfxPositional(0x661);
                        }
                        ENTITY_UNK80(self) = old + 1;
                        if ((s16) old >= 0x41) {
                            self->step_s++;
                        }
                    }
                    break;
                }
                case 4:
                    self->animCurFrame = 0;
                    ENTITY_UNK84(self) = 0;
                    break;
            }
            break;

        case 0xFF:
            FntPrint(D_us_801A1E8C, self->animCurFrame);
            if (g_pads_1_pressed & 0x80) {
                if (self->params == 0) {
                    self->animCurFrame++;
                    self->params |= 1;
                }
            } else {
                self->params = 0;
            }
            if (g_pads_1_pressed & 0x20) {
                if (self->step_s == 0) {
                    self->animCurFrame--;
                    self->step_s |= 1;
                }
            } else {
                self->step_s = 0;
            }
            break;

        default:
            break;
    }

    {
        u8 idx = D_us_80180908[self->step];
        u8* rec = &D_us_80180880[idx * 4];
        self->hitboxOffX = (s8) rec[0];
        self->hitboxOffY = (s8) rec[1];
        self->hitboxWidth = rec[2];
        self->hitboxHeight = rec[3];
    }
    func_us_801A2610();
}

// A hitbox/limb-tracking sub-entity permanently linked to the entity right
// before it in the array (self-1), which must be the main boss "conductor"
// (E_UNK_17 == func_us_801A269C) or this self-destructs. step 0: inits,
// falls into step 1. step 1: copies position/facing from the conductor and
// derives its own hitbox rect from a per-conductor-animCurFrame lookup
// table; advances to step 2 once ENTITY_UNK90(self) is set externally.
// step 2: launches upward (a jump-up sub-state), then an arc/fall sub-state
// with a landing-sound tick, ending once posY.i.hi crosses -0x40, then a
// step 2 (sub-state 2) that waits for ENTITY_UNK90(self) to clear before
// advancing to step 3. step 3: snaps X to the conductor's X, arcs upward,
// and once close enough to the conductor's Y, signals it back
// (ENTITY_UNK90(prev) = 1) and returns to step 1.
void func_us_801A3818(Entity* self) {
    Entity* prev = self - 1;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_8018053C);
            self->animCurFrame = 0;
            // fallthrough
        case 1:
            if (prev->entityId != 0x17) {
                DestroyEntity(self);
                break;
            }
            {
                u8 frameIndex = D_us_80180A00[prev->animCurFrame];
                u8* rec = &D_us_80180964[frameIndex * 4];
                self->hitboxState = 1;
                self->facingLeft = prev->facingLeft;
                self->posX = prev->posX;
                self->posY = prev->posY;
                self->hitboxOffX = (s8) rec[0];
                self->hitboxOffY = (s8) rec[1];
                self->hitboxWidth = rec[2];
                self->hitboxHeight = rec[3];
                if (ENTITY_UNK90(self)) {
                    SetStep(2);
                }
            }
            break;
        case 2:
            if (self->step_s == 0) {
                self->animCurFrame = 0x42;
                self->drawFlags = 4;
                self->facingLeft = prev->facingLeft;
                self->hitboxState = 0;
                self->velocityY = -0x80000;
                self->velocityX = self->facingLeft ? 0x4000 : -0x4000;
                self->step_s++;
                // fallthrough
            } else if (self->step_s != 1) {
                if (self->step_s == 2) {
                    if (!ENTITY_UNK90(self)) {
                        SetStep(3);
                    }
                }
                break;
            }
            MoveEntity();
            self->rotate += 0x100;
            self->velocityY += 0x1000;
            if (!(g_Timer & 0xF)) {
                PlaySfxPositional(0x625);
            }
            if (self->posY.i.hi < -0x40) {
                self->step_s++;
            }
            break;
        case 3:
            if (self->step_s == 0) {
                self->velocityY = 0x40000;
                self->velocityX = 0;
                self->posX.i.hi = prev->posX.i.hi;
                self->step_s++;
            } else if (self->step_s != 1) {
                break;
            }
            MoveEntity();
            self->rotate += 0x100;
            self->velocityY += 0x4000;
            if (prev->posY.i.hi - self->posY.i.hi < 0x20) {
                ENTITY_UNK90(prev) = 1;
                self->animCurFrame = 0;
                self->drawFlags = 0;
                SetStep(1);
            }
            break;
        default:
            break;
    }
}

// step 0: initializes, derives a launch velocity from a paired angle table
// indexed by self->params (rcos/rsin scaled by -table.first, mirrored on X
// when facingLeft), then falls into step 1's behavior. step 1: simple
// projectile motion (MoveEntity + gravity via velocityY += 0x2000). Any
// other step: no-op.
void func_us_801A3AC8(Entity* self) {
    switch (self->step) {
        case 0: {
            s32 first;
            InitializeEntity(D_us_80180548);
            first = -D_us_80180A5C[self->params * 2];
            self->velocityX = (first * rcos(D_us_80180A60[self->params * 2])) >> 12;
            self->velocityY = (first * rsin(D_us_80180A60[self->params * 2])) >> 12;
            if (self->facingLeft) {
                self->velocityX = -self->velocityX;
            }
        }
        // fallthrough
        case 1:
            MoveEntity();
            self->velocityY += 0x2000;
            break;
        default:
            break;
    }
}

// A small explosion/spark debris particle. step 0: initializes, randomizes
// launch angle around self->rotate (biased +0x80, minus Random()), derives
// velocityX/velocityY from rsin/rcos of that angle, and sets up its
// blend/draw/scale/opacity. step 1: moves, grows in scale, fades out via
// opacity, and self-destructs once its animation finishes.
void func_us_801A3BB4(Entity* self) {
    switch (self->step) {
        case 0: {
            s16 angle;
            InitializeEntity(D_us_80180554);
            self->palette = 0x816B;
            if (self->facingLeft) {
                self->rotate = -self->rotate;
            }
            self->facingLeft = 0;
            angle = self->rotate + 0x80 - Random();
            self->rotate = angle;
            self->velocityX = rsin(angle) << 5;
            self->velocityY = -(rcos(angle) << 5);
            self->blendMode = 0x30;
            self->drawFlags = 0xF;
            self->scaleX = 0x40;
            self->scaleY = 0x80;
            self->opacity = 0x80;
            break;
        }
        case 1:
            MoveEntity();
            self->scaleX += 0x10;
            self->opacity -= 1;
            self->scaleY += 0xE;
            if (!AnimateEntity(D_us_80180834, self)) {
                DestroyEntity(self);
            }
            break;
        default:
            break;
    }
}

// A fading smoke/dust particle. step 0: initializes (opacity derived from
// self->params, facingLeft randomized), falls into step 1's behavior.
// step 1: moves, decays velocityY/opacity/scaleY while growing scaleX, and
// self-destructs once opacity hits 0 or its animation finishes.
void func_us_801A3CF0(Entity* self) {
    switch (self->step) {
        case 0: {
            s16 params = self->params;
            InitializeEntity(g_EInitParticle);
            self->animSet = -0x7FFC;
            self->unk5A = 0x5C;
            self->palette = 0x21E;
            self->blendMode = 0x30;
            self->drawFlags = 0xB;
            self->scaleY = 0x180;
            self->scaleX = 0xA0;
            self->velocityY = -0x30000;
            self->opacity = -0x80 - (params * 2);
            self->facingLeft = Random() & 1;
        }
        // fallthrough
        case 1:
            MoveEntity();
            self->velocityY -= 0x1000;
            self->scaleX += 8;
            self->opacity -= 2;
            self->scaleY -= 4;
            if (self->opacity == 0 || !AnimateEntity(D_us_80180864, self)) {
                DestroyEntity(self);
            }
            break;
        default:
            break;
    }
}

// Boss-defeat / ending-trigger conductor (debug strings confirm this:
// "set_step %x\n" on self->step, "boss_go %x\n" on D_us_80180AAC). Operates
// on self+1 and self+4 (the two visible boss-part entities), plus two raw
// Entity-sized data blocks (D_80078618[0]/[1]) used as pseudo-entities for
// the ending's staff-roll trigger. Steps 5/6/7 cycle the two boss parts
// through a per-configuration position table (D_us_80180AB4), tracking the
// player's side via a sign-of-product test in step 7. Step 8 stops the
// music, spawns a fade-to-black child entity (entityId 0x24), and waits for
// g_api.func_80131F68() (fade done?) before restarting the boss theme.
void func_us_801A3E04(Entity* self) {
    Entity* e1 = self + 1;
    Entity* e4 = self + 4;
    s16 scrollX = g_Tilemap.scrollX.i.hi;
    s16 scrollY = g_Tilemap.scrollY.i.hi;

    FntPrint(D_us_801A1EF4, self->step);
    FntPrint(D_us_801A1F04, D_us_80180AAC);

    if (self->step >= 5) {
        if ((e1->flags & 0x100) && (e4->flags & 0x100) && self->step != 8) {
            SetStep(8);
        }
    }
    if (self->step >= 9) {
        return;
    }

    switch (self->step) {
        case 0: {
            s32 primIndex;
            Primitive* prim;

            InitializeEntity(D_us_801804E8);
            primIndex = (s16) g_api.AllocPrimitives(PRIM_G4, 1);
            if (primIndex == -1) {
                self->step = 0;
                return;
            }

            prim = &g_PrimBuf[primIndex];
            self->primIndex = primIndex;
            ENTITY_UNK7C(self) = prim;
            self->flags |= FLAG_HAS_PRIMS;

            // full-screen black quad (fade-to-black overlay), broadcasting
            // vertex 0's zeroed r0/g0/b0 word across vertices 1-3
            prim->b0 = 0;
            prim->g0 = 0;
            prim->r0 = 0;
            *(s32*) &prim->r1 = *(s32*) &prim->r0;
            *(s32*) &prim->r2 = *(s32*) &prim->r0;
            *(s32*) &prim->r3 = *(s32*) &prim->r0;

            prim->x1 = 0x100;
            prim->x3 = 0x100;
            prim->y3 = 0x100;
            prim->y2 = 0x100;
            prim->priority = 0x1F0;
            prim->x2 = 0;
            prim->x0 = 0;
            prim->y1 = 0;
            prim->y0 = 0;
            prim->drawMode = 8;

            ENTITY_UNK8C(self) = 0;
            // falls through
        }
        case 1:
            if (g_CastleFlags[0xB4] || g_PlayableCharacter || g_DemoMode) {
                if (GetDistanceToPlayerX() >= 0x10) {
                    return;
                }
                D_us_80180B74 = 1;
                stopMusicFlag = 0;
                currentMusicId = 0x31D;
                g_api.PlaySfx(0x31D);
            } else if (!(D_us_801B68EC & 0x10)) {
                return;
            }
            SetStep(2);
            return;

        case 2:
            switch (self->step_s) {
                case 0: {
                    Entity* ent1 = &D_80078618[0];
                    Entity* ent2 = &D_80078618[1];

                    func_us_801A9CE8(0x28, ent1);
                    ent1->posX.i.hi = 0x150 - scrollX;
                    ent1->posY.i.hi = 0xD0 - scrollY;
                    ent1->params = 0;
                    func_us_801A9CE8(0x28, ent2);
                    ent2->posX.i.hi = 0xA0 - scrollX;
                    ent2->posY.i.hi = ent1->posY.i.hi;
                    ent2->params = 1;

                    ENTITY_UNK80(self) = 0x80;
                    self->step_s++;
                    // falls through
                }
                case 1:
                    ENTITY_UNK80(self)--;
                    if (ENTITY_UNK80(self) == 0) {
                        SetStep(3);
                    }
                    return;
                default:
                    return;
            }

        case 3:
            func_us_801A9CE8(0x17, e4);
            func_us_801A9CE8(0x1B, e1);
            e4->posX.i.hi = 0x160 - scrollX;
            e4->posY.i.hi = 0xA8 - scrollY;
            e1->posX.i.hi = 0xA0 - scrollX;
            e1->posY.i.hi = 0xB5 - scrollY;
            g_api.TimeAttackController(4, 2);
            SetStep(4);
            // falls through

        case 4:
            switch (self->step_s) {
                case 0:
                    ENTITY_UNK80(self) = 0x100;
                    self->step_s++;
                    // falls through
                case 1:
                    ENTITY_UNK80(self)--;
                    if (ENTITY_UNK80(self) == 0) {
                        self->step_s++;
                        return;
                    }
                    return;
                case 2:
                    if (g_CastleFlags[0xB4] || g_PlayableCharacter || g_DemoMode) {
                        D_us_80180AAC = 1;
                    }
                    SetStep(5);
                    return;
                default:
                    return;
            }

        case 5: {
            s32 idx = ENTITY_UNK8C(self);
            BossPartInitEntry* entry = &D_us_80180AB4[idx];

            if (!(e4->flags & 0x100)) {
                ENTITY_UNK84(e4) = entry->unk0;
                ENTITY_UNK88(e4) = entry->unk4;
            }
            if (!(e1->flags & 0x100)) {
                ENTITY_UNK84(e1) = entry->unk8;
                ENTITY_UNK88(e1) = entry->unkC;
            }
            SetStep(6);
            return;
        }

        case 6: {
            s32 idx;
            BossPartInitEntry* entry;
            s16 dist;

            if (!ENTITY_UNK8D(e4)) {
                return;
            }
            if (!ENTITY_UNK8D(e1)) {
                return;
            }

            idx = ENTITY_UNK8C(self);
            entry = &D_us_80180AB4[idx];

            if (*(s32*) &entry->unkC == 0xE && *(s32*) &entry->unk4 == 0xC) {
                dist = e4->posX.i.hi - e1->posX.i.hi;
                if (dist < 0) {
                    dist = -dist;
                }
                if (dist < 0x40) {
                    ENTITY_UNK88(e4) = 0xB;
                    ENTITY_UNK88(e1) = 0x18;
                }
            }
            ENTITY_UNK8E(e1) = 1;
            ENTITY_UNK8E(e4) = 1;
            SetStep(7);
            return;
        }

        case 7: {
            u8 idx;
            BossPartInitEntry* entry;

            ENTITY_UNK8C(self)++;
            idx = ENTITY_UNK8C(self);
            entry = &D_us_80180AB4[idx];

            if (*(s32*) &entry->unkC == 0x18) {
                s32 a = e4->posX.i.hi - PLAYER_posX_i_hi;
                s32 b = e1->posX.i.hi - PLAYER_posX_i_hi;
                if (a * b < 0) {
                    ENTITY_UNK8C(self) = idx + 1;
                    entry++;
                }
            }
            if (entry->unk0 == -1) {
                ENTITY_UNK8C(self) = 0;
            }
            SetStep(5);
            return;
        }

        case 8:
            switch (self->step_s) {
                case 0:
                    g_api.PlaySfx(0x92);
                    ENTITY_UNK80(self) = 0x100;
                    g_api.TimeAttackController(4, 1);
                    self->step_s++;
                    // falls through
                case 1:
                    ENTITY_UNK80(self)--;
                    if (ENTITY_UNK80(self) != 0) {
                        return;
                    }
                    self->step_s++;
                    if (g_CastleFlags[0xB4] || g_PlayableCharacter || g_DemoMode) {
                        self->step_s++;
                        return;
                    }
                    func_us_801A9CE8(0x25, &D_8007C6B8);
                    D_8007C6E8 = 1;
                    func_us_801A9CE8(0x26, (void*) ((u8*) &D_8007C6B8 - 0x4C60));
                    D_80077A88 = 1;
                    return;
                case 2:
                    if (D_us_801B68EC & 0x1000) {
                        self->step_s++;
                    }
                    return;
                case 3: {
                    Entity* child = AllocEntity(&g_Entities[160], &g_Entities[192]);
                    if (child == NULL) {
                        return;
                    }
                    func_us_801A9D5C(0x24, self, child);
                    stopMusicFlag = 1;
                    D_us_80180B74 = 0;
                    child->posX.i.hi = 0x100 - scrollX;
                    child->params = 2;
                    child->posY.i.hi = 0x80 - scrollY;
                    currentMusicId = 0x323;
                    self->step_s++;
                    return;
                }
                case 4:
                    if (g_api.func_80131F68()) {
                        return;
                    }
                    stopMusicFlag = 0;
                    g_api.PlaySfx(currentMusicId);
                    self->step_s++;
                    return;
                default:
                    return;
            }

        default:
            return;
    }
}

// Boss-defeat "flash" overlay controller: allocates a 2-entry PRIM_GT4 chain
// used purely as PRIM_ENV draw-environment-change primitives (one covering
// most of the screen below y=0x14, one restoring the previous draw env right
// after it in the display list), fades a secondary counter in over ~30
// frames (step_s 0/1/2), then shrinks the overlay's clip rect toward
// self->posX before spawning a smoke/debris child entity (entityId 0x29)
// with a randomized X offset once D_us_80180AB0 clears.
void func_us_801A460C(Entity* self) {
    Primitive* p1;
    Primitive* p2;
    DR_ENV* dr_env;
    DRAWENV env;
    RECT rect;
    Entity* child;
    s32 primIndex;
    s32 t0, t1, t4;
    s16 scrollY;

    if (self->step == 0) {
        InitializeEntity(D_us_801804E8);
        primIndex = (s16) g_api.AllocPrimitives(PRIM_GT4, 2);
        if (primIndex == -1) {
            DestroyEntity(self);
            return;
        }

        p1 = &g_PrimBuf[primIndex];
        self->primIndex = primIndex;
        ENTITY_UNK7C(self) = p1;
        self->flags |= FLAG_HAS_PRIMS;

        dr_env = g_api.func_800EDB08((POLY_GT4*) p1);
        if (dr_env == NULL) {
            DestroyEntity(self);
            return;
        }
        p1->type = PRIM_ENV;

        env = g_CurrentBuffer->draw;
        env.ofs[0] = 0;
        rect.x = 0;
        rect.y = 0x14;
        rect.w = 0x100;
        rect.h = 0xCF;
        env.clip = rect;
        env.isbg = 0;
        env.r0 = 0;
        env.g0 = 0;
        env.b0 = 0;
        env.dtd = 0;
        SetDrawEnv(dr_env, &env);

        p1->drawMode = 8;
        p1->priority = self->params * 4 + 0x9F;

        p2 = p1->next;
        if (g_api.func_800EDB08((POLY_GT4*) p2) == NULL) {
            DestroyEntity(self);
            return;
        }
        p2->type = PRIM_ENV;
        p2->drawMode = 0x808;
        p2->priority = self->params * 4 + 0xA1;
        ENTITY_UNK88(self) = 0;
    } else if (self->step != 1) {
        return;
    }

    switch (self->step_s) {
        case 0:
            ENTITY_UNK80(self)++;
            if (ENTITY_UNK80(self) & 1) {
                ENTITY_UNK88(self)++;
            }
            if ((g_Timer & 0xF) == 0) {
                g_api.PlaySfx(0x661);
            }
            if ((s16) ENTITY_UNK88(self) >= 0x1E) {
                self->step_s++;
            }
            break;

        case 1:
            if (D_us_80180AB0) {
                ENTITY_UNK80(self) = 0x80;
                self->step_s++;
            }
            if ((g_Timer & 0xF) == 0) {
                g_api.PlaySfx(0x661);
            }
            break;

        case 2:
            ENTITY_UNK80(self)--;
            if (ENTITY_UNK80(self) == 0) {
                DestroyEntity(self);
                return;
            }
            break;

        default:
            break;
    }

    t0 = self->posX.i.hi - (s16) ENTITY_UNK88(self);
    t1 = self->posX.i.hi + (s16) ENTITY_UNK88(self);
    scrollY = g_Tilemap.scrollY.i.hi;
    t4 = 0xD0 - scrollY;

    if (t0 < 0) {
        t0 = 0;
    }
    if (t0 >= 0x101) {
        t0 = 0x100;
    }
    if (t1 < 0) {
        t1 = 0;
    }
    if (t1 >= 0x101) {
        t1 = 0x100;
    }

    p1 = ENTITY_UNK7C(self);
    dr_env = *(DR_ENV**) &p1->r1;
    env = g_CurrentBuffer->draw;
    rect.x = env.ofs[0] + t0;
    rect.y = 0x14;
    rect.w = t1 - t0;
    rect.h = t4 - 0x15;
    env.clip = rect;
    env.isbg = 0;
    SetDrawEnv(dr_env, &env);

    p1->drawMode = 0;
    p2 = p1->next;
    p2->drawMode = 0x800;

    if (D_us_80180AB0) {
        return;
    }
    if (g_Timer & 1) {
        return;
    }

    if (Random() & 1) {
        child = AllocEntity(&g_Entities[144], &g_Entities[256]);
        if (child == NULL) {
            return;
        }
        func_us_801A9CE8(0x29, child);
        child->zPriority = self->params * 4 + 0xA0;
        child->posX.i.hi = self->posX.i.hi - (s16) ENTITY_UNK88(self) / 2;
        child->posY.i.hi = 0xD0;
        child->posX.i.hi -= Random() & 0xF;
        child->rotate = Random() + 0x40;
    } else {
        child = AllocEntity(&g_Entities[144], &g_Entities[256]);
        if (child == NULL) {
            return;
        }
        func_us_801A9CE8(0x29, child);
        child->zPriority = self->params * 4 + 0xA0;
        child->posX.i.hi = self->posX.i.hi + (s16) ENTITY_UNK88(self) / 2;
        child->posY.i.hi = 0xD0;
        child->posX.i.hi += Random() & 0xF;
        child->rotate = -0x40 - (Random() & 0xF);
    }
}

// A small orbiting particle (its ext+0x10 field, named orbitRadius here,
// scales rsin/rcos(self->rotate) to steer velocityX/velocityY every frame,
// producing a circling motion). step 0: initializes, picks an initial
// velocity and orbitRadius (Random()-derived), falls into step 1. step 1:
// moves along the orbit, fades opacity, self-destructs once its animation
// finishes.
typedef struct {
    /* 0x7C */ u8 pad7C[0x10];
    /* 0x8C (ext+0x10) */ s32 orbitRadius;
} ET_801A4BA4;

void func_us_801A4BA4(Entity* self) {
    ET_801A4BA4* ext = (ET_801A4BA4*) &self->ext;

    switch (self->step) {
        case 0: {
            s16 angle;
            InitializeEntity(g_EInitParticle);
            self->animSet = 0xE;
            self->unk5A = 0x79;
            self->palette = 0x227;
            self->blendMode = 0x30;
            self->drawFlags = 0xC;
            self->opacity = 0x80;
            self->facingLeft = Random() & 1;
            self->scaleY = 0x180;
            self->drawFlags |= 2;
            angle = self->rotate;
            if (self->facingLeft) {
                angle = -angle;
            }
            self->velocityX = rsin(angle) * 40;
            self->velocityY = -(rcos(angle) * 40);
            ext->orbitRadius = (Random() * 16) + 0x1000;
        }
        // fallthrough
        case 1: {
            s16 angle;
            MoveEntity();
            angle = self->rotate;
            self->opacity -= 2;
            if (self->facingLeft) {
                angle = -angle;
            }
            self->velocityX += (rsin(angle) * ext->orbitRadius) >> 12;
            self->velocityY += (-ext->orbitRadius * rcos(angle)) >> 12;
            if (!AnimateEntity(D_us_80180B44, self)) {
                DestroyEntity(self);
            }
            break;
        }
        default:
            break;
    }
}

// step 0: initializes, sizes itself via a self->params-indexed lookup
// table, falls into step 1. step 1: just animates and flickers its
// palette (alternating every 8 frames via g_Timer bit 2). step 0xFF: debug
// frame-stepper (same pad-driven animCurFrame nudger pattern already seen
// in other bosses -- L1/R1 style buttons single-step forward/back while
// held, using self->params/step_s as one-shot-per-press latches).
void func_us_801A4D4C(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_80180524);
            self->drawFlags = 3;
            self->scaleY = self->scaleX = D_us_80180B6C[self->params];
            // fallthrough
        case 1:
            AnimateEntity(D_us_80180B60, self);
            if (g_Timer & 4) {
                self->palette = self->params * 2 - 0x7FA6;
            } else {
                self->palette = self->params * 2 - 0x7FA5;
            }
            break;
        case 0xFF:
            FntPrint(D_us_801A1F54, self->animCurFrame);
            if (g_pads_1_pressed & 0x80) {
                if (self->params != 0) {
                    break;
                }
                self->animCurFrame++;
                self->params |= 1;
            } else {
                self->params = 0;
            }
            if (g_pads_1_pressed & 0x20) {
                if (self->step_s != 0) {
                    break;
                }
                self->animCurFrame--;
                self->step_s |= 1;
            } else {
                self->step_s = 0;
            }
            break;
        default:
            break;
    }
}

// Main conductor for the two visible boss-part entities: self+1 and self+2
// (raw offsets 0xBC/0x178 = 1x/2x sizeof(Entity)) are re-tagged as entityId
// 0x23 and nudged into position relative to self, mirrored depending on
// self->params (0 vs nonzero picks one screen side). Steps 2/4 slide
// self->posX/posY back toward center over a short timer (ENTITY_UNK80);
// step 0x10 blinks self->palette while a cutscene flag is set; step 0xFF is
// the same g_pads_1_pressed-driven debug frame-stepper seen in
// func_us_801A4D4C, reusing D_us_80180524/D_us_801A1F60.
void func_us_801A4EE0(Entity* self) {
    Entity* e1 = self + 1;
    Entity* e2 = self + 2;
    s16 newTimer;

    switch (self->step) {
        case 0: {
            u16 params;

            InitializeEntity(D_us_80180524);
            params = self->params;

            if (params & 0x100) {
                self->animSet = 2;
                self->zPriority = 0x6A;
                self->step = 0x10;
                return;
            }
            if (params & 0x200) {
                self->animSet = 3;
                self->zPriority = 0x6B;
                self->step = 0x20;
                return;
            }

            self->animSet = 1;
            self->zPriority = 0x6A;

            if (self->params != 0) {
                self->facingLeft = 0;
                self->posX.i.hi = 0x1B1 - g_Tilemap.scrollX.i.hi;
                self->posY.i.hi = 0x96 - g_Tilemap.scrollY.i.hi;
                func_us_801A9D5C(0x23, self, e1);

                e1->params = 0x100;
                e1->posX.i.hi += 7;
                e1->posY.i.hi += 0x14;
                e1->facingLeft = self->facingLeft;
                func_us_801A9D5C(0x23, self, e2);

                e2->params = 0x200;
                e2->posX.i.hi += 6;
                e2->facingLeft = self->facingLeft;
                self->posY.i.hi += 0x50;
                e2->posY.i.hi += 0x41;
            } else {
                self->facingLeft = 1;
                self->posX.i.hi = 0x4F - g_Tilemap.scrollX.i.hi;
                self->posY.i.hi = 0xA6 - g_Tilemap.scrollY.i.hi;
                func_us_801A9D5C(0x23, self, e1);

                e1->params = 0x100;
                e1->posX.i.hi -= 7;
                e1->facingLeft = self->facingLeft;
                e1->posY.i.hi += 0xD;
                func_us_801A9D5C(0x23, self, e2);

                e2->params = 0x200;
                e2->posX.i.hi -= 8;
                e2->facingLeft = self->facingLeft;
                e2->posY.i.hi += 0x31;
                self->posY.i.hi += 0x40;
            }
            // falls through to case 1's shared body
        }
        case 1:
            if (!D_us_80180B74) {
                return;
            }
            g_api.PlaySfx(0x607);
            ENTITY_UNK80(self) = (self->params == 0) ? 0x40 : 0x50;

            TILEMAP_RAW16(0x208) = 0xF6;
            TILEMAP_RAW16(0x248) = 0xFE;
            TILEMAP_RAW16(0x288) = 0x106;
            TILEMAP_RAW16(0x2C8) = 0xFE;
            TILEMAP_RAW16(0x1F6) = 0xEE;
            TILEMAP_RAW16(0x236) = 0x17F;
            TILEMAP_RAW16(0x276) = 0x187;
            TILEMAP_RAW16(0x2B6) = 0x17F;
            TILEMAP_RAW16(0x2F6) = 0x187;
            goto shared_tail;

        case 2:
            if (ENTITY_UNK80(self) & 1) {
                self->posX.i.hi++;
            } else {
                self->posX.i.hi--;
            }
            self->posY.i.hi--;
            newTimer = ENTITY_UNK80(self) - 1;
            ENTITY_UNK80(self) = newTimer;
            if (newTimer != 0) {
                return;
            }
            PlaySfxPositional(0x675);
            goto shared_tail;

        case 3:
            ENTITY_UNK80(self) = 0x60;
            if (D_us_80180B74) {
                return;
            }
            TILEMAP_RAW16(0x208) = 0x1A5;
            TILEMAP_RAW16(0x248) = 0x1A6;
            TILEMAP_RAW16(0x288) = 0x1A7;
            TILEMAP_RAW16(0x2C8) = 0x1A6;
            TILEMAP_RAW16(0x1F6) = 0x1A4;
            TILEMAP_RAW16(0x236) = 0x1A8;
            TILEMAP_RAW16(0x276) = 0x1A9;
            TILEMAP_RAW16(0x2B6) = 0x1A8;
            TILEMAP_RAW16(0x2F6) = 0x1A9;
            goto shared_tail;

        case 4:
            if (ENTITY_UNK80(self) & 1) {
                self->posX.i.hi++;
            } else {
                self->posX.i.hi--;
            }
            self->posY.i.hi++;
            newTimer = ENTITY_UNK80(self) - 1;
            ENTITY_UNK80(self) = newTimer;
            if (newTimer != 0) {
                return;
            }
            goto shared_tail;

        case 0x10:
            if (D_us_80180B74) {
                self->palette = (g_Timer & 1) + 1;
            } else {
                self->palette = 0;
            }
            return;

        case 0x20:
            return;

        case 0xFF:
            FntPrint(D_us_801A1F60, self->animCurFrame);

            if (g_pads_1_pressed & 0x80) {
                if (self->params != 0) {
                    return;
                }
                self->animCurFrame++;
                self->params |= 1;
            } else {
                self->params = 0;
            }

            if (g_pads_1_pressed & 0x20) {
                if (self->step_s != 0) {
                    return;
                }
                self->animCurFrame--;
                self->step_s |= 1;
            } else {
                self->step_s = 0;
            }
            return;

        default:
            return;
    }

shared_tail:
    self->step++;
}

// Resets the shared boss-AI scratch block's state/sub-state pair to 2/2,
// clears the index/timer field (D_us_801B684E), and derives D_us_801B6846
// from D_us_801B6848 (+0x14). Also zeroes D_us_801B6852 and the small
// flag pair at D_us_801B6856/6857 (0/8).
void func_us_801A5460(void) {
    D_us_801B684C = 2;
    D_us_801B684A = 2;
    D_us_801B6846 = D_us_801B6848 + 0x14;
    D_us_801B684E = 0;
    D_us_801B6852 = 0;
    D_us_801B6856 = 0;
    D_us_801B6857 = 8;
}

// Allocates a 7(+1?)-node PRIM_SPRT chain and wires up the shared boss-AI
// state block from it: the first 6 nodes become the tracked primitive-slot
// globals (D_us_801B6858/685C/6860/6864/6868/686C, all initialized with
// drawMode = 8), then two more nodes reached by walking ->next past the
// 6th slot (not separately named -- always reached via that chain) are set
// up as a PRIM_G4 gouraud quad and a plain PRIM_TILE, likely a gradient
// bar/shockwave effect. Also resets D_us_801B687C/6874/6870 and re-arms
// D_us_801B684E's state via func_us_801A5460. Returns 1 on success, 0 if
// the primitive allocation failed.
s32 func_us_801A54B4(Entity* self) {
    Primitive *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
    s32 primIndex = (s16) g_api.AllocPrimitives(PRIM_SPRT, 7);

    D_us_801B6878 = primIndex;
    if (primIndex == -1) {
        D_us_801B6878 = 0;
        return 0;
    }

    D_us_801B6840 = (s32) self;
    D_us_801B687C = 0;
    D_us_801B6874 = -1;
    D_us_801B6870 = -1;
    func_us_801A5460();

    p1 = &g_PrimBuf[primIndex];
    D_us_801B6858[0] = p1;
    p2 = p1->next;
    p1->drawMode = 8;
    D_us_801B6858[1] = p2;
    p3 = p2->next;
    p2->drawMode = 8;
    D_us_801B6858[2] = p3;
    p4 = p3->next;
    p3->drawMode = 8;
    D_us_801B6858[3] = p4;
    p5 = p4->next;
    p4->drawMode = 8;
    D_us_801B6858[4] = p5;
    p6 = p5->next;
    p5->drawMode = 8;
    D_us_801B686C = p6;
    p7 = p6->next;
    p6->type = PRIM_GT4;
    p6->drawMode = 8;

    p7->type = PRIM_G4;
    p7->r3 = p7->r2 = p7->r1 = p7->r0 = 0xFF;
    p7->g3 = p7->g2 = p7->g1 = p7->g0 = 0;
    p7->b3 = p7->b2 = p7->b1 = p7->b0 = 0;
    p7->x2 = 4;
    p7->x0 = 4;
    p7->x3 = 0xF8;
    p7->x1 = 0xF8;
    p7->priority = 0x1FD;
    p7->drawMode = 8;

    p8 = p7->next;
    p8->type = PRIM_TILE;
    p8->x0 = 3;
    p8->y0 = 0x2F;
    p8->v0 = 0x4A;
    p8->b0 = 0xFF;
    p8->g0 = 0xFF;
    p8->r0 = 0xFF;
    p8->priority = 0x1FC;
    p8->drawMode = 8;

    return 1;
}

// Clears a small horizontal strip of the frame buffer to black -- the
// strip's Y position is derived from `index` (each index step covers a
// 12-pixel-tall band starting at Y=0x180).
void func_us_801A5678(s16 index) {
    RECT rect;
    rect.x = 0;
    rect.y = index * 12 + 0x180;
    rect.w = 0x40;
    rect.h = 0xC;
    ClearImage(&rect, 0, 0, 0);
}

// Initializes the Primitive at D_us_801B6858[index] (index =
// D_us_801B684E, the shared boss-AI state block's small primitive-pointer
// array -- see bo2.h): clears a tile strip via func_us_801A5678(index),
// then sets its position/clut/UV/texpage/priority so it draws as a plain
// solid tile, with drawMode reset to 0 (visible).
void func_us_801A56D4(void) {
    s16 index = D_us_801B684E;
    Primitive* prim = D_us_801B6858[index];

    func_us_801A5678(index);

    prim->tpage = 0x10;
    prim->clut = D_us_801B6854;
    prim->u0 = 0;
    prim->y0 = D_us_801B6846;
    prim->x0 = D_us_801B6844 + 4;
    prim->u1 = 0xC0;
    prim->v1 = 0xC;
    prim->priority = 0x1FF;
    prim->drawMode = 0;
    prim->v0 = (u8) ((index * 12) - 0x80);
}

// Renders a bitmap-font string (D_us_80180C20[stringId], 0xFF-escaped,
// 0-terminated) as a chain of PRIM_SPRT glyph tiles: counts visible glyphs
// (escape sequences don't count), allocates that many primitives, and lays
// each one out left-to-right (8px advance, 4px on skipped/escape bytes),
// using the low/high nibble of each byte as the glyph's sheet column/row.
// Destroys `self` if the primitive allocation fails.
void func_us_801A578C(s32 stringId, Entity* self) {
    u8* str = D_us_80180C20[stringId];
    s32 count = 0;
    s32 primIndex;
    Primitive* prim;
    s32 x;

    for (;;) {
        u8 c = *str++;
        if (c == 0xFF) {
            c = *str++;
            if (c == 0) {
                break;
            }
        } else if (c == 0) {
            break;
        }
        count++;
    }

    primIndex = (s16) g_api.AllocPrimitives(PRIM_SPRT, count);
    if (primIndex == -1) {
        DestroyEntity(self);
        return;
    }

    prim = &g_PrimBuf[primIndex];
    D_us_801B6874 = primIndex;
    str = D_us_80180C20[stringId];

    if (prim != NULL) {
        x = 0x38;
        do {
            u8 c = *str++;
            if (c == 0) {
                x += 4;
            } else {
                prim->u0 = (c & 0xF) << 3;
                prim->type = PRIM_SPRT;
                prim->tpage = 0x1E;
                prim->clut = 0x196;
                prim->v0 = (c & 0xF0) >> 1;
                prim->v1 = 8;
                prim->u1 = 8;
                prim->priority = 0x1FF;
                prim->drawMode = 8;
                prim->x0 = x;
                prim->y0 = D_us_801B6848 + 6;
                x += 8;
                prim = prim->next;
            }
        } while (prim != NULL);
    }
}

// Biases arg0 by 0x100000 (fixing up an overlay-relative script address to
// an absolute one) and stores it as D_us_801B6880, the bytecode VM's script
// cursor (see func_us_801A593C) -- then arms the shared boss-AI scratch
// block (D_us_801B687C = 1, D_us_801B687E = 0).
void func_us_801A5910(u8* arg0) {
    D_us_801B6880 = arg0 + 0x100000;
    D_us_801B687E = 0;
    D_us_801B687C = 1;
}

// A small timestamped bytecode VM driving a scripted sequence in
// g_Entities_64, stepped once per call. Each event is
// {u16 timestamp; u8 opcode; u8 params[]}; if D_us_801B687E (a
// free-running frame counter) hasn't reached the event's timestamp yet,
// the cursor (D_us_801B6880) rewinds and this call is a no-op. Opcodes:
// 0 = respawn g_Entities_64[params[0]] with a new entityId + packed
// posX/posY (params[1..5]); 1 = destroy g_Entities_64[params[0]];
// 2 = wait until bit params[0] of D_us_801B68EC is set, then clear it;
// 3 = set bit params[0] of D_us_801B68EC (no wait). If the free-running
// counter wraps past 0xFFFE, the script is reset (D_us_801B687C = 0).
void func_us_801A593C(void) {
    D_us_801B687E++;
    if (D_us_801B687E > 0xFFFE) {
        D_us_801B687C = 0;
        return;
    }

    for (;;) {
        u8* eventStart = D_us_801B6880;
        u16 timestamp = (eventStart[0] << 8) | eventStart[1];
        u8 opcode = eventStart[2];
        u8* pc = eventStart + 3;

        if (D_us_801B687E < timestamp) {
            D_us_801B6880 = eventStart;
            return;
        }

        switch (opcode) {
            case 0: {
                Entity* e = &g_Entities_64[pc[0]];
                DestroyEntity(e);
                e->entityId = pc[1];
                e->pfnUpdate = (&D_us_80180410)[e->entityId];
                e->posX.i.hi = pc[2] << 4;
                e->posX.i.hi |= pc[3];
                e->posY.i.hi = pc[4] << 4;
                e->posY.i.hi |= pc[5];
                D_us_801B6880 = pc + 6;
                break;
            }
            case 1:
                DestroyEntity(&g_Entities_64[pc[0]]);
                D_us_801B6880 = pc + 1;
                break;
            case 2:
                if (!((D_us_801B68EC >> pc[0]) & 1)) {
                    D_us_801B6880 = eventStart + 2;
                    return;
                }
                D_us_801B68EC &= ~(1 << pc[0]);
                D_us_801B6880 = pc + 1;
                break;
            case 3:
                D_us_801B68EC |= 1 << pc[0];
                D_us_801B6880 = pc + 1;
                break;
            default:
                /* unreachable in practice: real asm doesn't advance the
                 * cursor here either, so this would spin forever -- kept
                 * faithful to the original rather than "fixed". */
                break;
        }
    }
}

// Debug/cheat trigger, only armed when g_pads_0_tapped == 0x800 (a single
// specific button tapped this frame): frees this entity's own primitive
// plus two other tracked primitive slots from the shared boss-AI state
// block (D_us_801B6874/6870, each guarded by a -1 "unused" sentinel),
// plays a sound, and resets step/step_s to 1/0.
void func_us_801A5BE8(Entity* self) {
    if (g_pads_0_tapped == 0x800) {
        g_api.FreePrimitives(self->primIndex);
        D_us_801B683C = 1;
        self->flags ^= FLAG_HAS_PRIMS;
        if (D_us_801B6874 != -1) {
            g_api.FreePrimitives(D_us_801B6874);
        }
        if (D_us_801B6870 != -1) {
            g_api.FreePrimitives(D_us_801B6870);
        }
        g_api.PlaySfx(0xA);
        self->step = 1;
        self->step_s = 0;
    }
}

// Applies `amount` of damage/shrink to the Primitive at D_us_801B6858
// selected by (D_us_801B684E + 1) % 5: decrements its `v1` field (used
// here as an opacity/HP-like counter) and bumps its `v0` field, and once
// `v1` reaches 0 marks it "done" (drawMode = 8) and advances
// g_CurrentEntity->step_s -- but only while g_CurrentEntity->step_s == 0.
// Then applies `amount` to every OTHER slot's y0 (a steering nudge), and
// finally increments+returns the shared counter D_us_801B6850.
s32 func_us_801A5CC8(s32 amount) {
    s32 index = (D_us_801B684E + 1) % 5;
    s32 i;

    if (g_CurrentEntity->step_s == 0) {
        Primitive* target = D_us_801B6858[index];
        target->v1 -= amount;
        target->v0 += amount;
        if (target->v1 == 0) {
            g_CurrentEntity->step_s++;
            target->drawMode = 8;
        }
    }

    amount &= 0xFF;
    for (i = 0; i < 5; i++) {
        if (i != index) {
            Primitive* other = D_us_801B6858[i];
            other->y0 -= amount;
        }
    }

    return ++D_us_801B6850;
}

// The boss-intro cutscene controller: picks and runs one of two static
// bytecode scripts (D_us_80181DC5/CCC, see BOSS_SCRIPT_CURSOR in bo2.h)
// through a small per-frame VM (step 1), then runs a handful of cleanup/
// animation phases (steps 2-7) once the script signals it's done (opcode 0
// sets self->step = 7 directly; other opcodes advance through 2-6 first).
//
// Every call (while self->step != 0) first checks whether the real ending
// condition has already been met -- either g_GameClearFlag, or the
// matching g_Settings+0x10C completion bit for self->params' ending (the
// same bits step 7 sets) -- and if so and D_us_801B683C isn't already set,
// calls the (already-decompiled) debug-cheat/reset helper
// func_us_801A5BE8(self). It also re-drives the OTHER (g_Entities_64)
// bytecode VM via func_us_801A593C once per call while D_us_801B687C is
// armed -- unrelated to this function's own BOSS_SCRIPT_CURSOR VM below.
//
// Opcode reference for step 1's VM (byte fetched via BOSS_SCRIPT_CURSOR):
//   0  = signal script end (self->step = 7)
//   1  = advance the shared text-prim animation (calls func_us_801A56D4)
//        through up to 5 frames per script "waypoint", then step_s=0 and
//        self->step++ once done
//   2  = set D_us_801B6857 from the next byte (no pause gate)
//   3  = set D_us_801B6856 (a wait-frame counter) from the next byte
//   4  = mark 5 chained prims (D_us_801B6858[0]->next chain) drawMode=8
//   5  = configure a two-tone sprite from 2 bytes (byte1 bit7 swaps the
//        two UV halves, byte0/byte1 index the palette-ish D_us_80180B7C/
//        80/84 tables), plays the matching string via func_us_801A578C,
//        and jumps to self->step = 3
//   6  = mark the same 5 chained prims visible=8, free D_us_801B6874's
//        chain, arm D_us_801B6850=6, self->step = 4
//   7  = shift 2 prims (from D_us_801B686C) into place from 2 bytes,
//        self->step = 5
//   8  = arm D_us_801B6850=0x18, self->step = 6
//   9  = same as opcode 20 while unpaused; otherwise just skips 2 bytes
//        like the other paused-opcodes below
//   10 = wait for g_api.func_80131F68() to return nonzero (rewinds and
//        returns while it doesn't)
//   11 = wait for g_api.func_80131F68() to return exactly 1
//   12 = read 4 nibble-packed bytes and call func_us_801A5910 with them
//   13 = no-op
//   14 = compute a jump target via a table lookup, then behaves like
//        opcode 15 from that computed address
//   15 = read 4 consecutive bytes, pack them into a nibble-shifted value,
//        and OVERWRITE BOSS_SCRIPT_CURSOR with (packed + 0x100000) --
//        i.e. an absolute jump encoded as 4 script bytes
//   16 = wait for bit (next byte) of D_us_801B68EC to be set, then clear it
//        (peeks the byte; rewinds+returns without consuming it if not set)
//   17 = set bit (next byte) of D_us_801B68EC
//   18 = clear D_us_801B687C
//   19 = LoadTPage from 5 bytes (tp=1, abr=0, x=D_us_80180B88[byte4],
//        y/w/h = 0x100/0x30/0x48) -- skips the 5 bytes instead if paused
//   20 = PlaySfx from 2 bytes ((byte0<<4)|byte1)
//   21 = reset D_us_801B68EC/683C/68F0 to 0
//   22 = clear bit (next byte) of D_us_801B68EC
//   23 = return immediately (pure terminator, no bytes consumed)
//   24 = same wait-for-bit-set pattern as opcode 16, but doesn't clear the
//        bit once set
//   >=25 (0x19) = "draw" pseudo-opcode: copies D_us_801B6857 into
//        D_us_801B6856, then either just advances D_us_801B684A by 2 (raw
//        opcode byte == 0x20) or MoveImage's a small tile from a position
//        encoded in the opcode byte's own nibbles before advancing
//
// Steps 2-7 (post-script phases): 2 waits on func_us_801A5CC8(2)'s prim-
// shrink counter; 3/4 slide the D_us_801B686C prim pair in/out and manage
// D_us_801B6850 as a frame counter, looping back to step 1 (3) or advancing
// (4); 5 builds/tears down a 72-tile HUD gauge (D_us_801B6870) across its
// own step_s 0/1/2 sub-states; 6 is a countdown "wipe" that eventually
// resets to step 1 or advances to 7; 7 sets the real completion flags
// (g_CastleFlags[0xB4]/0x85/0x62 and the matching g_Settings+0x10C bits for
// the true ending, or D_us_80180B78 + a restarted boss theme for the
// normal ending) and destroys self.
void func_us_801A5DCC(Entity* self) {
    FntPrint(D_us_801A1F84, D_us_801B6840);

    if (self->step != 0) {
        if (D_us_801B68F0 != 0 && D_us_801B683C == 0) {
            s32 done;

            if (g_GameClearFlag) {
                done = 1;
            } else if (self->params != 0) {
                done = (*(u32*) ((u8*) &g_Settings + 0x10C) & 0x4000) != 0;
            } else {
                done = (*(u32*) ((u8*) &g_Settings + 0x10C) & 0x10) != 0;
            }
            if (done) {
                func_us_801A5BE8(self);
            }
        }
    }

    if (self->step != 0 && D_us_801B687C != 0) {
        func_us_801A593C();
    }

    if (self->step >= 8) {
        return;
    }

    switch (self->step) {
        case 0: {
            u8* script;
            s32 ok;

            if (self->params == 0) {
                if (g_CastleFlags[0xB4] || g_PlayableCharacter || g_DemoMode) {
                    DestroyEntity(self);
                    return;
                }
                script = D_us_80181DC5;
            } else {
                if (g_CastleFlags[0xB4] || D_us_80180B78 || g_PlayableCharacter ||
                    g_DemoMode) {
                    DestroyEntity(self);
                    return;
                }
                script = D_us_80181CCC;
            }

            ok = func_us_801A54B4((Entity*) script) & 0xFF;
            if (!ok) {
                return;
            }

            self->flags |= 0x802000;
            D_us_801B68EC = 0;
            D_us_801B68F0 = 0;
            D_us_801B683C = 0;
            g_CutsceneHasControl = 1;
            self->step++;
            self->primIndex = D_us_801B6878;
            return;
        }

        case 1: {
            u8* scriptPtr;

            for (;;) {
                u8 opcode;

                if (D_us_801B6856 != 0 && D_us_801B683C == 0) {
                    D_us_801B6856--;
                    return;
                }

                opcode = *BOSS_SCRIPT_CURSOR++;

                if (opcode < 0x19) {
                    switch (opcode) {
                        case 0:
                            self->step = 7;
                            return;

                        case 1:
                            D_us_801B684A = D_us_801B684C;
                            if (!(D_us_801B6852 & 1)) {
                                D_us_801B6846 += 0xC;
                            }
                            D_us_801B684E++;
                            if ((s16) D_us_801B684E >= 5) {
                                D_us_801B684E = 0;
                            }
                            func_us_801A56D4();
                            if (!(D_us_801B6852 & 1)) {
                                if ((s16) D_us_801B684E < 4) {
                                    continue;
                                }
                                D_us_801B6852 |= 1;
                            }
                            D_us_801B6850 = 0;
                            self->step_s = 0;
                            self->step++;
                            return;

                        case 2:
                            D_us_801B6857 = *BOSS_SCRIPT_CURSOR++;
                            continue;

                        case 3:
                            D_us_801B6856 = *BOSS_SCRIPT_CURSOR++;
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            return;

                        case 4:
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            {
                                Primitive* p = D_us_801B6858[0];
                                s32 i;
                                for (i = 0; i < 5; i++) {
                                    p->drawMode = 8;
                                    p = p->next;
                                }
                            }
                            return;

                        case 5:
                            if (D_us_801B683C != 0) {
                                goto skip_2_bytes;
                            }
                            {
                                u8 byte0 = *BOSS_SCRIPT_CURSOR++;
                                u8 byte1 = *BOSS_SCRIPT_CURSOR++;
                                Primitive* prim = D_us_801B686C;
                                u8 idx = byte1 & 1;
                                u8 a0 = D_us_80180B7C[idx];
                                u8 a1 = D_us_80180B80[idx];
                                u8 u0, u1;

                                prim->tpage = 0x90;
                                prim->clut = D_us_80180B84[byte0];
                                if (byte1 & 0x80) {
                                    u0 = a0 + 0x2F;
                                    u1 = a0;
                                } else {
                                    u0 = a0;
                                    u1 = a0 + 0x2F;
                                }
                                prim->u0 = u0;
                                prim->u2 = u0;
                                prim->u1 = u1;
                                prim->u3 = u1;
                                prim->v0 = a1;
                                prim->v1 = a1;
                                prim->v2 = a1 + 0x48;
                                prim->v3 = a1 + 0x48;

                                prim->x3 = D_us_801B6844 - 0x1E;
                                prim->x2 = prim->x3;
                                prim->x1 = prim->x3;
                                prim->x0 = prim->x3;
                                prim->y3 = D_us_801B6848 + 0x24;
                                prim->y2 = prim->y3;
                                prim->y1 = prim->y3;
                                prim->y0 = prim->y3;

                                D_us_801B6854 = D_us_80180B8C[byte0];
                                func_us_801A5460();
                                func_us_801A56D4();
                                func_us_801A578C(byte0, self);
                                prim->priority = 0x1FE;
                                prim->drawMode = 0;
                                D_us_801B6850 = 6;
                                self->step = 3;
                                return;
                            }

                        case 6:
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            {
                                Primitive* p = D_us_801B6858[0];
                                s32 i;
                                for (i = 0; i < 5; i++) {
                                    p->drawMode = 8;
                                    p = p->next;
                                }
                            }
                            g_api.FreePrimitives(D_us_801B6874);
                            D_us_801B6874 = -1;
                            D_us_801B6850 = 6;
                            self->step = 4;
                            return;

                        case 7:
                            if (D_us_801B683C != 0) {
                                goto skip_2_bytes;
                            }
                            {
                                u8 byte0 = *BOSS_SCRIPT_CURSOR++;
                                u8 byte1 = *BOSS_SCRIPT_CURSOR++;
                                Primitive* p = D_us_801B686C;

                                D_us_801B6844 = byte0;
                                D_us_801B6848 = byte1;

                                p = p->next;
                                p->y1 = byte1;
                                p->y0 = byte1;
                                p->y3 = D_us_801B6848 + 0x48;
                                p->y2 = D_us_801B6848 + 0x48;

                                p = p->next;
                                p->y0 = D_us_801B6848 - 1;
                                p->u0 = 0xF6;

                                D_us_801B6848 = 0x18;
                                self->step = 5;
                                self->step_s = 0;
                                return;
                            }

                        case 8:
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            D_us_801B6850 = 0x18;
                            self->step = 6;
                            return;

                        case 9:
                            if (D_us_801B683C == 0) {
                                goto play_sfx;
                            }
                            goto skip_2_bytes;

                        case 10:
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            if (g_api.func_80131F68() == 0) {
                                goto rewind_and_return;
                            }
                            continue;

                        case 11:
                            if (D_us_801B683C != 0) {
                                continue;
                            }
                            if (g_api.func_80131F68() != 1) {
                                continue;
                            }
                            goto rewind_and_return;

                        case 12: {
                            u8 b0 = *BOSS_SCRIPT_CURSOR++;
                            u8 b1 = *BOSS_SCRIPT_CURSOR++;
                            u8 b2 = *BOSS_SCRIPT_CURSOR++;
                            u8 b3 = *BOSS_SCRIPT_CURSOR++;
                            u32 packed = (((((u32) b0 << 4) | b1) << 4) | b2) << 4 | b3;
                            func_us_801A5910((u8*) packed);
                            continue;
                        }

                        case 13:
                            continue;

                        case 14: {
                            u8 b0 = *BOSS_SCRIPT_CURSOR++;
                            u8 b1 = *BOSS_SCRIPT_CURSOR++;
                            u8 b2 = *BOSS_SCRIPT_CURSOR++;
                            u32 idx = (((u32) b0 << 4 | b1) << 4) | b2;
                            u16 tableVal = *(u16*) idx;
                            scriptPtr = (u8*) (tableVal * 4 + (u32) BOSS_SCRIPT_CURSOR);
                            goto read_packed_word;
                        }

                        case 15:
                            scriptPtr = BOSS_SCRIPT_CURSOR;
                            goto read_packed_word;

                        case 16: {
                            u8 bit = *BOSS_SCRIPT_CURSOR;
                            if (!((D_us_801B68EC >> bit) & 1)) {
                                goto rewind_and_return;
                            }
                            D_us_801B68EC &= ~(1u << bit);
                            BOSS_SCRIPT_CURSOR++;
                            continue;
                        }

                        case 17: {
                            u8 bit = *BOSS_SCRIPT_CURSOR++;
                            D_us_801B68EC |= 1u << bit;
                            continue;
                        }

                        case 18:
                            D_us_801B687C = 0;
                            continue;

                        case 19:
                            if (D_us_801B683C != 0) {
                                BOSS_SCRIPT_CURSOR += 5;
                                continue;
                            }
                            {
                                u8 b0 = *BOSS_SCRIPT_CURSOR++;
                                u8 b1 = *BOSS_SCRIPT_CURSOR++;
                                u8 b2 = *BOSS_SCRIPT_CURSOR++;
                                u8 b3 = *BOSS_SCRIPT_CURSOR++;
                                u8 b4 = *BOSS_SCRIPT_CURSOR++;
                                u32 packed = (((((u32) b0 << 4) | b1) << 4) | b2) << 4 | b3;
                                LoadTPage((u_long*) (packed + 0x100000), 1, 0,
                                          D_us_80180B88[b4], 0x100, 0x30, 0x48);
                            }
                            continue;

                        play_sfx:
                        case 20: {
                            u8 b0 = *BOSS_SCRIPT_CURSOR++;
                            u8 b1 = *BOSS_SCRIPT_CURSOR++;
                            g_api.PlaySfx(b1 | (b0 << 4));
                            continue;
                        }

                        case 21:
                            D_us_801B68EC = 0;
                            D_us_801B683C = 0;
                            D_us_801B68F0 = 0;
                            continue;

                        case 22: {
                            u8 bit = *BOSS_SCRIPT_CURSOR++;
                            D_us_801B68EC &= ~(1u << bit);
                            continue;
                        }

                        case 23:
                            return;

                        case 24: {
                            u8 bit = *BOSS_SCRIPT_CURSOR;
                            if (!((D_us_801B68EC >> bit) & 1)) {
                                goto rewind_and_return;
                            }
                            BOSS_SCRIPT_CURSOR++;
                            continue;
                        }
                    }
                    continue;

                read_packed_word: {
                    u8 c0 = scriptPtr[0];
                    u8 c1 = scriptPtr[1];
                    u8 c2 = scriptPtr[2];
                    u32 packed;
                    BOSS_SCRIPT_CURSOR = scriptPtr + 3;
                    packed = (((((u32) c0 << 4) | c1) << 4) | c2) << 4 | *BOSS_SCRIPT_CURSOR;
                    D_us_801B6840 = (s32) (packed + 0x100000);
                    continue;
                }
                } else {
                    if (D_us_801B683C != 0) {
                        continue;
                    }
                    D_us_801B6856 = D_us_801B6857;
                    if (opcode == 0x20) {
                        D_us_801B684A += 2;
                        return;
                    }
                    {
                        RECT rect;
                        rect.x = ((opcode & 0xF) << 1) + 0x380;
                        rect.y = ((opcode & 0xF0) >> 1) + 0xF0;
                        rect.w = 2;
                        rect.h = 8;
                        MoveImage(&rect, D_us_801B684A, D_us_801B684E * 12 + 0x180);
                        D_us_801B684A += 2;
                        return;
                    }
                }

            skip_2_bytes:
                BOSS_SCRIPT_CURSOR += 2;
                continue;

            rewind_and_return:
                BOSS_SCRIPT_CURSOR--;
                return;
            }
        }

        case 2:
            func_us_801A5CC8(2);
            if (D_us_801B6850 < 6) {
                return;
            }
            self->step--;
            return;

        case 3: {
            Primitive* p = D_us_801B686C;
            Primitive* p2;

            p->x2 -= 4;
            p->x0 = p->x2;
            p->x3 += 4;
            p->x1 = p->x3;
            p->y1 -= 6;
            p->y0 = p->y1;
            p->y3 += 6;
            p->y2 = p->y3;

            D_us_801B6850--;
            if ((s16) D_us_801B6850 != 0) {
                return;
            }
            self->step = 1;

            for (p2 = &g_PrimBuf[D_us_801B6874]; p2 != NULL; p2 = p2->next) {
                p2->drawMode = 0;
            }
            return;
        }

        case 4: {
            Primitive* p = D_us_801B686C;

            p->x2 += 4;
            p->x0 = p->x2;
            p->x3 -= 4;
            p->x1 = p->x3;
            p->y1 += 6;
            p->y0 = p->y1;
            p->y3 -= 6;
            p->y2 = p->y3;

            {
                s16 base = D_us_801B6844;
                if (p->x1 >= base - 2) {
                    p->x3 = base - 3;
                    p->x1 = base - 3;
                }
            }

            D_us_801B6850--;
            if ((s16) D_us_801B6850 != 0) {
                return;
            }
            self->step = 1;
            return;
        }

        case 5:
            switch (self->step_s) {
                case 0: {
                    s32 primIndex = (s16) g_api.AllocPrimitives(PRIM_TILE, 0x48);
                    Primitive* p;
                    s32 i;

                    D_us_801B68F0 = 1;
                    if (primIndex == -1) {
                        DestroyEntity(self);
                        return;
                    }

                    D_us_801B6870 = primIndex;
                    p = &g_PrimBuf[primIndex];
                    for (i = 0; p != NULL; i++) {
                        p->r1 = 0x7F;
                        p->r0 = 0x7F;
                        p->g1 = 0;
                        p->g0 = 0;
                        p->b1 = 0;
                        p->b0 = 0;
                        p->x1 = 0xF7;
                        p->x0 = 0xF7;
                        p->x3 = 0xF70;
                        p->y1 = D_us_801B6848 + i;
                        p->y0 = D_us_801B6848 + i;
                        p->priority = 0x1FE;
                        p->drawMode = 0;
                        p->x2 = D_us_80180B90[i];
                        p = p->next;
                    }
                    self->step_s++;
                    return;
                }

                case 1: {
                    Primitive* p = &g_PrimBuf[D_us_801B6870];
                    s32 flag = 0;

                    if (p != NULL) {
                        do {
                            s16 x3 = p->x3 - p->x2;
                            s16 v1;
                            s16 x0;

                            p->x3 = x3;
                            p->x2 = p->x2 + 2;

                            v1 = x3;
                            if (v1 < 0) {
                                v1 += 0xF;
                            }
                            x0 = v1 >> 4;
                            p->x0 = x0;
                            if (x0 < 5) {
                                p->x0 = 4;
                            } else {
                                flag = 1;
                            }

                            p = p->next;
                        } while (p != NULL);
                    }

                    if (flag != 0) {
                        return;
                    }

                    g_api.FreePrimitives(D_us_801B6870);
                    D_us_801B6870 = -1;
                    {
                        Primitive* p2 = D_us_801B686C;
                        p2 = p2->next;
                        p2->drawMode = 0x11;
                        p2 = p2->next;
                        p2->drawMode = 0x51;
                    }
                    self->step_s++;
                    return;
                }

                case 2: {
                    Primitive* p = D_us_801B686C;
                    s32 flag = 0;
                    u8 v;

                    p = p->next;
                    if (p->r0 < 0x10) {
                        p->r3 = 0;
                        p->r2 = 0;
                        p->r1 = 0;
                        p->r0 = 0;
                    } else {
                        v = p->r3 - 0x10;
                        p->r3 = v;
                        p->r2 = v;
                        p->r1 = v;
                        p->r0 = v;
                        flag = 1;
                    }

                    if (p->b0 < 0x7F) {
                        v = 0x7F;
                    } else {
                        v = p->b1 + 8;
                        flag = 1;
                    }
                    p->b1 = v;
                    p->b0 = v;

                    p = p->next;
                    if (p->r0 < 0x10) {
                        v = 0x10;
                    } else {
                        v = p->b0 - 0xF;
                        flag = 1;
                    }
                    p->b0 = v;
                    p->g0 = v;
                    p->r0 = v;

                    if (flag != 0) {
                        return;
                    }
                    self->step = 1;
                    return;
                }
            }
            return;

        case 6: {
            Primitive* p = D_us_801B686C;
            s16 v;

            p = p->next;
            D_us_801B6850--;
            v = (s16) D_us_801B6850;

            if (v >= 0xC) {
                Primitive* p2 = p->next;
                u8 oldU0 = p2->u0;
                p2->u0 = oldU0 + 0xEC;
                if (D_us_801B6850 & 1) {
                    p2->u0 = oldU0 - 0x15;
                }
                return;
            }

            if (v == 0) {
                self->step = 1;
                p->drawMode = 8;
            } else {
                p->y3 -= 6;
                p->y2 = p->y3;
            }

            p = p->next;
            p->drawMode = 8;
            return;
        }

        case 7:
            if (self->params != 0) {
                u32* flags = (u32*) ((u8*) &g_Settings + 0x10C);
                u32 orig = *flags;

                g_CastleFlags[0xB4] = 1;
                g_CastleFlags[0x85] = 1;
                g_CastleFlags[0x62] = 1;
                *flags = orig | 0x4000;
                *flags = orig | 0x4100;
                *flags = orig | 0x4300;
            } else {
                u32* flags = (u32*) ((u8*) &g_Settings + 0x10C);
                u32 v;

                D_us_80180B78 = 1;
                v = *flags;
                stopMusicFlag = 0;
                currentMusicId = 0x31D;
                v |= 0x10;
                *flags = v;
            }
            DestroyEntity(self);
            g_CutsceneHasControl = 0;
            return;
    }
}

// The boss-defeat cutscene controller: freezes normal player input
// (g_Player.padSim/demo_timer) and walks the player toward a scripted
// position as the camera/HUD entity (g_Entities[0]) mirrors an animation,
// skipping the mirrored-animation copy when g_Status.wornEquipment[1] ==
// ITEM_AXE_LORD_ARMOR (true-ending path). Advances its own step machine as
// each phase completes and finally destroys itself.
void func_us_801A6EF8(Entity* self) {
    s16 worldX = PLAYER_posX_i_hi + g_Tilemap.scrollX.i.hi;

    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801804DC);
            self->animSet = 0;
            break;
        case 1: {
            PlayerStateStatus status;

            if (worldX < 0x61 && self->params == 0) {
                break;
            }
            if (self->params == 0) {
                D_us_80180B74 = 1;
                g_Entities[0].posX.i.hi = 0x60;
            }
            status = g_Player.status;
            g_PauseAllowed = 0;
            g_unkGraphicsStruct.g_zEntityCenter = 1;
            if (status & 1) {
                g_Player.padSim = 8;
            } else if (status & 2) {
                g_Player.padSim = 4;
            } else if (status & 4) {
                g_Player.padSim = 2;
            } else {
                g_Player.padSim = 0;
            }
            g_Player.demo_timer = 1;
            if (self->params != 0) {
                self->step = 6;
                break;
            }
            D_us_801B68EC |= 1;
            self->step++;
            break;
        }
        case 2:
            if (g_Player.status & 4) {
                g_Player.padSim = 0;
                if (g_Timer & 1) {
                    g_Player.padSim = 2;
                }
            } else if (D_us_801B68EC & 0x80) {
                g_Player.padSim = 0x2000;
                self->step++;
            }
            g_Player.demo_timer = 1;
            break;
        case 3:
            if (worldX < 0xE8) {
                g_Player.padSim = 0x2000;
            } else {
                g_Player.padSim = 0;
                g_Entities[0].posX.i.hi = 0xE8 - g_Tilemap.scrollX.i.hi;
                if (g_Status.wornEquipment[1] != ITEM_AXE_LORD_ARMOR) {
                    AnimateEntity(D_us_80180C8C, self);
                    g_Entities[0].animCurFrame = self->animCurFrame;
                }
                self->step = 4;
            }
            g_Player.demo_timer = 1;
            break;
        case 4:
            if (g_Status.wornEquipment[1] != ITEM_AXE_LORD_ARMOR) {
                AnimateEntity(D_us_80180C8C, self);
                g_Entities[0].animCurFrame = self->animCurFrame;
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            if (D_us_801B68EC & 0x40) {
                SetStep(5);
            }
            break;
        case 5:
            if (!AnimateEntity(D_us_80180C98, self)) {
                g_PauseAllowed = 1;
                g_unkGraphicsStruct.g_zEntityCenter = 0;
                D_us_80180AAC = 1;
                DestroyEntity(self);
                break;
            }
            if (g_Status.wornEquipment[1] != ITEM_AXE_LORD_ARMOR) {
                g_Entities[0].animCurFrame = self->animCurFrame;
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            break;
        case 6:
            if ((g_Player.status & 4) && (g_Timer & 1)) {
                g_Player.padSim = 2;
            }
            if (g_Player.vram_flag & 1) {
                D_us_801B68EC |= 1;
                self->step++;
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            break;
        case 7:
            if ((g_Player.status & 4) && (g_Timer & 1)) {
                g_Player.padSim = 2;
            }
            if (D_us_801B68EC & 0x1000) {
                g_PauseAllowed = 1;
                g_unkGraphicsStruct.g_zEntityCenter = 0;
                DestroyEntity(self);
                break;
            }
            g_Player.padSim = 0;
            g_Player.demo_timer = 1;
            break;
        default:
            break;
    }
}

// A staged visual effect entity (fade in, animate through several frame
// sets gated by bits of the shared D_us_801B68EC bitmask, then fade out via
// opacity and self-destruct). If D_us_801B683C is set while in steps 3-5,
// forced straight to step 6 (early abort into the fade-out).
void func_us_801A7340(Entity* self) {
    if (D_us_801B683C && (u32) (self->step - 3) < 3) {
        SetStep(6);
    }

    switch (self->step) {
        case 0:
            InitializeEntity(g_EInitCommon);
            self->animSet = -0x7FFB;
            self->animCurFrame = 1;
            self->unk5A = 0x5A;
            self->palette = 0x22A;
            self->drawFlags = 8;
            self->opacity = 0;
            break;
        case 1:
            if (D_us_801B68EC & 0x80) {
                SetStep(2);
            }
            break;
        case 2:
            if (self->opacity < 0x80) {
                self->opacity++;
            }
            AnimateEntity(D_us_80180C28, self);
            if (D_us_801B68EC & 4) {
                self->animCurFrame = 1;
                SetStep(3);
            }
            break;
        case 3:
            if (D_us_801B68EC & 8) {
                SetStep(4);
            }
            break;
        case 4:
            AnimateEntity(D_us_80180C30, self);
            if (D_us_801B68EC & 0x20) {
                SetStep(5);
            }
            break;
        case 5:
            AnimateEntity(D_us_80180C50, self);
            if (D_us_801B68EC & 0x100) {
                self->animCurFrame = 1;
                SetStep(6);
            }
            break;
        case 6:
            AnimateEntity(D_us_80180C28, self);
            if (self->opacity < 9) {
                self->step = 7;
                self->palette = 0x15F;
                self->blendMode = 0x50;
                self->opacity = 0x60;
            } else {
                self->opacity--;
            }
            break;
        case 7:
            AnimateEntity(D_us_80180C28, self);
            if (self->opacity == 0) {
                DestroyEntity(self);
            } else {
                self->opacity -= 2;
            }
            break;
        default:
            break;
    }
}

// step 0: initializes only. step 1: on a specific button tap
// (g_pads_1_tapped & 0x10), pokes two still-unresolved helper calls
// (func_us_801A9CE8, see create_entity.c task) with fixed IDs/addresses
// into what looks like core/main game state rather than bo2-private data,
// then advances step.
void func_us_801A75B4(Entity* self) {
    switch (self->step) {
        case 0:
            InitializeEntity(D_us_801804DC);
            break;
        case 1:
            if (g_pads_1_tapped & 0x10) {
                func_us_801A9CE8(0x25, &D_8007C6B8);
                D_8007C6E8 = 1;
                func_us_801A9CE8(0x26, &D_8007C6B8 - 0x4C60);
                D_80077A88 = 1;
                self->step++;
            }
            break;
        default:
            break;
    }
}
