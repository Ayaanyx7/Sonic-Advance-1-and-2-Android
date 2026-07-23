LOCAL_PATH := $(call my-dir)

# 1. Define the Prebuilt Main Game Decomp Library
include $(CLEAR_VARS)
LOCAL_MODULE    := sa2_game
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libsa2_game.a
include $(PREBUILT_STATIC_LIBRARY)

# 2. Define the Prebuilt agbsyscall Library
include $(CLEAR_VARS)
LOCAL_MODULE    := agbsyscall
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../libagbsyscall/build/android/libagbsyscall.a
include $(PREBUILT_STATIC_LIBRARY)

# 3. Build the Final Native Shared Library for Android
include $(CLEAR_VARS)
LOCAL_MODULE    := main

# Use the exact flat local filename copied by your workflow
LOCAL_CFLAGS    := -fPIC -fno-common -DWIDESCREEN_HACK=1

# ⬇️ POINT DIRECTLY TO THE MAIN REPOSITORY HEADERS ⬇️
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../include \
                    $(LOCAL_PATH)/../../../../libagbsyscall

LOCAL_WHOLE_STATIC_LIBRARIES := sa2_game agbsyscall
LOCAL_SHARED_LIBRARIES       := SDL2
LOCAL_LDLIBS                 := -llog -landroid -Wl,--allow-multiple-definition -Wl,-Bsymbolic
include $(BUILD_SHARED_LIBRARY)
