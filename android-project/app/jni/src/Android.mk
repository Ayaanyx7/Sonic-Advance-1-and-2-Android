LOCAL_PATH := $(call my-dir)

# 1. Define the Prebuilt Main Game Decomp Library
include $(CLEAR_VARS)
LOCAL_MODULE    := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

# 2. Build the Final Native Shared Library for Android
include $(CLEAR_VARS)
LOCAL_MODULE    := main

# ⬇️ ADD THE AGBSYSCALL C FILE SOURCE HERE ⬇️
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libagbsyscall/src/agbsyscall.c

LOCAL_CFLAGS    := -fPIC

# Include headers for the syscall emulation layer
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../libagbsyscall/include

LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game
LOCAL_SHARED_LIBRARIES       := SDL2
LOCAL_LDLIBS                 := -llog -landroid -Wl,-z,notext

include $(BUILD_SHARED_LIBRARY)
