#include "global.h"
#include "game/sa2/assets/compressed/roms.h"

#ifdef __ANDROID__

#ifdef JAPAN

__asm__(
    ".section .rodata\n"
    ".global gMultiBootProgram_TinyChaoGarden\n"
    ".align 2\n"
    "gMultiBootProgram_TinyChaoGarden:\n"
    ".incbin \"" WORKSPACE_ROOT "/data/sa2/mb_chao_garden_japan.gba.lz\"\n"
);

#else

__asm__(
    ".section .rodata\n"
    ".global gMultiBootProgram_TinyChaoGarden\n"
    ".align 2\n"
    "gMultiBootProgram_TinyChaoGarden:\n"
    ".incbin \"" WORKSPACE_ROOT "/chao_garden/mb_chao_garden.gba.lz\"\n"
);

#endif

#else

#ifdef JAPAN
const u8 gMultiBootProgram_TinyChaoGarden[] =
    INCBIN_U8("data/sa2/mb_chao_garden_japan.gba.lz");
#else
const u8 gMultiBootProgram_TinyChaoGarden[] =
    INCBIN_U8("chao_garden/mb_chao_garden.gba.lz");
#endif

#endif
