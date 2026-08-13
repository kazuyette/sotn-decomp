// Superseded: these 5 addresses are pointer targets INSIDE gfx_data's own
// raw bytes (bucket A's pointer table references them), not separate code.
// Claiming them via INCLUDE_ASM here would duplicate their bytes elsewhere
// in the file and break byte-exactness. Fixed instead via linker-level
// symbol aliases appended directly to gfx_data.c/tilemaps.c (see
// probe_dre_gen_aliases.sh) which point the names at the existing content
// with zero added bytes. This file intentionally left compiling to nothing.
#include "common.h"
