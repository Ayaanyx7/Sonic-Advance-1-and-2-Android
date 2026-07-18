#
# NOTE: Overrideable default flags are set in config.mk
#
include config.mk

MAKEFLAGS += --no-print-directory

# Clear the default suffixes
.SUFFIXES:
# Don't delete intermediate files
.SECONDARY:
# Delete files that weren't built properly
.DELETE_ON_ERROR:
# Secondary expansion is required for dependency variables in object rules.
.SECONDEXPANSION:

# Quotes must remain to ensure that paths with spaces are respected
ROOT_DIR := "$(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))")"
OS       := $(shell uname)

### TOOLCHAIN ###

# GBA
ifeq ($(PLATFORM),gba)
  TOOLCHAIN := $(DEVKITARM)
  COMPARE ?= 0

  ifeq (compare,$(MAKECMDGOALS))
    COMPARE := 1
  endif

  ifneq (,$(TOOLCHAIN))
    ifneq ($(wildcard $(TOOLCHAIN)/bin),)
	  export PATH := $(TOOLCHAIN)/bin:$(PATH)
    endif
  endif

  PREFIX := arm-none-eabi-
# ANDROID (NDK, arm64)
else ifeq ($(PLATFORM),android)
  ANDROID_NDK_HOME ?= $(ANDROID_NDK_ROOT)
  ANDROID_API      ?= 24
  ANDROID_TOOLCHAIN := $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/bin
  export PATH := $(ANDROID_TOOLCHAIN):$(PATH)
  PREFIX := aarch64-linux-android$(ANDROID_API)-
# x86
else ifeq ($(CPU_ARCH),i386)
  ifeq ($(PLATFORM),sdl_win32)
    TOOLCHAIN := /usr/x86_64-w64-mingw32/
    PREFIX := x86_64-w64-mingw32-
  else ifeq ($(PLATFORM),win32)
    TOOLCHAIN := /usr/x86_64-w64-mingw32/
    PREFIX := x86_64-w64-mingw32-
  endif
# PSP
else ifeq ($(PLATFORM),sdl_psp)
  PSPDEV    ?= $(HOME)/pspdev
  PSPSDK    := $(PSPDEV)/psp/sdk
  export PATH := $(PSPDEV)/bin:$(PATH)
  PREFIX    := psp-
else ifeq ($(PLATFORM),ps2)
  PREFIX := mips64r5900el-ps2-elf-
else
# Native
  ifneq ($(PLATFORM),sdl)
    $(error Unsupported CPU arch for platform '$(CPU_ARCH)', '$(PLATFORM)')
  endif
endif # (PLATFORM == gba)


ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

SHELL     := /bin/bash -o pipefail
SHA1 	  := $(shell { command -v sha1sum || command -v shasum; } 2>/dev/null) -c

ifeq ($(PLATFORM),gba)
CC1       := tools/agbcc/bin/agbcc$(EXE)
CC1_OLD   := tools/agbcc/bin/old_agbcc$(EXE)
else ifeq ($(PLATFORM),android)
CC1       := $(PREFIX)clang$(EXE)
CXX       := $(PREFIX)clang++$(EXE)
CC1_OLD   := $(CC1)
else
CC1       := $(PREFIX)gcc$(EXE)
CXX       := $(PREFIX)g++$(EXE)
CC1_OLD   := $(CC1)
endif

ifeq ($(PLATFORM),android)
CPP       := $(CC1) -E
LD        := $(CC1)
OBJCOPY   := $(PREFIX)objcopy
AS        := $(PREFIX)clang
else
CPP       := $(PREFIX)cpp
LD        := $(PREFIX)ld
OBJCOPY   := $(PREFIX)objcopy
AS 		  := $(PREFIX)as
endif

FORMAT    := clang-format-13

### TOOLS ###
GFX 	  := tools/gbagfx/gbagfx$(EXE)
ENT_POS   := tools/entity_positions/entity_positions$(EXE)
AIF		  := tools/aif2pcm/aif2pcm$(EXE)
MID2AGB   := tools/mid2agb/mid2agb$(EXE)
SCANINC   := tools/scaninc/scaninc$(EXE)
PREPROC	  := tools/preproc/preproc$(EXE)
RAMSCRGEN := tools/ramscrgen/ramscrgen$(EXE)
FIX 	  := tools/gbafix/gbafix$(EXE)

