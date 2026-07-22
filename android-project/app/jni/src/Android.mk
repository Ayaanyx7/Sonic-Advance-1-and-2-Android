LOCAL_PATH := $(call my-dir)

# --- Define Prebuilt Static Library: sa2_game ---
include $(CLEAR_VARS)
LOCAL_MODULE := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

# --- Define Prebuilt Static Library: agbsyscall ---
include $(CLEAR_VARS)
LOCAL_MODULE := agbsyscall
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libagbsyscall/build/android/libagbsyscall.a
include $(PREBUILT_STATIC_LIBRARY)

# --- Final Shared Library Target ---
include $(CLEAR_VARS)
LOCAL_MODULE := main

# Combined source files (Your automated find script + the new stub.c)
LOCAL_SRC_FILES := stub.c \
    $(shell find $(LOCAL_PATH)/../../src -name "*.c" \
    -not -path "*/platform/win32/*" \
    -not -path "*/platform/ps2/*" \
    -not -path "*/platform/sdl_psp/*")

# Header directories from old configuration
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../include

# Compiler flags from old configuration
LOCAL_CFLAGS := -D PLATFORM_GBA=0 -D PLATFORM_SDL=1 -D PLATFORM_WIN32=0 \
    -D CPU_ARCH_X86=0 -D CPU_ARCH_ARM=0 \
    -D GAME=GAME_SA2 -D USA \
    -D DEBUG=0 -D PORTABLE=0 -D TAS_TESTING=0 -D ENABLE_DECOMP_CREDITS=0 \
    -Wno-parentheses-equality -Wno-unused-value

# Link the two new prebuilt static libraries
LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game agbsyscall

# Shared library dependencies
LOCAL_SHARED_LIBRARIES := SDL2

# Linker flags for Android system logging and native window management
LOCAL_LDLIBS := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
