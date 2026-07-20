LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := main

# Mirrors the Makefile's PLATFORM=sdl source selection: exclude win32/ps2
# platform code, keep everything else (shared game logic + pret_sdl backend)
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH)/../../src -name "*.c" \
    -not -path "*/platform/win32/*" \
    -not -path "*/platform/ps2/*" \
    -not -path "*/platform/sdl_psp/*")

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include

LOCAL_CFLAGS := -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 \
    -D CPU_ARCH_X86=0 -D CPU_ARCH_ARM=0 \
    -D GAME=GAME_SA2 -D USA \
    -D DEBUG=0 -D PORTABLE=0 -D TAS_TESTING=0 -D ENABLE_DECOMP_CREDITS=0 \
    -Wno-parentheses-equality -Wno-unused-value

LOCAL_SHARED_LIBRARIES := SDL2

include $(BUILD_SHARED_LIBRARY)
