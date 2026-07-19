#include "global.h"
#include "game/sa2/assets/compressed/roms.h"

#ifdef __ANDROID__
  #ifdef JAPAN
    __asm__(
        ".section .rodata\n"
        ".align 2\n"
        "__asset_binary_ref__:\n"
        ".incbin \"data/sa2/mb_chao_garden_japan.gba.lz\"\n"
    );
  #else
    __asm__(
        ".section .rodata\n"
        ".align 2\n"
        "__asset_binary_ref__:\n"
        ".incbin \"chao_garden/mb_chao_garden.gba.lz\"\n"
    );
  #endif

  extern const u8 __asset_binary_ref__[];
  const u8 *const gMultiBootProgram_TinyChaoGarden = __asset_binary_ref__;

#else
  // Original GBA toolchain behaviors left entirely intact
  #ifdef JAPAN
  const u8 gMultiBootProgram_TinyChaoGarden[] = INCBIN_U8("data/sa2/mb_chao_garden_japan.gba.lz");
  #else
  const u8 gMultiBootProgram_TinyChaoGarden[] = INCBIN_U8("chao_garden/mb_chao_garden.gba.lz");
  #endif
#endif
