LOCAL_PATH := $(call my-dir)

# 1. Define the Prebuilt Main Game Decomp Library
include $(CLEAR_VARS)
LOCAL_MODULE    := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

# 2. Build the Final Native Library for Android
include $(CLEAR_VARS)
LOCAL_MODULE    := main

# Leave this empty. The game code library has its own entry point!
LOCAL_SRC_FILES := 

LOCAL_CFLAGS    := -fPIC

# Pull in your game logic library
LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game
LOCAL_SHARED_LIBRARIES       := SDL2
LOCAL_LDLIBS                 := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
