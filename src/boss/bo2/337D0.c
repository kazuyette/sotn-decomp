#include "bo2.h"

// Bounces g_CurrentEntity's posX.i.hi back within the room's horizontal
// bounds, mirroring func_us_801A2610 but with an 8px margin and different
// clamp values (0x1A8/0x58 instead of 0x10px margin/0x1A0/0x60).
void func_us_801B37D0(void) {
    Entity* entity = g_CurrentEntity;
    s16 scrollX = g_Tilemap.scrollX.i.hi;
    s32 worldX = entity->posX.i.hi + scrollX;
    s32 velocityX = entity->velocityX;

    if (velocityX > 0) {
        worldX += 8;
        if (worldX >= 0x1B1) {
            entity->posX.i.hi = 0x1A8 - scrollX;
            entity = g_CurrentEntity;
        }
        velocityX = entity->velocityX;
    }

    if (velocityX < 0) {
        worldX -= 8;
        if (worldX < 0x50) {
            entity->posX.i.hi = 0x58 - (u16) g_Tilemap.scrollX.i.hi;
        }
    }
}

// The second boss "conductor" (E_UNK_1B), mirror image of func_us_801A269C
// in 22610.c: same architecture (a large switch on self->step, several
// steps with their own step_s/jump-table sub-machines, a debug menu at
// step 0xFF, a shared epilogue that looks up a per-pose hitbox record and
// calls this overlay half's own room-bounds bouncer, func_us_801B37D0).
// The established "self - 3" idiom from func_us_801A269C (reading a second
// copy of the conductor) appears here as its exact mirror, "self + 3"
// (self->0x234/0x236/0x248/0x268/0x2C3 are all (self+3)'s own fields at
// offsets 0/2/0x14/0x34/0x8F) -- confirming the two conductors reference
// each other symmetrically.
void func_us_801B385C(Entity* self) {
    FntPrint(D_us_801A219C, self->step);
    FntPrint(D_us_801A21AC, ENTITY_UNK88(self));

    if ((self->flags & 0x100) && self->step < 0x20) {
        SetStep(0x20);
    }

    switch (self->step) {
        case 0: {
            InitializeEntity(D_us_80180560);
            self->hitboxState = 0;
            func_us_801A9CE8(0x1C, self + 1);
        }
        // fallthrough
        case 1: {
            s32 side;

            AnimateEntity(D_us_80181868, self);
            side = GetSideToPlayer();
            self->facingLeft = !(side & 1);
            UnkCollisionFunc3(D_us_80181858);

            switch (self->step_s) {
                case 0:
                    self->palette = 0x815F;
                    self->blendMode = 0x50;
                    self->opacity = 0;
                    self->drawFlags |= 8;
                    self->step_s++;
                    // fallthrough
                case 1:
                    self->opacity += 4;
                    if (self->opacity == 0x80) {
                        self->blendMode = 0;
                        self->opacity = 0;
                        self->palette = D_us_80180566;
                        self->step_s++;
                    }
                    break;

                case 2:
                    self->opacity += 4;
                    if (self->opacity == 0x80) {
                        self->drawFlags = 0;
                        // Raw table address stashed into a flag field --
                        // same "s32 holds a pointer" idiom used elsewhere
                        // in this overlay (see bo2.h's BOSS_SCRIPT_CURSOR).
                        D_us_80180AB0 = (s32) D_us_80181858;
                        self->step_s++;
                    }
                    break;

                case 3:
                    if (D_us_80180AAC != 0) {
                        self->hitboxState = 3;
                        SetStep(2);
                    }
                    break;

                default:
                    break;
            }
            break;
        }

        case 2: {
            if (self->step_s == 0) {
                if (ENTITY_UNK82(self) == 0) {
                    ENTITY_UNK82(self) = 0x20;
                }
                ENTITY_UNK8D(self) = 0;
                self->step_s++;
            }

            AnimateEntity(D_us_80181868, self);
            {
                s32 side = GetSideToPlayer();
                self->facingLeft = !(side & 1);
            }

            if (ENTITY_UNK82(self) != 0) {
                ENTITY_UNK82(self)--;
            } else {
                ENTITY_UNK8D(self) = 1;
            }

            if (ENTITY_UNK84(self) != 0) {
                ENTITY_UNK8D(self) = 0;
                SetStep(4);
                break;
            }

            if (ENTITY_UNK8E(self) != 0) {
                SetStep(ENTITY_UNK88(self));
                ENTITY_UNK8D(self) = 0;
                ENTITY_UNK8E(self) = 0;
                break;
            }

            {
                s32 relVelocity = self->facingLeft ? -PLAYER_velocityX : PLAYER_velocityX;

                if (self->facingLeft == PLAYER_facingLeft &&
                    GetDistanceToPlayerX() < 0x38 && relVelocity > 0) {
                    ENTITY_UNK8D(self) = 0;
                    SetStep(6);
                }
            }
            break;
        }

        case 4: {
            s32 s0;

            if (self->step_s == 0) {
                s32 playerX = PLAYER_posX_i_hi;
                s32 selfX = self->posX.i.hi;

                s0 = ENTITY_UNK84(self);
                if (playerX < selfX) {
                    s0 += playerX + g_Tilemap.scrollX.i.hi;
                    if (s0 >= 0x191) {
                        s0 = 0x190;
                    }
                } else {
                    s0 = playerX - s0 + g_Tilemap.scrollX.i.hi;
                    if (s0 < 0x70) {
                        s0 = 0x70;
                    }
                }

                ENTITY_UNK84(self) = s0;
                self->velocityX = (ENTITY_UNK84(self) - (self->posX.i.hi + g_Tilemap.scrollX.i.hi)) > 0 ? 0x12000 : -0x12000;
                self->step_s++;
            }

            {
                s32 side = GetSideToPlayer();
                self->facingLeft = !(side & 1);
            }
            AnimateEntity(D_us_80181874, self);
            UnkCollisionFunc2(D_us_80181850);

            {
                s32 worldX = self->posX.i.hi + g_Tilemap.scrollX.i.hi;
                s32 target = ENTITY_UNK84(self);
                s32 done;

                if (self->velocityX > 0) {
                    s0 = target - worldX;
                    done = (s0 > 0);
                } else {
                    done = (s0 >= 0);
                }

                if (done) {
                    ENTITY_UNK84(self) = 0;
                    SetStep(2);
                }
            }
            break;
        }

        case 6:
            if (AnimateEntity(D_us_80181884, self) == 0) {
                SetStep(2);
            }
            if (self->pose == 6) {
                PlaySfxPositional(0x6C7);
            }
            break;

        case 8: {
            switch (self->step_s) {
                case 0: {
                    Entity* other = self + 3;
                    s32 diff = other->posX.i.hi - self->posX.i.hi;

                    if (self->facingLeft == other->facingLeft) {
                        if (diff < 0) diff = -diff;
                        ENTITY_UNK8F(self) = (diff < 0x20) ? 1 : 0;
                    } else {
                        ENTITY_UNK8F(self) = 0;
                    }
                    ENTITY_UNK8F(other) = !ENTITY_UNK8F(self);
                    self->step_s++;
                }
                // fallthrough
                case 1:
                    if (AnimateEntity(D_us_801818A4, self) == 0) {
                        self->pose = 0x2F;
                        self->velocityX = self->facingLeft ? 0x20000 : -0x20000;
                        self->velocityY = -0x20000;
                        SetSubStep(3);
                    }
                    break;

                case 3:
                    if (UnkCollisionFunc3(D_us_80181858) & 1) {
                        self->step_s++;
                    }
                    break;

                case 4: {
                    if (AnimateEntity(D_us_801818AC, self) == 0) {
                        SetStep(4);
                    }
                    if (self->pose == 4) {
                        Entity* e = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1D, self, e);
                            e->facingLeft = self->facingLeft;
                            *(Entity**) ((u8*) e + 0x7C) = self + 3;
                            e->posY.i.hi -= 0x10;
                            ENTITY_UNK8F(e) = ENTITY_UNK8F(self);
                        }
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 0xA: {
            if (self->step_s == 0) {
                s32 side;

                self->velocityX = 0;
                side = GetSideToPlayer();
                self->velocityY = 0;
                self->facingLeft = !(side & 1);
                self->step_s++;
            }

            UnkCollisionFunc2(D_us_80181850);
            if (AnimateEntity(D_us_801818BC, self) == 0) {
                SetStep(2);
            }

            self->velocityX -= self->velocityX / 16;

            if (self->pose == 3) {
                PlaySfxPositional(0x616);
                self->velocityX = self->facingLeft ? 0x40000 : -0x40000;
            }
            break;
        }

        case 0xC: {
            if (self->step_s == 0) {
                if (AnimateEntity(D_us_801818D0, self) == 0) {
                    ENTITY_UNK80(self) = 0x80;
                    self->step_s++;
                }
                break;
            }

            AnimateEntity(D_us_801818D8, self);
            {
                u16 counter = ENTITY_UNK80(self) - 1;
                ENTITY_UNK80(self) = counter;
                if ((s16) counter == 0) {
                    SetStep(0xE);
                }
            }
            break;
        }

        case 0xE: {
            self->blendMode = ((self->flags & 0xF) == 0) ? ((g_Timer & 1) ? 0x30 : 0x70) : 0;

            switch (self->step_s) {
                case 0:
                    self->velocityX = 0;
                    self->velocityY = 0;
                    ENTITY_UNK8F(self) = 0;
                    self->step_s++;
                    break;

                case 1:
                    if (AnimateEntity(D_us_801818E0, self) == 0) {
                        SetSubStep(3);
                    }
                    if (self->pose == 1) {
                        self->velocityX = self->facingLeft ? 0x60000 : -0x60000;
                        ENTITY_UNK80(self) = 0x30;
                    }
                    break;

                case 2: {
                    u16 counter;

                    UnkCollisionFunc2(D_us_80181850);
                    self->velocityX -= self->velocityX / 64;
                    counter = ENTITY_UNK80(self) - 1;
                    self->pose = 0x2B;
                    ENTITY_UNK80(self) = counter;
                    if ((s16) counter == 0) {
                        SetSubStep(4);
                    }
                    break;
                }

                case 3:
                    UnkCollisionFunc2(D_us_80181850);
                    self->velocityX -= self->velocityX / 16;
                    if (AnimateEntity(D_us_801818EC, self) == 0) {
                        self->blendMode = 0;
                        SetStep(4);
                    }
                    break;

                default:
                    break;
            }
            break;
        }

        case 0x12: {
            switch (self->step_s) {
                case 0:
                    self->velocityX = self->facingLeft ? 0x38000 : -0x38000;
                    ENTITY_UNK80(self) = 0x20;
                    self->pose = 0x17;
                    self->velocityY = 0;
                    self->drawFlags |= 4;
                    self->step_s++;
                    // fallthrough
                case 1: {
                    UnkCollisionFunc2(D_us_80181850);
                    if (!(g_Timer & 8)) {
                        self->rotate -= 0x100;
                        PlaySfxPositional(0x785);
                    }
                    if ((ENTITY_UNK80(self) & 3) == 0) {
                        Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1E, self, e);
                            e->facingLeft = self->facingLeft;
                            e->rotate = self->rotate;
                            e->params = self->pose;
                        }
                    }
                    {
                        u16 counter = ENTITY_UNK80(self) - 1;
                        ENTITY_UNK80(self) = counter;
                        if ((s16) counter == 0) {
                            self->step_s = 2;
                        }
                    }
                    break;
                }

                case 2:
                    self->drawFlags = 0;
                    SetStep(4);
                    break;

                default:
                    break;
            }
            break;
        }

        case 0x14: {
            self->blendMode = ((self->flags & 0xF) == 0) ? ((g_Timer & 1) ? 0x30 : 0x70) : 0;

            switch (self->step_s) {
                case 0: {
                    s32 side = GetSideToPlayer();

                    self->facingLeft = !(side & 1);
                    ENTITY_UNK8F(self) = 0;
                    self->step_s++;
                    if (AnimateEntity(D_us_80181930, self) == 0) {
                        self->step_s++;
                    }
                    break;
                }

                case 1: {
                    self->velocityY = -0x40000;
                    self->velocityX = 0;
                    self->drawFlags = 4;
                    self->step_s++;
                    MoveEntity();
                    if (!(g_Timer & 7)) {
                        self->rotate -= 0x100;
                        PlaySfxPositional(0x785);
                    }
                    self->pose = (g_Timer & 1) ? 0x18 : 0x19;
                    if ((ENTITY_UNK80(self) & 3) == 0) {
                        Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1E, self, e);
                            e->facingLeft = self->facingLeft;
                            e->rotate = self->rotate;
                            e->params = self->pose;
                        }
                    }
                    if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x28) {
                        self->step_s++;
                    }
                    break;
                }

                case 2:
                    self->velocityX = self->facingLeft ? 0x28000 : -0x28000;
                    self->velocityY = 0;
                    self->step_s++;
                    MoveEntity();
                    // fallthrough
                case 3: {
                    self->velocityX = self->facingLeft ? 0x28000 : -0x28000;
                    if (!(g_Timer & 7)) {
                        self->rotate -= 0x100;
                        PlaySfxPositional(0x785);
                    }
                    self->pose = (g_Timer & 1) ? 0x18 : 0x19;
                    if (ENTITY_UNK8F(self) != 0) {
                        self->pose = 0xA;
                        self->drawFlags = 0;
                        self->velocityX = 0;
                        self->velocityY = 0x20000;
                        self->step_s++;
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 0x16: {
            switch (self->step_s) {
                case 0: {
                    Entity* other = self + 3;

                    ENTITY_UNK8F(self) = 0;
                    self->facingLeft = (other->posX.i.hi < self->posX.i.hi) ? 0 : 1;
                    self->velocityX = self->facingLeft ? 0x38000 : -0x38000;
                    self->velocityY = 0;

                    if (other->flags & 0x100) {
                        SetStep(0xA);
                        return;
                    }

                    self->step_s++;
                    if (AnimateEntity(D_us_80181930, self) == 0) {
                        self->pose = 0x17;
                        self->drawFlags |= 4;
                        self->step_s++;
                    }
                    break;
                }

                case 1: {
                    UnkCollisionFunc2(D_us_80181850);
                    if (!(g_Timer & 7)) {
                        self->rotate -= 0x100;
                        PlaySfxPositional(0x785);
                    }
                    if ((ENTITY_UNK80(self) & 3) == 0) {
                        Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1E, self, e);
                            e->facingLeft = self->facingLeft;
                            e->rotate = self->rotate;
                            e->params = self->pose;
                        }
                    }
                    {
                        Entity* other = self + 3;
                        s32 diff = 0; // stays uninitialized-equivalent when facingLeft==0, matching the raw asm

                        if (self->facingLeft != 0) {
                            diff = other->posX.i.hi - self->posX.i.hi;
                        }
                        if (diff < 0x20) {
                            ENTITY_UNK8F(other) = 1;
                            self->rotate = 0;
                            self->step_s++;
                        }
                        if (other->flags & 0x100) {
                            SetSubStep(7);
                        }
                    }
                    break;
                }

                case 2: {
                    s32 raw = (s32) g_Entities - *(s32*) &self->posX;

                    self->velocityY = -0x88000;
                    self->velocityX = raw / 64;
                    if (self->velocityX > 0) {
                        self->pose = 8;
                        self->facingLeft = 1;
                    } else {
                        self->facingLeft = 0;
                    }
                    self->step_s++;
                    break;
                }

                case 3:
                    MoveEntity();
                    self->velocityY += 0x4000;
                    if ((self->posY.i.hi + g_Tilemap.scrollY.i.hi) < 0x2C) {
                        self->posY.i.hi = 0x2C - g_Tilemap.scrollY.i.hi;
                        ENTITY_UNK80(self) = 8;
                        self->step_s++;
                    }
                    break;

                case 4: {
                    u16 counter = ENTITY_UNK80(self) - 1;

                    self->pose = 0x17;
                    self->rotate -= 0x180;
                    ENTITY_UNK80(self) = counter;
                    if ((s16) counter == 0) {
                        ENTITY_UNK80(self) = 0xC;
                        self->step_s++;
                    }
                    break;
                }

                case 5: {
                    u16 counter = ENTITY_UNK80(self) - 1;

                    self->pose = 0x1B;
                    self->rotate = 0;
                    ENTITY_UNK80(self) = counter;
                    if ((s16) counter == 0) {
                        s16 angle = (s16) GetAngleBetweenEntities(self, g_Entities);

                        self->velocityX = rcos(angle) * 80;
                        self->velocityY = rsin(angle) * 80;
                        ENTITY_UNK9C(self) = self->attack;
                        self->attack *= 2;
                        self->step_s++;
                    }
                    break;
                }

                case 6:
                    self->pose = 0x17;
                    if (!(g_Timer & 7)) {
                        self->rotate -= 0x180;
                        PlaySfxPositional(0x785);
                    }
                    if (UnkCollisionFunc3(D_us_80181858) & 1) {
                        PlaySfxPositional(0x647);
                        self->pose = 0x10;
                        self->attack = ENTITY_UNK9C(self);
                        self->step_s++;
                    }
                    if ((ENTITY_UNK80(self) & 3) == 0) {
                        Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1E, self, e);
                            e->facingLeft = self->facingLeft;
                            e->rotate = self->rotate;
                            e->params = self->pose;
                        }
                    }
                    break;

                case 7: {
                    u16 counter = ENTITY_UNK80(self) - 1;

                    self->drawFlags = 0;
                    self->pose = 0xC;
                    ENTITY_UNK80(self) = counter;
                    if ((s16) counter == 0) {
                        SetStep(4);
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        case 0x18: {
            switch (self->step_s) {
                case 0: {
                    s32 side = GetSideToPlayer();

                    ENTITY_UNK8F(self) = 0;
                    self->facingLeft = !(side & 1);
                    self->step_s++;
                    break;
                }

                case 1:
                    if (AnimateEntity(D_us_80181930, self) == 0) {
                        self->pose = 0x17;
                        self->drawFlags |= 4;
                        self->velocityX = self->facingLeft ? 0x38000 : -0x38000;
                        ENTITY_UNK80(self) = 0x40;
                        self->step_s++;
                    }
                    break;

                case 2: {
                    u16 counter;
                    Entity* other = self + 3;

                    UnkCollisionFunc2(D_us_80181850);
                    if (!(g_Timer & 7)) {
                        self->rotate -= 0x100;
                        PlaySfxPositional(0x785);
                    }
                    if ((ENTITY_UNK80(self) & 3) == 0) {
                        Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                        if (e != NULL) {
                            func_us_801A9D5C(0x1E, self, e);
                            e->facingLeft = self->facingLeft;
                            e->rotate = self->rotate;
                            e->params = self->pose;
                        }
                    }
                    if (!(other->flags & 0x100)) {
                        Entity* e = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                        if (e != NULL) {
                            s16 angle;

                            func_us_801A9D5C(0x1A, self, e);
                            e->rotate = self->rotate;
                            angle = self->facingLeft ? -self->rotate : self->rotate;
                            *(s32*) &e->posX += rsin(angle) * 192;
                            *(s32*) &e->posY -= rcos(angle) * 192;
                            e->facingLeft = self->facingLeft;
                            e->zPriority = self->zPriority + 1;
                        }
                    }
                    counter = ENTITY_UNK80(self) - 1;
                    ENTITY_UNK80(self) = counter;
                    if ((s16) counter == 0) {
                        self->step_s++;
                    }
                    break;
                }

                case 3:
                    self->drawFlags = 0;
                    SetStep(4);
                    break;

                default:
                    break;
            }
            break;
        }

        case 0x20: {
            ENTITY_UNK8D(self) = 1;
            ENTITY_UNK8F(self + 3) = 1;

            switch (self->step_s) {
                case 0: {
                    s32 result;

                    self->hitboxState = 0;
                    self->rotate = 0;
                    self->velocityX = 0;
                    self->velocityY = 0;
                    self->step_s++;

                    result = UnkCollisionFunc3(D_us_80181858);
                    if (result & 1) {
                        self->step_s++;
                    }

                    if (AnimateEntity(D_us_801818FC, self) == 0) {
                        PlaySfxPositional(0x654);
                        {
                            Entity* e = AllocEntity(g_Entities_224, (Entity*) ((u8*) g_Entities_224 + 0x1780));
                            if (e != NULL) {
                                func_us_801A9D5C(0x20, self, e);
                                e->posY.i.hi += 0x1B;
                                e->zPriority = self->zPriority;
                            }
                        }
                        SetSubStep(3);
                    }
                    break;
                }

                case 1:
                    if (AnimateEntity(D_us_80181908, self) == 0) {
                        SetSubStep(4);
                    }
                    break;

                case 2:
                    ENTITY_UNK84(self) = 0;
                    self->pose = 0;
                    break;

                default:
                    break;
            }
            break;
        }

        case 0xFF:
            FntPrint(D_us_801A21C0, self->pose);

            if (g_pads_1_pressed & 0x80) {
                if (self->params == 0) {
                    self->pose++;
                    self->params = 1;
                }
            } else {
                self->params = 0;
            }

            if (g_pads_1_pressed & 0x20) {
                if (self->step_s == 0) {
                    self->pose--;
                    self->step_s = 1;
                }
            } else {
                self->step_s = 0;
            }
            break;

        default:
            break;
    }

    {
        u8 frameIndex = D_us_801819F8[self->pose];
        u8* rec = &D_us_80181958[frameIndex * 4];

        self->hitboxOffX = (s8) rec[0];
        self->hitboxOffY = (s8) rec[1];
        self->hitboxWidth = rec[2];
        self->hitboxHeight = rec[3];
    }
    func_us_801B37D0();
}

// A hitbox-tracker linked to the entity right before it (mirrors
// func_us_801A3818, same idiom): copies position/facing and an
// animCurFrame-derived hitbox rect from that entity every frame, and
// self-destructs if that entity's entityId isn't 0x1B (E_UNK_1B ==
// func_us_801B385C, this overlay half's own conductor).
void func_us_801B4F5C(Entity* self) {
    Entity* prev = self - 1;

    if (self->step == 0) {
        InitializeEntity(D_us_8018056C);
        self->animCurFrame = 0;
    }

    if (prev->entityId != 0x1B) {
        DestroyEntity(self);
        return;
    }

    {
        u8 frameIndex = D_us_80181A88[prev->animCurFrame];
        u8* rec = &D_us_80181A3C[frameIndex * 4];

        self->facingLeft = prev->facingLeft;
        self->posX = prev->posX;
        self->posY = prev->posY;
        self->hitboxOffX = (s8) rec[0];
        self->hitboxOffY = (s8) rec[1];
        self->hitboxWidth = rec[2];
        self->hitboxHeight = rec[3];
    }
}

// step 0: initializes, sets a launch velocityX from facingLeft, then reads
// a linked entity pointer stored at ext+0x7C -- here genuinely an Entity*
// (see bo2.h comment on this function), NOT the Primitive* that
// func_us_801B52FC below stores at the same offset -- to decide whether
// both self and the linked entity's ENTITY_UNK8F flag should be armed.
// step 1 (falls in from step 0): moves, derives an angle from
// velocityX/velocityY via ratan2, and picks one of 4 animCurFrame values
// from angle buckets; once ENTITY_UNK8F(self) is set, checks X-proximity to
// the linked entity and, once close enough, arms the linked entity's own
// ENTITY_UNK8F, bounces facingLeft/velocityX, and advances self->step.
// step 2: just moves.
void func_us_801B503C(Entity* self) {
    switch (self->step) {
        case 0: {
            Entity* linked = *(Entity**) ((u8*) self + 0x7C);
            s32 v1;

            InitializeEntity(D_us_80180578);
            self->animCurFrame = 0x39;
            self->velocityX = self->facingLeft ? -0x48000 : 0x48000;

            self->velocityY = 0;
            v1 = linked->velocityX;
            if (self->facingLeft) {
                v1 = -v1;
            }
            if (v1 > 0 || (linked->flags & 0x100)) {
                ENTITY_UNK8F(self) = 0;
                ENTITY_UNK8F(linked) = 1;
            }
        }
        // fallthrough
        case 1: {
            s32 angle;

            MoveEntity();

            self->velocityY -= 0x1400;
            {
                s32 a0 = -self->velocityY;
                s32 a1 = self->velocityX;

                if (a1 < 0) {
                    a1 = -a1;
                }
                angle = ratan2(a0, a1);
            }

            if (angle >= 0x201) {
                if (angle < 0x281) {
                    self->animCurFrame = 0x3A;
                } else if (angle < 0x301) {
                    self->animCurFrame = 0x3B;
                } else if (angle < 0x381) {
                    self->animCurFrame = 0x3C;
                } else {
                    self->animCurFrame = 0x3D;
                }
            }

            if (ENTITY_UNK8F(self) != 0) {
                Entity* linked2;
                s32 diff;

                PlaySfxPositional(0x640);
                linked2 = *(Entity**) ((u8*) self + 0x7C);
                diff = self->posX.i.hi - linked2->posX.i.hi;
                if (diff < 0) {
                    diff = -diff;
                }
                if (diff < 0x10) {
                    ENTITY_UNK8F(linked2) = 1;
                    self->velocityY = 0x20000;
                    self->animCurFrame = 0x39;
                    self->facingLeft ^= 1;
                    self->velocityX = -self->velocityX * 2;
                    self->step++;
                }
            }
            break;
        }

        case 2:
            MoveEntity();
            break;

        default:
            break;
    }
}

// A flash/pulse effect entity: step 0 initializes blend/draw flags,
// opacity, palette (D_us_80180566 - 0x7FF7) and flags |= 0x202000, then
// every frame cycles animCurFrame between 0 and self->params via an
// ENTITY_UNK80 frame counter (resets to 0 when the counter itself wraps to
// 0), and fades opacity down by 8 each frame, self-destructing once it
// reaches 0.
void func_us_801B5218(Entity* self) {
    u16 counter;

    if (self->step == 0) {
        InitializeEntity(D_us_80180560);
        self->blendMode = 0x30;
        self->drawFlags = 0xC;
        self->opacity = 0x60;
        self->hitboxState = 0;
        self->palette = D_us_80180566 - 0x7FF7;
        self->animCurFrame = self->params;
        self->flags |= 0x202000;
    }

    counter = ENTITY_UNK80(self) + 1;
    ENTITY_UNK80(self) = counter;
    if ((s16) counter != 0) {
        self->animCurFrame = self->params;
    } else {
        self->animCurFrame = 0;
    }

    self->opacity -= 8;
    if (self->opacity == 0) {
        DestroyEntity(self);
    }
}

// step 0: allocates 3 chained PRIM_GT4 prims, sets up their quad geometry
// from posX/posY/zPriority (wide 0x18px half-width if self->params==0, else
// narrow 0xC), stores the chain head at ext+0x7C (ENTITY_UNK7C, a genuine
// Primitive* here), and -- when self->params==0 -- spawns two E_UNK_1F
// helper entities via func_us_801A9D5C, offset +-0x24 in posX.i.hi.
// step 1/2: an alternating countdown/countup pair of animation-progress
// counters piggybacked on the prim chain's own tpage/clut/u/v fields
// (reused here as scratch bookkeeping rather than real texture info, same
// "reuse a struct's memory for unrelated data" idiom seen with the
// Primitive-shaped Fake/Axe/FrozenShade/NumericPrim variants); step 1 counts
// up (self->step++ once close enough), step 2 counts down (self->step++
// once close enough), step 3 destroys self. Every step always finishes by
// deriving self->hitboxWidth/Height from the prim chain head's own
// x0/x1/y0/y2 spread and setting an alternating UV flash pattern gated on
// g_Timer's parity.
void func_us_801B52FC(Entity* self) {
    switch (self->step) {
        case 0: {
            s32 primIndex = (s16) g_api.AllocPrimitives(PRIM_GT4, 3);
            Primitive* p;

            InitializeEntity(D_us_80180584);
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }

            p = &g_PrimBuf[primIndex];
            self->primIndex = primIndex;
            ENTITY_UNK7C(self) = p;
            self->flags |= 0x800000;

            for (; p != NULL; p = p->next) {
                p->tpage = 0x16;
                p->clut = 0x211;
                p->u2 = 0x80;
                p->u0 = 0x80;
                p->u3 = 0xA0;
                p->u1 = 0xA0;
                p->v1 = 0;
                p->v0 = 0;
                p->v3 = 0;
                p->v2 = 0;
                if (self->params == 0) {
                    p->x2 = p->x0 = self->posX.i.hi - 0x18;
                    p->x3 = p->x1 = self->posX.i.hi + 0x18;
                } else {
                    p->x2 = p->x0 = self->posX.i.hi - 0xC;
                    p->x3 = p->x1 = self->posX.i.hi + 0xC;
                }
                p->y3 = p->y2 = p->y1 = p->y0 = self->posY.i.hi;
                p->drawMode = 2;
                p->priority = self->zPriority;
            }

            if (self->params == 0) {
                s32 offset = -0x24;
                s32 i;

                for (i = -1; i < 2; i += 2) {
                    Entity* e = AllocEntity(g_Entities_160, (Entity*) ((u8*) g_Entities_160 + 0x1780));
                    if (e != NULL) {
                        func_us_801A9D5C(0x1F, self, e);
                        e->params = 1;
                        e->posX.i.hi += offset;
                    }
                    offset += 0x48;
                }
            }
        }
        // fallthrough
        case 1: {
            Primitive* p = ENTITY_UNK7C(self);
            s32 s1 = (self->params != 0) ? 4 : 2;
            s32 a0 = s1 << 1;
            s32 v1 = p->v3 + s1;
            s32 v0 = p->tpage - a0;
            s32 a1 = p->v0;

            p->v2 = v1;
            p->tpage = v0;
            p->y0 = v0;
            p->v3 = v1;
            if ((v1 - a1) < 0x40) {
                break;
            }
            self->step++;
            break;
        }

        case 2: {
            Primitive* p = ENTITY_UNK7C(self);
            s32 s1 = (self->params != 0) ? 4 : 2;
            s32 half = s1 / 2;
            s32 a0 = p->tpage + s1;
            s32 v1 = p->v3 - half;
            s32 a2 = p->y2;

            p->y0 = a0;
            p->tpage = a0;
            p->v3 = v1;
            p->v2 = v1;
            if ((a2 - a0) <= 0) {
                self->step++;
            }
            break;
        }

        case 3:
            DestroyEntity(self);
            break;

        default:
            break;
    }

    {
        Primitive* p = ENTITY_UNK7C(self);
        s32 w = p->x0 - p->x1;
        s32 h = p->y0 - p->y2;

        if (w < 0) {
            w = -w;
        }
        self->hitboxWidth = (w - 8) / 2;

        if (h < 0) {
            h = -h;
        }
        self->hitboxHeight = (h * 3) / 10;

        self->hitboxOffY = 0xE - self->hitboxHeight;
        if (g_Timer & 1) {
            p->u2 = p->u0 = 0x80;
            p->u3 = p->u1 = 0xA0;
        } else {
            p->u2 = p->u0 = 0xA0;
            p->u3 = p->u1 = 0x80;
        }
    }
}

