// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo2.h"

// Confirmed via real disassembly (EntityHeartDrop's callers): same
// convention as bo0/bo1/bo4.
#define HEART_DROP_CASTLE_FLAG 0

#include "../../st/e_collect.h"
