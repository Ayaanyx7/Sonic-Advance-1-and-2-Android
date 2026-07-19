#include "global.h"
#include "game/sa2/assets/compressed/roms.h"

// Macro helper to safely stringify macro definitions passed by the Makefile
#define ASSET_STRING_GLUE(x) #x
#define ASSET_PATH(dir, file) ASSET_STRING_GLUE(dir/file)

#ifdef __ANDROID__
  #ifdef JAPAN
    __asm__(
        ".section .rodata\n"
        ".global gMultiBootProgram_TinyChaoGarden\n"
        ".align 2\n"
        "gMultiBootProgram_TinyChaoGarden:\n"
        ".incbin " ASSET_PATH(WORKSPACE_ROOT, data/sa2/mb_chao_garden_japan.gba.lz) "\n"
    );
  #else
    __asm__(
        ".section .rodata\n"
        ".global gMultiBootProgram_TinyChaoGarden\n"
        ".align 2\n"
        "gMultiBootProgram_TinyChaoGarden:\n"
        ".incbin " ASSET_PATH(WORKSPACE_ROOT, chao_garden/mb_chao_garden.gba.lz) "\n"
    );
  #endif
#else
  // Original GBA toolchain behaviors left entirely intact
  #ifdef JAPAN
  const u8 gMultiBootProgram_TinyChaoGarden[] = INCBIN_U8("data/sa2/mb_chao_garden_japan.gba.lz");
  #else
  const u8 gMultiBootProgram_TinyChaoGarden[] = INCBIN_U8("chao_garden/mb_chao_garden.gba.lz");
  #endif
#endif