// Boss-variant twin of the regular Werewolf's EntityWerewolfDeathFlames
// (src/st/rare/e_werewolf.c) -- same effect (a chain of death-flame prims
// launched radially and animated via a byte-indexed sprite-frame table),
// just different tuning constants (0x140 lifetime vs 0x100, faster r0
// fade -2/frame vs -1, 0x81/0x80 color clamp vs 0x60/0x40, a byte-based
// D_us_801A16F0 sprite table instead of e_werewolf's u16-based
// sprites_rare_4). This is what resolved the `$s5`/otz question below:
// e_werewolf.c's VERSION_US branch calls `gte_stszotz(otz)` (passing the
// plain `long otz` local by value, not `&otz`) -- an already-established,
// accepted quirk of this exact macro under VERSION_US, not a bug specific
// to this function. Confirmed field-for-field against that twin (ext+0x7C
// prim chain head, ext+0x80 timer, ext+0x9C a running offset) rather than
// guessed.
void func_us_801B56C0(Entity* self) {
    switch (self->step) {
        case 0: {
            s32 primIndex = (s16) g_api.AllocPrimitives(PRIM_GT4, 0x80);
            Primitive* p;

            InitializeEntity(D_us_801804E8); // g_EInitInteractable
            if (primIndex == -1) {
                DestroyEntity(self);
                return;
            }

            self->flags |= 0x800000;
            self->primIndex = primIndex;
            p = &g_PrimBuf[primIndex];
            ENTITY_UNK7C(self) = p;

            for (; p != NULL; p = p->next) {
                p->tpage = 0x17;
                p->clut = 0x223;
                p->drawMode = DRAW_HIDE;
                p->priority = self->zPriority;
            }

            ENTITY_UNK80(self) = 0x140;
        }
        // fallthrough
        case 1: {
            u16 timer = ENTITY_UNK80(self) - 1;
            Primitive* p;
            s32 posY;

            ENTITY_UNK80(self) = timer;
            if (timer == 0) {
                DestroyEntity(self);
                return;
            }

            if ((timer & 0x1F) == 0) {
                PlaySfxPositional(0x661);
            }

            if ((timer & 3) == 0) {
                s32 color;
                s32 i;

                ENTITY_UNK9C(self) -= 0x20;
                color = (s16) timer;
                if (color >= 0x81) {
                    color = 0x80;
                }

                for (i = 0; i < 3; i++) {
                    Primitive* found = FindFirstUnkPrim2(ENTITY_UNK7C(self), 2);
                    if (found != NULL) {
                        UnkPolyFunc2(found);
                        found->next->x2 = 0;
                        found->next->y2 = 0;
                        found->next->x1 = ENTITY_UNK9C(self) + (i * 0x555) + Random();
                        found->next->y1 = 0;
                        PGREY(found, 0) = PGREY(found, 1) = PGREY(found, 2) =
                            PGREY(found, 3) = color;
                        found->next->drawMode = DRAW_HIDE;
                    }
                }
            }

            SetGeomScreen(0x200);
            SetGeomOffset(self->posX.i.hi, self->posY.i.hi);
            p = ENTITY_UNK7C(self);
            posY = self->posY.i.hi + 8;

            for (; p != NULL; p = p->next) {
                if (p->p3 == 8) {
                    VECTOR vec;
                    SVECTOR svec;
                    MATRIX matrix;
                    long otz;

                    vec.vx = 0;
                    vec.vy = p->next->y1;
                    vec.vz = 0x200;
                    TransMatrix(&matrix, &vec);
                    SetTransMatrix(&matrix);

                    svec.vx = 0;
                    svec.vy = p->next->x1;
                    svec.vz = 0x180;
                    RotMatrix(&svec, &matrix);
                    SetRotMatrix(&matrix);

                    gte_ldv3c(D_us_80181ACC);
                    gte_rtpt();
                    gte_stsxy3_gt3(p);
                    gte_ldv0(&D_us_80181ACC[3]);
                    gte_rtps();
                    gte_stsxy((long*) &p->x3);
                    gte_avsz4();
                    gte_stszotz(otz);

                    if (otz > 0x80) {
                        p->priority = self->zPriority - 1;
                    } else {
                        p->priority = self->zPriority + 1;
                    }

                    if (posY < p->y2) {
                        p->y2 = posY;
                    }
                    if (posY < p->y3) {
                        p->y3 = posY;
                    }
                    p->drawMode = DRAW_TPAGE2 | DRAW_TPAGE | DRAW_COLORS |
                                  DRAW_UNK02 | DRAW_TRANSP;
                }
            }

            p = ENTITY_UNK7C(self);
            for (; p != NULL; p = p->next) {
                if (p->p3 == 8) {
                    p->next->y1 -= 3;
                    p->next->x1 += 0x30;
                    p->r0 -= 2;
                    if (p->r0 == 0) {
                        UnkPolyFunc0(p);
                        continue;
                    }

                    p->g0 = p->b0 = p->r0;
                    PGREY(p, 1) = PGREY(p, 2) = PGREY(p, 3) = p->r0;

                    if (p->next->y2 == 0) {
                        p->next->x2++;
                        if (p->next->x2 > 0xD) {
                            UnkPolyFunc0(p);
                            continue;
                        }

                        {
                            u8* ptr = D_us_801A16F0[p->next->x2] + 0x10;

                            p->u2 = p->u0 = *ptr;
                            ptr += 2;
                            p->v1 = p->v0 = *ptr;
                            ptr += 2;
                            p->u3 = p->u1 = *ptr;
                            p->v3 = p->v2 = ptr[2];
                        }
                        p->next->y2 = 3;
                    } else {
                        p->next->y2--;
                    }
                }
            }
            break;
        }

        default:
            break;
    }
}

// Empty stub (nop; jr ra) -- no-op entity update, matches the real asm exactly.
void func_us_801B5C2C(Entity* self) {
}
