// SPDX-License-Identifier: AGPL-3.0-or-later
#include "bo1.h"

// g_EInitUnkId13 (used only by e_misc.h's own EntityUnkId13, no other
// still-migrated file needs the literal name), confirmed against
// D_us_80180AC8 -- {0,0,0,0,2}. Kept under its raw name + alias since
// nothing else in bo1 references it by the real name, harmless either way.
extern u16 D_us_80180AC8[];
#define g_EInitUnkId13 D_us_80180AC8

// g_EInitParticle and g_bigRedFireballAnim are NOT aliased here (unlike the
// first pass of this checkpoint): both are also referenced by literal name
// from e_collect.h, which either defines the real data itself
// (g_bigRedFireballAnim, hardcoded in e_collect.h, confirmed byte-for-byte
// against D_us_80181950 -- carved as e_collect's own data in
// splat.us.bobo1.yaml) or expects a real definition elsewhere
// (g_EInitParticle, defined for real in e_init.c). Aliasing them to raw
// D_us_ names here would silently rename e_misc.h's own extern references
// to symbols e_collect.c never provides -- this is exactly what broke the
// first WSL attempt on this checkpoint ("undefined reference to
// g_EInitParticle"). e_misc.h already declares both as
// `extern EInit g_EInitParticle;` / `extern u8 g_bigRedFireballAnim[];`
// itself, so nothing further is needed here.

#include "../../st/e_misc.h"
