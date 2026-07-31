// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo1.h"

// Confirmed via real disassembly (EntityHeartDrop.s): self->params is used
// directly as the g_CastleFlags index with no visible add/sub instruction,
// same as bo0/bo4's value.
#define HEART_DROP_CASTLE_FLAG 0

#include "../../st/e_collect.h"