TOOLDIRS := $(filter-out tools/agbcc/ tools/BriBaSA_ex/, $(dir $(wildcard tools/*/Makefile)))
TOOLBASE = $(TOOLDIRS:tools/%=%)
TOOLS = $(foreach tool,$(TOOLBASE),tools/$(tool)/$(tool)$(EXE))

### DEPS ###

SDL_MINGW_PKG     := $(ROOT_DIR)/ext/SDL2-2.30.3/x86_64-w64-mingw32
SDL_MINGW_INCLUDE := $(SDL_MINGW_PKG)/include/SDL2
SDL_MINGW_SDL_DLL := $(SDL_MINGW_PKG)/bin/SDL2.dll
SDL_MINGW_LIB     := $(SDL_MINGW_PKG)/lib
SDL_MINGW_FLAGS   := -I$(SDL_MINGW_INCLUDE) -D_THREAD_SAFE
SDL_MINGW_LIBS    := -L$(SDL_MINGW_LIB) -lSDL2main -lSDL2.dll

# For Android, SDL2 headers/libs come from the android-project template's
# own vendored SDL2 source (app/jni/SDL), not a system package.
ANDROID_SDL_DIR   := android-project/app/jni/SDL
ANDROID_SDL_FLAGS := -I$(ANDROID_SDL_DIR)/include

LIBABGSYSCALL_LIBS := -L$(ROOT_DIR)/libagbsyscall/build/$(PLATFORM) -lagbsyscall

### FILES ###

OBJ_DIR  := build/$(PLATFORM)/$(BUILD_NAME)
ifeq ($(PLATFORM),gba)
ROM      := $(BUILD_NAME).gba
ELF      := $(ROM:.gba=.elf)
MAP      := $(ROM:.gba=.map)
else ifeq ($(PLATFORM),sdl)
ROM      := $(BUILD_NAME).sdl
ELF      := $(ROM).elf
MAP      := $(ROM).map
else ifeq ($(PLATFORM),android)
ROM      := lib$(BUILD_NAME).so
ELF      := $(ROM).elf
MAP      := $(BUILD_NAME).android.map
else ifeq ($(PLATFORM),sdl_psp)
ROM      := EBOOT.PBP
ELF      := $(BUILD_NAME).sdl_psp.elf
MAP      := $(BUILD_NAME).sdl_psp.map
else ifeq ($(PLATFORM),ps2)
ROM      := $(BUILD_NAME).$(PLATFORM).iso
ELF      := $(ROM:.iso=.elf)
MAP      := $(ROM:.iso=.map)
else
ROM      := $(BUILD_NAME).$(PLATFORM).exe
ELF      := $(ROM:.exe=.exe)
MAP      := $(ROM:.exe=.map)
endif

INCLUDE_DIRS = include
INCLUDE_CPP_ARGS := $(INCLUDE_DIRS:%=-iquote %)
INCLUDE_SCANINC_ARGS := $(INCLUDE_DIRS:%=-I %)

ASM_SUBDIR = asm
ASM_BUILDDIR = $(OBJ_DIR)/$(ASM_SUBDIR)

C_SUBDIR = src
C_BUILDDIR = $(OBJ_DIR)/$(C_SUBDIR)

DATA_ASM_SUBDIR = data/$(GAME_NAME)
DATA_ASM_BUILDDIR = $(OBJ_DIR)/$(DATA_ASM_SUBDIR)

SONG_SUBDIR = sound/$(GAME_NAME)/songs
SONG_BUILDDIR = $(OBJ_DIR)/$(SONG_SUBDIR)

SOUND_ASM_SUBDIR = sound
SOUND_ASM_BUILDDIR = $(OBJ_DIR)/$(SOUND_ASM_SUBDIR)

MID_SUBDIR = sound/$(GAME_NAME)/songs/midi
MID_BUILDDIR = $(OBJ_DIR)/$(MID_SUBDIR)

SAMPLE_SUBDIR = sound/$(GAME_NAME)/direct_sound_samples
SHARED_SAMPLE_SUBDIR = sound/shared/direct_sound_samples

OBJ_TILES_4BPP_SUBDIR = graphics/$(GAME_NAME)/obj_tiles/4bpp
TILESETS_SUBDIR = graphics/$(GAME_NAME)/tilesets/

ifeq ($(GAME), GAME_SA1)
C_SRC_IGNORE_PATHS := -not -path "*/sa2/*"
else ifeq ($(GAME), GAME_SA2)
C_SRC_IGNORE_PATHS := -not -path "*/sa1/*"
endif

ifeq ($(PLATFORM),gba)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/*")
else ifeq ($(PLATFORM),sdl)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/win32/*" -not -path "*/platform/ps2/*")
else ifeq ($(PLATFORM),android)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/win32/*" -not -path "*/platform/ps2/*" -not -path "*/platform/sdl_psp/*")
else ifeq ($(PLATFORM),sdl_psp)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/win32/*" -not -path "*/platform/ps2/*")
else ifeq ($(PLATFORM),ps2)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/win32/*" -not -path "*/platform/pret_sdl/*")
else ifeq ($(PLATFORM),sdl_win32)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/win32/*" -not -path "*/platform/ps2/*")
else ifeq ($(PLATFORM),win32)
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS) -not -path "*/platform/pret_sdl/*" -not -path "*/platform/ps2/*")
else
C_SRCS_IN := $(shell find $(C_SUBDIR) -name "*.c" $(C_SRC_IGNORE_PATHS))
endif

C_SRCS := $(foreach src,$(C_SRCS_IN),$(if $(findstring .inc.c,$(src)),,$(src)))
C_OBJS := $(patsubst $(C_SUBDIR)/%.c,$(C_BUILDDIR)/%.o,$(C_SRCS))

ifeq ($(PLATFORM),gba)
CXX_SRCS := $(shell find $(C_SUBDIR) -name "*.cc" -not -path "*/platform/*")
else
CXX_SRCS := $(shell find $(C_SUBDIR) -name "*.cc")
endif

CXX_OBJS := $(patsubst $(C_SUBDIR)/%.cc,$(C_BUILDDIR)/%.o,$(CXX_SRCS))

C_HEADERS := $(shell find $(INCLUDE_DIRS) -name "*.h" -not -path "*/sa1/*" -not -path "*/platform/*")

ifeq ($(PLATFORM),gba)
C_ASM_SRCS := $(shell find $(C_SUBDIR) -name "*.s")
C_ASM_OBJS := $(patsubst $(C_SUBDIR)/%.s,$(C_BUILDDIR)/%.o,$(C_ASM_SRCS))

ASM_SRCS := $(wildcard $(ASM_SUBDIR)/*.s)
ASM_OBJS := $(patsubst $(ASM_SUBDIR)/%.s,$(ASM_BUILDDIR)/%.o,$(ASM_SRCS))
endif

DATA_ASM_SRCS := $(wildcard $(DATA_ASM_SUBDIR)/*.s)
DATA_ASM_OBJS := $(patsubst $(DATA_ASM_SUBDIR)/%.s,$(DATA_ASM_BUILDDIR)/%.o,$(DATA_ASM_SRCS))

SONG_SRCS := $(wildcard $(SONG_SUBDIR)/*.s)
SONG_OBJS := $(patsubst $(SONG_SUBDIR)/%.s,$(SONG_BUILDDIR)/%.o,$(SONG_SRCS))

MID_SRCS := $(wildcard $(MID_SUBDIR)/*.mid)
MID_OBJS := $(patsubst $(MID_SUBDIR)/%.mid,$(MID_BUILDDIR)/%.o,$(MID_SRCS))

SOUND_ASM_SRCS := $(wildcard $(SOUND_ASM_SUBDIR)/*.s)
SOUND_ASM_OBJS := $(patsubst $(SOUND_ASM_SUBDIR)/%.s,$(SOUND_ASM_BUILDDIR)/%.o,$(SOUND_ASM_SRCS))

OBJS := $(C_OBJS) $(CXX_OBJS) $(ASM_OBJS) $(C_ASM_OBJS) $(DATA_ASM_OBJS) $(SONG_OBJS) $(MID_OBJS)
OBJS_REL := $(patsubst $(OBJ_DIR)/%,%,$(OBJS))

FORMAT_SRC_PATHS := $(shell find . -name "*.c" ! -path '*/src/data/*' ! -path '*/build/*' ! -path '*/ext/*')
FORMAT_H_PATHS   := $(shell find . -name "*.h" ! -path '*/build/*' ! -path '*/ext/*')

### COMPILER FLAGS ###

CPPFLAGS ?= $(INCLUDE_CPP_ARGS) -D $(GAME_REGION) -D GAME=$(GAME)
CC1FLAGS ?= -Wimplicit -Wparentheses -Werror

ifneq ($(GAME_VARIANT), DEFAULT)
	CPPFLAGS += -D $(GAME_VARIANT)
endif

ifeq ($(PLATFORM),gba)
	INCLUDE_SCANINC_ARGS += -I tools/agbcc/include
	CPPFLAGS += -D PLATFORM_GBA=1 -D PLATFORM_SDL=0 -D PLATFORM_WIN32=0 -D CPU_ARCH_X86=0 -D CPU_ARCH_ARM=1 -nostdinc -I tools/agbcc/include
	CC1FLAGS += -fhex-asm

ifeq ($(GAME_NAME), sa1)
    PROLOGUE_FIX := -fprologue-bugfix
endif # BUILD_NAME == sa1

else
	CC1FLAGS += -Wstrict-overflow=1
	ifeq ($(PLATFORM),sdl)
		CC1FLAGS += -Wno-parentheses-equality -Wno-unused-value
		CPPFLAGS += -D TITLE_BAR=$(BUILD_NAME).$(PLATFORM) -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 $(shell sdl2-config --cflags)
	else ifeq ($(PLATFORM),android)
		CC1FLAGS += -Wno-parentheses-equality -Wno-unused-value -fPIC
		CPPFLAGS += -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 -D ANDROID -D __ANDROID__ $(ANDROID_SDL_FLAGS) --target=aarch64-linux-android$(ANDROID_API) --sysroot=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
	else ifeq ($(PLATFORM),sdl_psp)
		CC1FLAGS += -G0
		CPPFLAGS += -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 -D SDL_MAIN_HANDLED -I$(PSPDEV)/psp/include/SDL2 -I$(PSPDEV)/psp/include -I$(PSPSDK)/include -D_PSP_FW_VERSION=600
	else ifeq ($(PLATFORM),ps2)
		CC1FLAGS += -G0 -Wno-parentheses-equality -Wno-unused-value -ffast-math
		CPPFLAGS += -D PLATFORM_GBA=0 -D PLATFORM_SDL=0 -D PLATFORM_WIN32=0 -D_EE -D__PS2__ -I$(PS2SDK)/common/include -I$(PS2SDK)/ee/include -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include
	else ifeq ($(PLATFORM),sdl_win32)
		CPPFLAGS += -D TITLE_BAR=$(BUILD_NAME).$(PLATFORM) -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 $(SDL_MINGW_FLAGS)
	else ifeq ($(PLATFORM),win32)
		CPPFLAGS += -D TITLE_BAR=$(BUILD_NAME).$(PLATFORM) -D PLATFORM_GBA=0 -D PLATFORM_SDL=0 -D PLATFORM_WIN32=1
	endif

	ifeq ($(CPU_ARCH),i386)
        CPPFLAGS += -D CPU_ARCH_X86=1 -D CPU_ARCH_ARM=0
        CC1FLAGS += -masm=intel
	else 
        CPPFLAGS += -D CPU_ARCH_X86=0 -D CPU_ARCH_ARM=0
	endif
endif

ifeq ($(DEBUG),1)
  CC1FLAGS += -g3 -O0
  CPPFLAGS += -D DEBUG=1
else
  ifeq ($(PLATFORM),sdl_psp)
    CC1FLAGS += -O3 -funroll-loops -fomit-frame-pointer
  else ifeq ($(PLATFORM),ps2)
    CC1FLAGS += -O3 -fomit-frame-pointer
  else
    CC1FLAGS += -O2
  endif
  CPPFLAGS += -D DEBUG=0
endif

ifeq ($(PORTABLE),1)
  CPPFLAGS += -D PORTABLE=1
else
  CPPFLAGS += -D PORTABLE=0
endif

ifeq ($(TAS_TESTING),1)
  CPPFLAGS += -D TAS_TESTING=1
else
  CPPFLAGS += -D TAS_TESTING=0
endif

ifeq ($(ENABLE_DECOMP_CREDITS),0)
  CPPFLAGS += -D ENABLE_DECOMP_CREDITS=0
else
  CPPFLAGS += -D ENABLE_DECOMP_CREDITS=1
endif

CXXFLAGS := $(CC1FLAGS) $(CPPFLAGS) -fno-rtti -fno-exceptions -std=c++11

ifeq ($(PLATFORM),gba)
  ASFLAGS  += -mcpu=arm7tdmi -mthumb-interwork
  CC1FLAGS += -mthumb-interwork
else
  ifeq ($(PLATFORM), sdl)
    CPP := $(CC1) -E
  else ifeq ($(PLATFORM), android)
    CPP := $(CC1) -E --target=aarch64-linux-android$(ANDROID_API) --sysroot=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
  else ifeq ($(PLATFORM), sdl_psp)
    CPP := $(CC1) -E
  else ifeq ($(PLATFORM), ps2)
    ASFLAGS  += -msingle-float
  endif
  CC1FLAGS += -x c -S
  CXXFLAGS += -x c++ -S
endif

### LINKER FLAGS ###

ifeq ($(PLATFORM),gba)
    MAP_FLAG := -Map
else ifeq ($(PLATFORM),sdl)
    ifeq ($(OS), Darwin)
        MAP_FLAG := -Wl,-map,
    else
        MAP_FLAG := -Xlinker -Map=
    endif
else ifeq ($(PLATFORM),android)
    MAP_FLAG := -Xlinker -Map=
else
    MAP_FLAG := -Xlinker -Map=
endif

ifeq ($(PLATFORM),gba)
    LIBS := $(ROOT_DIR)/tools/agbcc/lib/libgcc.a $(ROOT_DIR)/tools/agbcc/lib/libc.a $(LIBABGSYSCALL_LIBS)
else ifeq ($(PLATFORM),sdl)
    LIBS := $(shell sdl2-config --cflags --libs) $(LIBABGSYSCALL_LIBS) -lm
else ifeq ($(PLATFORM),android)
    LIBS := --target=aarch64-linux-android$(ANDROID_API) --sysroot=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot \
            -shared -uANativeActivity_onCreate \
            -L$(ANDROID_SDL_DIR)/build/intermediates -lSDL2 -llog -landroid \
            $(LIBABGSYSCALL_LIBS) -lm
else ifeq ($(PLATFORM),sdl_psp)
    LIBS := -L$(PSPDEV)/psp/lib $(LIBABGSYSCALL_LIBS) -L$(PSPSDK)/lib -lSDL2 -lm -lGL -lpspvram -lpspaudio -lpspvfpu -lpspdisplay -lpspgu -lpspge -lpsphprm -lpspctrl -lpsppower -lpspdebug -lpspnet -lpspnet_apctl -Wl,-zmax-page-size=128
else ifeq ($(PLATFORM),ps2)
    LIBS := -T$(PS2SDK)/ee/startup/linkfile $(LIBABGSYSCALL_LIBS) -L$(PS2SDK)/common/lib -L$(PS2SDK)/ee/lib -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib -lgskit -ldmakit -lps2_drivers -lmc -lpatches -Wl,-zmax-page-size=128
else ifeq ($(PLATFORM),sdl_win32)
    LIBS := -mwin32 -lkernel32 -lwinmm -lmingw32 -lxinput $(LIBABGSYSCALL_LIBS) $(SDL_MINGW_LIBS)
else ifeq ($(PLATFORM), win32)
    LIBS := -mwin32 -lkernel32 -lwinmm -lgdi32 -lxinput -lopengl32 $(LIBABGSYSCALL_LIBS)
endif

#### MAIN TARGETS ####

.PHONY: clean tools tidy clean-tools $(TOOLDIRS) libagbsyscall ps2 sa1

$(shell mkdir -p $(C_BUILDDIR) $(ASM_BUILDDIR) $(DATA_ASM_BUILDDIR) $(SOUND_ASM_BUILDDIR) $(SONG_BUILDDIR) $(MID_BUILDDIR))

infoshell = $(foreach line, $(shell $1 | sed "s/ /__SPACE__/g"), $(info $(subst __SPACE__, ,$(line))))

ifeq (,$(filter-out all rom compare libagbsyscall,$(MAKECMDGOALS)))
$(call infoshell, $(MAKE) tools)
MAKE_TOOLS_OUTCOME=$(shell $(MAKE) tools > /dev/null 2>&1 && echo 0 || echo 1)
ifneq ($(MAKE_TOOLS_OUTCOME),0)
  $(error Make tools command failed!)
endif
else
NODEP ?= 1
endif

ifneq ($(NODEP),1)
export MACOSX_DEPLOYMENT_TARGET := 11
endif

ifeq ($(PLATFORM),gba)
$(C_BUILDDIR)/lib/m4a/m4a.o: CC1 := $(CC1_OLD)
$(C_BUILDDIR)/lib/m4a/m4a.o: PROLOGUE_FIX :=
$(C_BUILDDIR)/lib/agb_flash/agb_flash.o:  CC1FLAGS := -O1 -mthumb-interwork -Werror
$(C_BUILDDIR)/lib/agb_flash/agb_flash%.o: CC1FLAGS := -O1 -mthumb-interwork -Werror
endif

#### Main Targets ####

ifeq ($(PLATFORM),gba)
all: compare

compare: rom
	$(SHA1) $(BUILD_NAME).sha1
else
all: rom
endif

rom: $(ROM)

tools: $(TOOLDIRS)

tool_libs:
	@$(MAKE) -C tools/_shared

clean: tidy clean-tools
	@$(MAKE) clean -C tools/BriBaSA_ex
	@$(MAKE) clean -C chao_garden
	@$(MAKE) clean -C multi_boot/subgame_bootstrap
	@$(MAKE) clean -C multi_boot/programs/subgame_loader
	@$(MAKE) clean -C multi_boot/collect_rings
	@$(MAKE) clean -C libagbsyscall PLATFORM=$(PLATFORM) CPU_ARCH=$(CPU_ARCH)

ifneq ($(GAME_NAME),sa1)
	find sound \( -iname '*.bin' \) -exec $(RM) {} +
	find . \( -iwholename './data/*/maps/*/*/entities/*.bin' -o -iname '*.1bpp' -o -iname '*.4bpp' -o -iname '*.8bpp' -o -iname '*.gbapal' -o -iname '*.lz' -o -iname '*.rl' -o -iname '*.latfont' -o -iname '*.hwjpnfont' -o -iname '*.fwjpnfont' \) -exec $(RM) {} +

	@$(MAKE) clean GAME_NAME=sa1
endif

clean-tools:
	@$(foreach tooldir,$(TOOLDIRS),$(MAKE) clean -C $(tooldir);)

tidy:
	$(RM) -r build/*
	$(RM) SDL2.dll
	$(RM) $(BUILD_NAME)*.exe $(BUILD_NAME)*.elf $(BUILD_NAME)*.map $(BUILD_NAME)*.sdl $(BUILD_NAME)*.gba $(BUILD_NAME)*.iso $(BUILD_NAME)*.so
	$(RM) EBOOT.PBP PARAM.SFO

usa_beta: ; @$(MAKE) GAME_REGION=USA GAME_VARIANT=BETA

japan: ; @$(MAKE) GAME_REGION=JAPAN

japan_vc: ; @$(MAKE) GAME_REGION=JAPAN GAME_VARIANT=VIRTUAL_CONSOLE

europe: ; @$(MAKE) GAME_REGION=EUROPE

sdl: ; @$(MAKE) PLATFORM=sdl

android: ; @$(MAKE) PLATFORM=android

sdl_psp: ; @$(MAKE) PLATFORM=sdl_psp

ps2: ; @$(MAKE) PLATFORM=ps2

tas_sdl: ; @$(MAKE) sdl TAS_TESTING=1

sdl_win32:
	@$(MAKE) PLATFORM=sdl_win32 CPU_ARCH=i386

win32: ; @$(MAKE) PLATFORM=win32 CPU_ARCH=i386

#### RECIPES ####

include $(GAME_NAME)_songs.mk
include graphics.mk

%.s: ;
%.png: ;
%.pal: ;

%.1bpp: %.png  ; $(GFX) $< $@
%.4bpp: %.png  ; $(GFX) $< $@
%.8bpp: %.png  ; $(GFX) $< $@
%.gbapal: %.pal ; $(GFX) $< $@
%.gbapal: %.png ; $(GFX) $< $@

chao_garden/mb_chao_garden.gba.lz: chao_garden/mb_chao_garden.gba 
	$(GFX) $< $@ -search 1
    
data/$(GAME_NAME)/mb_chao_garden_japan.gba.lz: data/$(GAME_NAME)/mb_chao_garden_japan.gba
	$(GFX) $< $@ -search 1

%interactables.bin: %interactables.csv
	$(ENT_POS) $< $@ -entities INTERACTABLES -header "./include/constants/$(GAME_NAME)/interactables.h"

%itemboxes.bin: %itemboxes.csv
	$(ENT_POS) $< $@ -entities ITEMS -header "./include/constants/$(GAME_NAME)/items.h"

%enemies.bin: %enemies.csv
	$(ENT_POS) $< $@ -entities ENEMIES -header "./include/constants/$(GAME_NAME)/enemies.h"

%rings.bin: %rings.csv
	$(ENT_POS) $< $@ -entities RINGS

%.gba.lz: %.gba 
	$(GFX) $< $@
%.bin.lz: %.bin 
	$(GFX) $< $@

%.lz: % ; $(GFX) $< $@
%.rl: % ; $(GFX) $< $@

%.bin: %.aif ; $(AIF) $< $@

$(ELF): $(OBJS)
ifeq ($(PLATFORM),gba)
	@echo "$(LD) -T $(LDSCRIPT) $(MAP_FLAG) $(MAP) <objects> <lib> -o $@"
	@$(CPP) -P $(CPPFLAGS) $(LDSCRIPT) > $(OBJ_DIR)/$(LDSCRIPT)
	@cd $(OBJ_DIR) && $(LD) -T $(LDSCRIPT) $(MAP_FLAG) $(ROOT_DIR)/$(MAP) $(OBJS_REL) $(LIBS) -o $(ROOT_DIR)/$@
else
	@echo "$(CC1) $(MAP_FLAG)$(MAP) <objects> <lib> -o $@"
	@touch $(ROOT_DIR)/$(MAP)
	@cd $(OBJ_DIR) && $(CC1) $(MAP_FLAG)$(ROOT_DIR)/$(MAP) $(OBJS_REL) $(LIBS) -o $(ROOT_DIR)/$@
endif


$(ROM): $(ELF) libagbsyscall
ifeq ($(PLATFORM),gba)
	$(OBJCOPY) -O binary --pad-to 0x8400000 $< $@
	$(FIX) $@ -p -t"$(TITLE)" -c$(GAME_CODE) -m$(MAKER_CODE) -r$(GAME_REVISION) --silent
else ifeq ($(PLATFORM),win32)
	$(OBJCOPY) -O pei-x86-64 $< $@
else ifeq ($(PLATFORM),sdl)
	cp $< $@
else ifeq ($(PLATFORM),android)
	cp $< $@
else ifeq ($(PLATFORM),sdl_psp)
	@echo Creating $(ROM) from $(ELF)
	@psp-fixup-imports $<
	@mksfoex 'Sonic Advance 2' PARAM.SFO
	@psp-strip $< -o $(BUILD_NAME).psp_strip.elf
	@pack-pbp $@ PARAM.SFO NULL NULL NULL NULL NULL $(BUILD_NAME).psp_strip.elf NULL
	@-rm -f $(BUILD_NAME).psp_strip.elf
else ifeq ($(PLATFORM),ps2)
	@echo Creating $(ROM) from $(ELF)
	@mkdir -p $(OBJ_DIR)/iso
	@printf "BOOT2 = cdrom0:\\$(PS2_GAME_CODE);1\nVER = 1.00\nVMODE = NTSC" > $(OBJ_DIR)/iso/SYSTEM.CNF
	@cp $< $(OBJ_DIR)/iso/$(PS2_GAME_CODE)
	@mkisofs -o $(ROM) $(OBJ_DIR)/iso/
else
	$(OBJCOPY) -O pei-x86-64 $< $@
endif

$(C_BUILDDIR)/%.o: $(C_SUBDIR)/%.c
	@echo "$(CC1) <flags> -o $@ $<"
	@$(shell mkdir -p $(shell dirname '$(C_BUILDDIR)/$*.i'))
	@$(CPP) $(CPPFLAGS) $< -o $(C_BUILDDIR)/$*.i
	@$(PREPROC) $(C_BUILDDIR)/$*.i $(if $(filter android,$(PLATFORM)),sdl,$(PLATFORM)) "" | $(CC1) $(PROLOGUE_FIX) $(CC1FLAGS) -o $(C_BUILDDIR)/$*.s -
ifeq ($(PLATFORM), gba)
	@printf ".text\n\t.align\t2, 0\n" >> $(C_BUILDDIR)/$*.s
endif
	@$(AS) $(ASFLAGS) $(C_BUILDDIR)/$*.s -o $@

$(C_BUILDDIR)/%.o: $(C_SUBDIR)/%.cc
	@echo "$(CXX) <flags> -o $@ $<"
	@$(shell mkdir -p $(shell dirname '$(C_BUILDDIR)/$*.o'))
	@$(CXX) $(CXXFLAGS) -o $(C_BUILDDIR)/$*.s $<
	@$(AS) $(ASFLAGS) $(C_BUILDDIR)/$*.s -o $@

$(C_BUILDDIR)/%.d: $(C_SUBDIR)/%.c
	@$(shell mkdir -p $(shell dirname '$(C_BUILDDIR)/$*.d'))
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) $<

$(C_BUILDDIR)/%.d: $(C_SUBDIR)/%.cc
	@$(shell mkdir -p $(shell dirname '$(C_BUILDDIR)/$*.d'))
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) $<

$(C_BUILDDIR)/%.o: $(C_SUBDIR)/%.s
	@echo "$(AS) <flags> -o $@ $<"
	@$(AS) $(ASFLAGS) -o $@ $<

$(ASM_BUILDDIR)/%.o: $(ASM_SUBDIR)/%.s
	@echo "$(AS) <flags> -o $@ $<"
	@$(AS) $(ASFLAGS) -o $@ $<

$(DATA_ASM_BUILDDIR)/%.o: $(DATA_ASM_SUBDIR)/%.s
	@echo "$(AS) <flags> -o $@ $<"
	@$(PREPROC) $< $(if $(filter android,$(PLATFORM)),sdl,$(PLATFORM)) "" | $(CPP) $(CPPFLAGS) - | $(AS) $(ASFLAGS) -o $@ -

$(DATA_ASM_BUILDDIR)/%.d: $(DATA_ASM_SUBDIR)/%.s
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) $<
    
ifneq ($(NODEP),1)
-include $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.d))
-include $(addprefix $(OBJ_DIR)/,$(CXX_SRCS:.cc=.d))
-include $(addprefix $(OBJ_DIR)/,$(DATA_ASM_SRCS:.s=.d))
endif

$(SONG_BUILDDIR)/%.o: $(SONG_SUBDIR)/%.s
	@echo "$(AS) <flags> -o $@ $<"
	@$(PREPROC) $< $(if $(filter android,$(PLATFORM)),sdl,$(PLATFORM)) "" | $(CPP) $(CPPFLAGS) - | $(AS) $(ASFLAGS) -o $@ -

### SUB-PROGRAMS ###

chao_garden/mb_chao_garden.gba: 
ifeq ($(PLATFORM), gba)
	@$(MAKE) -C chao_garden DEBUG=0
else
	@echo "Not building on the chao garden rom, as platform is $(PLATFORM)"
	@printf "1" > chao_garden/mb_chao_garden.gba
endif

chao_garden: tools
	@$(MAKE) -C chao_garden DEBUG=0
    
multi_boot/subgame_bootstrap/subgame_bootstrap.gba: multi_boot/programs/subgame_loader/subgame_loader.bin
ifeq ($(PLATFORM), gba)
	@$(MAKE) -C multi_boot/subgame_bootstrap DEBUG=0
else
	@echo "Not building on the subgame bootstrap rom, as platform is $(PLATFORM)" 
	@printf "1" > multi_boot/subgame_bootstrap/subgame_bootstrap.gba
endif

multi_boot/programs/subgame_loader/subgame_loader.bin:
ifeq ($(PLATFORM), gba)
	@$(MAKE) -C multi_boot/programs/subgame_loader DEBUG=0
else
	@echo "Not building on the subgame loader rom, as platform is $(PLATFORM)" 
	@printf "1" > multi_boot/programs/subgame_loader/subgame_loader.bin
endif

multi_boot/collect_rings/mb_signed_collect_rings.gba:
ifeq ($(PLATFORM), gba)
	@$(MAKE) -C multi_boot/collect_rings DEBUG=0
else
	@echo "Not building on the collect the rings rom, as platform is $(PLATFORM)" 
	@printf "1" > multi_boot/collect_rings/mb_signed_collect_rings.gba
endif

subgame_bootstrap: tools
	@$(MAKE) -C multi_boot/subgame_bootstrap DEBUG=0

subgame_loader: tools
	@$(MAKE) -C multi_boot/programs/subgame_loader DEBUG=0

collect_rings: tools
	@$(MAKE) -C multi_boot/collect_rings DEBUG=0

libagbsyscall:
	@$(MAKE) -C libagbsyscall MODERN=0 PLATFORM=$(PLATFORM) CPU_ARCH=$(CPU_ARCH)

bribasa:
	@$(MAKE) -C tools/BriBaSA_ex

sa1:
	@$(MAKE) GAME_NAME=sa1

sa2:
	@$(MAKE) GAME_NAME=sa2

trilogy: sa1 sa2

$(TOOLDIRS): tool_libs
	@$(MAKE) -C $@
    
### DEPS INSTALL COMMANDS ###

$(SDL_MINGW_LIB):
	@mkdir -p ext
	cd ext && wget -qO- https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-devel-2.30.3-mingw.zip | bsdtar -xvf-

SDL2.dll: $(SDL_MINGW_LIB)
	cp $(SDL_MINGW_SDL_DLL) SDL2.dll

### FORMATTER ###

format:
	@echo $(FORMAT) -i -style=file "**/*.c" "**/*.h"
	@$(FORMAT) -i --verbose -style=file $(FORMAT_SRC_PATHS) $(FORMAT_H_PATHS)

check_format:
	@echo $(FORMAT) -i -style=file --dry-run --Werror "**/*.c" "**/*.h"
	@$(FORMAT) -i --verbose -style=file --dry-run --Werror $(FORMAT_SRC_PATHS) $(FORMAT_H_PATHS)


### DECOMP TOOLS ###

ctx.c: $(C_HEADERS)
	@for header in $(C_HEADERS); do echo "#include \"$$header\""; done > ctx.h
	gcc -P -E -dD -undef -nostdinc -I include -D GEN_CTX=1 -D PLATFORM_GBA=1 -D GAME=GAME_SA2 ctx.h | sed '/^#define __STDC/d' | sed '1s|^|#include <stdint.h>\n|' > ctx.c
	@rm ctx.h
